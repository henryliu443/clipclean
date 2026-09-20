#include "core/plain_text.h"

#include <cstdint>
#include <cwctype>

namespace clipclean {
namespace {

bool isAlpha(wchar_t c) {
    return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z');
}

bool isDigit(wchar_t c) {
    return c >= L'0' && c <= L'9';
}

int hexValue(wchar_t c) {
    if (c >= L'0' && c <= L'9') return c - L'0';
    if (c >= L'a' && c <= L'f') return c - L'a' + 10;
    if (c >= L'A' && c <= L'F') return c - L'A' + 10;
    return 0;
}

/// RTF destinations whose content is metadata rather than visible text.
bool isSkippedDestination(const std::wstring& word) {
    static const wchar_t* kSkipped[] = {
        L"fonttbl", L"colortbl",   L"stylesheet", L"info",     L"pict",
        L"header",  L"footer",     L"footerl",    L"footerr",  L"footnote",
        L"bkmkstart", L"bkmkend",  L"fldinst",    L"listtable",
        L"listoverridetable",      L"rsidtbl",    L"generator", L"filetbl",
        L"revtbl",  L"xmlnstbl",   L"datastore",  L"themedata", L"colorschememapping",
    };
    for (const wchar_t* candidate : kSkipped) {
        if (word == candidate) return true;
    }
    return false;
}

void appendCodePoint(std::wstring& out, unsigned code) {
    if (code <= 0xFFFF) {
        out.push_back(static_cast<wchar_t>(code));
    } else if (code <= 0x10FFFF) {
        code -= 0x10000;
        out.push_back(static_cast<wchar_t>(0xD800 + (code >> 10)));
        out.push_back(static_cast<wchar_t>(0xDC00 + (code & 0x3FF)));
    }
}

} // namespace

std::wstring stripRtf(const std::wstring& rtf) {
    std::wstring out;
    std::vector<bool> skipStack;
    bool skipping = false;

    const size_t n = rtf.size();
    size_t i = 0;
    while (i < n) {
        const wchar_t c = rtf[i];

        if (c == L'{') {
            skipStack.push_back(skipping);
            ++i;
            if (i < n && rtf[i] == L'\\') {
                size_t j = i + 1;
                if (j < n && rtf[j] == L'*') {
                    skipping = true;
                    i = j + 1;
                    continue;
                }
                std::wstring word;
                while (j < n && isAlpha(rtf[j])) word.push_back(rtf[j++]);
                if (isSkippedDestination(word)) skipping = true;
            }
            continue;
        }

        if (c == L'}') {
            if (!skipStack.empty()) {
                skipping = skipStack.back();
                skipStack.pop_back();
            } else {
                skipping = false;
            }
            ++i;
            continue;
        }

        if (skipping) {
            ++i;
            continue;
        }

        if (c == L'\\') {
            ++i;
            if (i >= n) break;
            const wchar_t d = rtf[i];

            if (isAlpha(d)) {
                std::wstring word;
                while (i < n && isAlpha(rtf[i])) word.push_back(rtf[i++]);

                bool negative = false;
                bool hasParam = false;
                long param = 0;
                if (i < n && (rtf[i] == L'-' || isDigit(rtf[i]))) {
                    hasParam = true;
                    if (rtf[i] == L'-') {
                        negative = true;
                        ++i;
                    }
                    while (i < n && isDigit(rtf[i])) {
                        param = param * 10 + (rtf[i] - L'0');
                        ++i;
                    }
                    if (negative) param = -param;
                }
                if (i < n && rtf[i] == L' ') ++i;

                if (word == L"par" || word == L"line") {
                    out.push_back(L'\n');
                } else if (word == L"tab") {
                    out.push_back(L'\t');
                } else if (word == L"u" && hasParam) {
                    // RTF stores \uN as a signed 16-bit value: U+FF01 is written
                    // \u-255 and astral chars use surrogate halves. Take the low
                    // 16 bits so those forms decode instead of being dropped.
                    appendCodePoint(out,
                                    static_cast<unsigned>(
                                        static_cast<unsigned short>(param)));
                    // Consume the ANSI fallback that follows \uN.
                    if (i + 3 < n && rtf[i] == L'\\' && rtf[i + 1] == L'\'') {
                        i += 4;
                    }
                }
                continue;
            }

            if (d == L'\'') {
                if (i + 2 < n) {
                    const int value = hexValue(rtf[i + 1]) * 16 + hexValue(rtf[i + 2]);
                    out.push_back(static_cast<wchar_t>(value));
                    i += 3;
                } else {
                    ++i;
                }
                continue;
            }

            switch (d) {
                case L'~': out.push_back(L'\u00A0'); break;
                case L'_': out.push_back(L'\u2011'); break;
                case L'{': out.push_back(L'{'); break;
                case L'}': out.push_back(L'}'); break;
                case L'\\': out.push_back(L'\\'); break;
                default: break;
            }
            ++i;
            continue;
        }

        out.push_back(c);
        ++i;
    }

    return out;
}

std::wstring stripHtml(const std::wstring& html) {
    std::wstring out;
    const size_t n = html.size();
    size_t i = 0;

    auto skipTo = [&](const std::wstring& closeTag) {
        const size_t found = html.find(closeTag, i);
        i = (found == std::wstring::npos) ? n : found + closeTag.size();
    };

    // True when `tag`'s name is exactly `name`, optionally followed by
    // whitespace, '/' or '>'. Avoids matching </html> as a heading, </pre> as a
    // paragraph or </link> as a list item.
    auto isNamedTag = [](const std::wstring& tag, const std::wstring& name) {
        if (tag.size() < name.size()) return false;
        if (tag.compare(0, name.size(), name) != 0) return false;
        if (tag.size() == name.size()) return true;
        const wchar_t c = tag[name.size()];
        return c == L' ' || c == L'\t' || c == L'/' || c == L'>';
    };

    while (i < n) {
        const wchar_t c = html[i];
        if (c == L'<') {
            const size_t end = html.find(L'>', i);
            if (end == std::wstring::npos) break;

            std::wstring tag = html.substr(i + 1, end - i - 1);
            for (wchar_t& ch : tag) ch = static_cast<wchar_t>(towlower(ch));
            const bool closing = !tag.empty() && tag[0] == L'/';

            if (!closing && (tag.rfind(L"script", 0) == 0 || tag.rfind(L"style", 0) == 0)) {
                const std::wstring closeTag =
                    (tag.rfind(L"script", 0) == 0) ? L"</script>" : L"</style>";
                i = end + 1;
                skipTo(closeTag);
                continue;
            }

            if (!closing && isNamedTag(tag, L"br")) {
                out.push_back(L'\n');
            } else if (isNamedTag(tag, L"/p") || isNamedTag(tag, L"/div") ||
                       isNamedTag(tag, L"/li") || isNamedTag(tag, L"/tr") ||
                       isNamedTag(tag, L"/h1") || isNamedTag(tag, L"/h2") ||
                       isNamedTag(tag, L"/h3") || isNamedTag(tag, L"/h4") ||
                       isNamedTag(tag, L"/h5") || isNamedTag(tag, L"/h6")) {
                out.push_back(L'\n');
            } else if (isNamedTag(tag, L"td")) {
                out.push_back(L'\t');
            }

            i = end + 1;
            continue;
        }

        if (c == L'&') {
            const size_t semi = html.find(L';', i);
            if (semi != std::wstring::npos && semi - i <= 10) {
                const std::wstring entity = html.substr(i + 1, semi - i - 1);
                if (entity == L"amp") {
                    out.push_back(L'&');
                } else if (entity == L"lt") {
                    out.push_back(L'<');
                } else if (entity == L"gt") {
                    out.push_back(L'>');
                } else if (entity == L"quot") {
                    out.push_back(L'"');
                } else if (entity == L"apos") {
                    out.push_back(L'\'');
                } else if (entity == L"nbsp") {
                    out.push_back(L'\u00A0');
                } else if (!entity.empty() && entity[0] == L'#') {
                    unsigned code = 0;
                    if (entity.size() > 1 && (entity[1] == L'x' || entity[1] == L'X')) {
                        for (size_t k = 2; k < entity.size(); ++k) {
                            code = code * 16 + static_cast<unsigned>(hexValue(entity[k]));
                        }
                    } else {
                        for (size_t k = 1; k < entity.size(); ++k) {
                            if (!isDigit(entity[k])) {
                                code = 0;
                                break;
                            }
                            code = code * 10 + static_cast<unsigned>(entity[k] - L'0');
                        }
                    }
                    appendCodePoint(out, code);
                } else {
                    out.append(html, i, semi - i + 1);
                }
                i = semi + 1;
                continue;
            }
        }

        out.push_back(c);
        ++i;
    }

    return out;
}

bool isPlainTextFormat(const std::wstring& formatName) {
    return formatName == L"CF_UNICODETEXT" || formatName == L"CF_TEXT" ||
           formatName == L"CF_OEMTEXT" || formatName == L"CF_LOCALE";
}

bool requiresPlainTextRewrite(const std::vector<std::wstring>& formatNames,
                              bool hasPlainText,
                              bool hasFileDrop) {
    if (formatNames.empty()) return false;
    if (hasFileDrop) return false;

    if (hasPlainText) {
        // Plain text is available: strip anything that is not plain text.
        for (const std::wstring& name : formatNames) {
            if (!isPlainTextFormat(name)) return true;
        }
        return false;
    }

    // No plain text at all. Only act on formats we can losslessly turn into
    // text (RTF / HTML); leave images and anything else untouched.
    for (const std::wstring& name : formatNames) {
        if (name == L"Rich Text Format" || name == L"HTML Format") return true;
    }
    return false;
}

} // namespace clipclean
