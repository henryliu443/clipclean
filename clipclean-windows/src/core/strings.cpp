#include "core/strings.h"

namespace clipclean {
namespace {

const Strings kEnglish{
    Language::English,
    L"Clipboard Mode",
    L"Mode",
    L"On",
    L"Off",
    L"Plain Text Mode",
    L"Settings",
    L"Settings\u2026",
    L"Launch at Login",
    L"Appearance",
    L"Dark Mode",
    L"Follow System",
    L"Shortcuts",
    L"Language",
    L"Turn Plain Text Mode on",
    L"Turn Plain Text Mode off",
    L"Quit Clipboard Mode",
    L"F6 to turn on \u00B7 F5 to turn off",
    L"Clipboard Mode",
    L"Running in the background. Click the tray icon to open it.",
};

const Strings kChinese{
    Language::Chinese,
    L"\u526A\u8D34\u677F\u6A21\u5F0F",
    L"\u6A21\u5F0F",
    L"\u5F00",
    L"\u5173",
    L"\u7EAF\u6587\u672C\u6A21\u5F0F",
    L"\u8BBE\u7F6E",
    L"\u8BBE\u7F6E\u2026",
    L"\u5F00\u673A\u81EA\u542F",
    L"\u5916\u89C2",
    L"\u6DF1\u8272\u6A21\u5F0F",
    L"\u8DDF\u968F\u7CFB\u7EDF",
    L"\u5FEB\u6377\u952E",
    L"\u8BED\u8A00",
    L"\u5F00\u542F\u7EAF\u6587\u672C\u6A21\u5F0F",
    L"\u5173\u95ED\u7EAF\u6587\u672C\u6A21\u5F0F",
    L"\u9000\u51FA\u526A\u8D34\u677F\u6A21\u5F0F",
    L"F6 \u5F00\u542F \u00B7 F5 \u5173\u95ED",
    L"\u526A\u8D34\u677F\u6A21\u5F0F",
    L"\u5DF2\u5728\u540E\u53F0\u8FD0\u884C\u3002\u70B9\u51FB\u6258\u76D8\u56FE\u6807\u6253\u5F00\u3002",
};

} // namespace

const Strings& stringsFor(Language language) {
    return language == Language::Chinese ? kChinese : kEnglish;
}

const wchar_t* languageDisplayName(Language language) {
    return language == Language::Chinese ? L"\u4E2D\u6587" : L"English";
}

} // namespace clipclean
