import AppKit

/// Converts arbitrary pasteboard contents into a single plain-text representation.
///
/// The transformer never invents content: if the pasteboard has no usable text
/// representation the callers are expected to leave the pasteboard untouched.
public enum PlainTextTransformer {

    /// Pasteboard types that carry nothing more than plain text.
    ///
    /// `setString(_:forType:)` also publishes the legacy `NSStringPboardType`
    /// alongside `public.utf8-plain-text`, so both must count as plain text or
    /// an ordinary text copy would be rewritten on every change.
    private static let plainTextTypes: Set<NSPasteboard.PasteboardType> = [
        .string,
        NSPasteboard.PasteboardType("NSStringPboardType"),
        NSPasteboard.PasteboardType("public.utf16-plain-text"),
        NSPasteboard.PasteboardType("public.utf16-external-plain-text"),
        NSPasteboard.PasteboardType("NeXT plain ascii pasteboard type")
    ]

    /// Returns the best available plain-text representation of the pasteboard
    /// contents, or `nil` when the contents cannot be represented as text.
    ///
    /// Preference order:
    /// 1. An explicit plain-text flavour (`public.utf8-plain-text`).
    /// 2. Rich Text Format, flattened to its characters.
    /// 3. HTML, flattened to its characters.
    ///
    /// File (URL) copies are deliberately ignored so that copying a file in the
    /// Finder keeps behaving like a file copy.
    public static func plainText(from pasteboard: NSPasteboard) -> String? {
        let types = pasteboard.types ?? []
        guard !types.isEmpty else { return nil }
        if types.contains(.fileURL) { return nil }

        if let text = pasteboard.string(forType: .string) {
            return text
        }

        if let data = pasteboard.data(forType: .rtf),
           let attributed = NSAttributedString(rtf: data, documentAttributes: nil) {
            return attributed.string
        }

        if let data = pasteboard.data(forType: .html),
           let attributed = NSAttributedString(html: data, documentAttributes: nil) {
            return attributed.string
        }

        return nil
    }

    /// Whether the pasteboard carries a usable text representation *and* carries
    /// something beyond plain text, and therefore should be rewritten to plain
    /// text only.
    public static func requiresPlainTextRewrite(_ pasteboard: NSPasteboard) -> Bool {
        let types = pasteboard.types ?? []
        guard !types.isEmpty else { return false }
        if types.contains(.fileURL) { return false }
        guard plainText(from: pasteboard) != nil else { return false }
        return types.contains { !plainTextTypes.contains($0) }
    }

    /// Replaces the pasteboard contents with a single plain-text representation.
    @discardableResult
    public static func rewrite(_ pasteboard: NSPasteboard, asPlainText text: String) -> Bool {
        pasteboard.clearContents()
        return pasteboard.setString(text, forType: .string)
    }
}
