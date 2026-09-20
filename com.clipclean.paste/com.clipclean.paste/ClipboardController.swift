import AppKit

/// Owns the plain-text mode state and wires the pasteboard monitor to the
/// transformer. The state is persisted so the toggle survives relaunches.
final class ClipboardController {

    private static let defaultsKey = "plainTextModeEnabled"

    private let defaults: UserDefaults
    private lazy var monitor = PasteboardMonitor { [weak self] pasteboard in
        self?.handlePasteboardChange(pasteboard)
    }

    /// Called on the main thread whenever the enabled state changes.
    var onStateChange: (() -> Void)?

    private(set) var isEnabled: Bool

    init(defaults: UserDefaults = .standard) {
        self.defaults = defaults
        self.isEnabled = defaults.bool(forKey: Self.defaultsKey)
        applyState()
    }

    func setEnabled(_ enabled: Bool) {
        guard enabled != isEnabled else { return }
        isEnabled = enabled
        defaults.set(enabled, forKey: Self.defaultsKey)
        applyState()
        onStateChange?()
    }

    func toggle() {
        setEnabled(!isEnabled)
    }

    private func applyState() {
        if isEnabled {
            monitor.start()
        } else {
            monitor.stop()
        }
    }

    private func handlePasteboardChange(_ pasteboard: NSPasteboard) {
        guard isEnabled else { return }
        guard PlainTextTransformer.requiresPlainTextRewrite(pasteboard),
              let text = PlainTextTransformer.plainText(from: pasteboard) else {
            return
        }
        PlainTextTransformer.rewrite(pasteboard, asPlainText: text)
        monitor.acknowledgeCurrentContents()
    }
}
