#include "win32/clipboard.h"

#include <cstdlib>
#include <cstring>
#include <vector>

#include <windows.h>

#include "core/plain_text.h"

namespace clipclean {
namespace {

bool openClipboardRetry() {
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (OpenClipboard(nullptr)) return true;
        Sleep(10);
    }
    return false;
}

std::wstring ansiToWide(const std::string& text, UINT codePage) {
    if (text.empty()) return {};
    const int length = MultiByteToWideChar(codePage, 0, text.data(),
                                           static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) return {};
    std::wstring wide(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(codePage, 0, text.data(), static_cast<int>(text.size()),
                        wide.data(), length);
    return wide;
}

std::string readGlobalBytes(HANDLE handle) {
    const SIZE_T size = GlobalSize(handle);
    const char* data = static_cast<const char*>(GlobalLock(handle));
    if (!data) return {};
    const size_t length = strnlen(data, size);
    std::string bytes(data, length);
    GlobalUnlock(handle);
    return bytes;
}

/// Extracts the HTML fragment from the Windows "HTML Format" clipboard payload,
/// whose header carries byte offsets into the same buffer.
std::string extractHtmlFragment(const std::string& raw) {
    auto offsetAfter = [&raw](const char* key) -> long {
        const size_t pos = raw.find(key);
        if (pos == std::string::npos) return -1;
        size_t cursor = pos + std::strlen(key);
        while (cursor < raw.size() && (raw[cursor] == ' ' || raw[cursor] == ':')) ++cursor;
        return std::atol(raw.c_str() + cursor);
    };

    const long start = offsetAfter("StartFragment");
    const long end = offsetAfter("EndFragment");
    if (start >= 0 && end > start && static_cast<size_t>(end) <= raw.size()) {
        return raw.substr(static_cast<size_t>(start), static_cast<size_t>(end - start));
    }

    size_t html = raw.find("<html");
    if (html == std::string::npos) html = raw.find('<');
    return html == std::string::npos ? raw : raw.substr(html);
}

std::wstring canonicalFormatName(UINT format) {
    switch (format) {
        case CF_TEXT: return L"CF_TEXT";
        case CF_BITMAP: return L"CF_BITMAP";
        case CF_OEMTEXT: return L"CF_OEMTEXT";
        case CF_DIB: return L"CF_DIB";
        case CF_UNICODETEXT: return L"CF_UNICODETEXT";
        case CF_HDROP: return L"CF_HDROP";
        case CF_LOCALE: return L"CF_LOCALE";
        case CF_DIBV5: return L"CF_DIBV5";
        default: break;
    }

    wchar_t name[256] = {};
    if (GetClipboardFormatNameW(format, name, 256) > 0) return name;

    wchar_t fallback[32] = {};
    wsprintfW(fallback, L"CF#%u", format);
    return fallback;
}

} // namespace

bool clipboardPlainText(std::wstring& out) {
    out.clear();
    if (!openClipboardRetry()) return false;

    bool found = false;

    if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        if (HANDLE handle = GetClipboardData(CF_UNICODETEXT)) {
            if (const wchar_t* text =
                    static_cast<const wchar_t*>(GlobalLock(handle))) {
                out = text;
                GlobalUnlock(handle);
                found = true;
            }
        }
    }

    if (!found) {
        const UINT rtf = RegisterClipboardFormatW(L"Rich Text Format");
        if (rtf && IsClipboardFormatAvailable(rtf)) {
            if (HANDLE handle = GetClipboardData(rtf)) {
                out = stripRtf(ansiToWide(readGlobalBytes(handle), CP_ACP));
                found = true;
            }
        }
    }

    if (!found) {
        const UINT html = RegisterClipboardFormatW(L"HTML Format");
        if (html && IsClipboardFormatAvailable(html)) {
            if (HANDLE handle = GetClipboardData(html)) {
                const std::string fragment = extractHtmlFragment(readGlobalBytes(handle));
                out = stripHtml(ansiToWide(fragment, CP_UTF8));
                found = true;
            }
        }
    }

    CloseClipboard();
    return found;
}

bool clipboardRequiresPlainTextRewrite() {
    std::vector<std::wstring> names;
    bool hasPlainText = false;
    bool hasFileDrop = false;

    if (!openClipboardRetry()) return false;

    UINT format = 0;
    while ((format = EnumClipboardFormats(format)) != 0) {
        names.push_back(canonicalFormatName(format));
        if (format == CF_UNICODETEXT) hasPlainText = true;
        if (format == CF_HDROP) hasFileDrop = true;
    }

    CloseClipboard();
    return requiresPlainTextRewrite(names, hasPlainText, hasFileDrop);
}

bool rewriteClipboardAsPlainText(const std::wstring& text) {
    if (!openClipboardRetry()) return false;
    if (!EmptyClipboard()) {
        CloseClipboard();
        return false;
    }

    bool wroteUnicode = false;

    const size_t unicodeBytes = (text.size() + 1) * sizeof(wchar_t);
    if (HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, unicodeBytes)) {
        if (void* destination = GlobalLock(handle)) {
            std::memcpy(destination, text.c_str(), unicodeBytes);
            GlobalUnlock(handle);
            if (SetClipboardData(CF_UNICODETEXT, handle)) {
                wroteUnicode = true;
                handle = nullptr;
            }
        }
        if (handle) GlobalFree(handle);
    }

    const int ansiLength = WideCharToMultiByte(CP_ACP, 0, text.c_str(),
                                               static_cast<int>(text.size()),
                                               nullptr, 0, nullptr, nullptr);
    if (ansiLength > 0) {
        const size_t ansiBytes = static_cast<size_t>(ansiLength) + 1;
        if (HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, ansiBytes)) {
            if (char* destination = static_cast<char*>(GlobalLock(handle))) {
                WideCharToMultiByte(CP_ACP, 0, text.c_str(),
                                    static_cast<int>(text.size()), destination,
                                    ansiLength, nullptr, nullptr);
                destination[ansiLength] = '\0';
                GlobalUnlock(handle);
                if (!SetClipboardData(CF_TEXT, handle)) GlobalFree(handle);
            } else {
                GlobalFree(handle);
            }
        }
    }

    CloseClipboard();
    return wroteUnicode;
}

unsigned long clipboardSequenceNumber() {
    return GetClipboardSequenceNumber();
}

} // namespace clipclean
