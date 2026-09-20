#include <cstdio>
#include <string>

#include "core/plain_text.h"

using namespace clipclean;

namespace {

int failures = 0;

void check(bool condition, const char* expression, int line) {
    if (!condition) {
        std::printf("FAIL line %d: %s\n", line, expression);
        ++failures;
    }
}

} // namespace

#define CHECK(expr) check((expr), #expr, __LINE__)

int main() {
    // RTF flattening.
    CHECK(stripRtf(L"{\\rtf1\\ansi Hello \\b world\\b0!}") == L"Hello world!");
    CHECK(stripRtf(L"line1\\par line2") == L"line1\nline2");
    CHECK(stripRtf(L"{\\fonttbl{\\f0 Arial;}}Text") == L"Text");
    CHECK(stripRtf(L"tab\\tab x") == L"tab\tx");
    CHECK(stripRtf(L"{\\*\\generator Foo;}Body") == L"Body");
    // Unicode: \uN escapes, with and without the ANSI fallback that follows.
    CHECK(stripRtf(L"\\u20013\\u25991\\u65281") == L"\u4E2D\u6587\uFF01");
    CHECK(stripRtf(L"\\u20013\\u25991\\u-255") == L"\u4E2D\u6587\uFF01");
    CHECK(stripRtf(L"\\u20013\\'d6\\u25991\\'ce") == L"\u4E2D\u6587");
    // Astral plane arrives as a surrogate pair of signed \u values.
    CHECK(stripRtf(L"\\u-10180\\u-8311") == L"\U0001F389");

    // HTML flattening.
    CHECK(stripHtml(L"<p>Hello <b>world</b></p>") == L"Hello world\n");
    CHECK(stripHtml(L"a &amp; b") == L"a & b");
    CHECK(stripHtml(L"<script>x = 1;</script>ok") == L"ok");
    CHECK(stripHtml(L"one<br>two") == L"one\ntwo");
    // Closing tags must match the whole name: </html> is not a heading,
    // </pre> is not a paragraph and </link> is not a list item.
    CHECK(stripHtml(L"<html><body>x</body></html>") == L"x");
    CHECK(stripHtml(L"<pre>code</pre>") == L"code");
    CHECK(stripHtml(L"<link rel=x>y") == L"y");
    CHECK(stripHtml(L"<h1>t</h1>") == L"t\n");
    CHECK(stripHtml(L"<div>a</div>") == L"a\n");
    // Unicode: numeric entities and astral characters (surrogate pairs).
    CHECK(stripHtml(L"&#x4E2D;&#25991;") == L"\u4E2D\u6587");
    CHECK(stripHtml(L"<p>\U0001F389</p>") == L"\U0001F389\n");

    // Format classification.
    CHECK(isPlainTextFormat(L"CF_UNICODETEXT"));
    CHECK(isPlainTextFormat(L"CF_TEXT"));
    CHECK(!isPlainTextFormat(L"HTML Format"));
    CHECK(!isPlainTextFormat(L"CF_DIB"));

    // Rewrite decision.
    CHECK(requiresPlainTextRewrite({L"CF_UNICODETEXT", L"HTML Format"}, true, false));
    CHECK(!requiresPlainTextRewrite({L"CF_UNICODETEXT"}, true, false));
    CHECK(!requiresPlainTextRewrite({L"CF_HDROP"}, false, true));
    CHECK(!requiresPlainTextRewrite({L"CF_DIB"}, false, false));
    // RTF / HTML are flattened even without a plain-text companion...
    CHECK(requiresPlainTextRewrite({L"Rich Text Format"}, false, false));
    CHECK(requiresPlainTextRewrite({L"HTML Format"}, false, false));
    // ...but images and file drops are always left alone.
    CHECK(!requiresPlainTextRewrite({L"CF_BITMAP", L"CF_DIB"}, false, false));
    CHECK(!requiresPlainTextRewrite({L"CF_DIB", L"Rich Text Format"}, false, true));

    if (failures == 0) {
        std::printf("plain_text: all checks passed\n");
    }
    return failures == 0 ? 0 : 1;
}
