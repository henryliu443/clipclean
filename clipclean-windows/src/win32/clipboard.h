#pragma once

#include <string>

namespace clipclean {

/// Best plain-text representation of the current clipboard, if any.
bool clipboardPlainText(std::wstring& out);

/// Whether the clipboard should be reduced to plain text. Mirrors
/// PlainTextTransformer.requiresPlainTextRewrite.
bool clipboardRequiresPlainTextRewrite();

/// Replaces the clipboard contents with plain text only.
bool rewriteClipboardAsPlainText(const std::wstring& text);

/// Monotonic counter incremented by the system on every clipboard change.
unsigned long clipboardSequenceNumber();

} // namespace clipclean
