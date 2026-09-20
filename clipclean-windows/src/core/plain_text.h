#pragma once

#include <string>
#include <vector>

namespace clipclean {

/// Flattens Rich Text Format content to its characters. This is a fallback for
/// clipboards that expose RTF without a plain-text flavour.
std::wstring stripRtf(const std::wstring& rtf);

/// Flattens an HTML fragment to its characters.
std::wstring stripHtml(const std::wstring& html);

/// Whether a clipboard format carries nothing beyond plain text.
///
/// Standard formats are passed using canonical names (`CF_UNICODETEXT`,
/// `CF_TEXT`, `CF_OEMTEXT`, `CF_LOCALE`); registered formats keep their own
/// name (for example `Rich Text Format`). Keeping this name-based means the
/// core layer stays free of the Windows ABI.
bool isPlainTextFormat(const std::wstring& formatName);

/// Whether a clipboard exposing these formats should be reduced to plain text.
///
/// Mirrors PlainTextTransformer.requiresPlainTextRewrite. `hasText` is true
/// when a usable text representation exists at all, whether plain, RTF or
/// HTML, so an RTF-only or HTML-only clipboard is still reduced. File copies
/// and clipboards without usable text are left untouched.
bool requiresPlainTextRewrite(const std::vector<std::wstring>& formatNames,
                              bool hasText,
                              bool hasFileDrop);

} // namespace clipclean
