import AppKit

/// Watches a pasteboard for changes using its change count.
///
/// macOS has no public pasteboard-changed notification, so a run-loop timer
/// polling `changeCount` is the simplest reliable approach. The timer is only
/// created while monitoring is active, so the app costs nothing when the
/// feature is off.
public final class PasteboardMonitor {

    public typealias Handler = (NSPasteboard) -> Void

    private let pasteboard: NSPasteboard
    private let interval: TimeInterval
    private let handler: Handler
    private var timer: Timer?
    private var lastObservedChangeCount: Int

    public init(
        pasteboard: NSPasteboard = .general,
        interval: TimeInterval = 0.3,
        handler: @escaping Handler
    ) {
        self.pasteboard = pasteboard
        self.interval = interval
        self.handler = handler
        self.lastObservedChangeCount = pasteboard.changeCount
    }

    public var isRunning: Bool { timer != nil }

    /// Starts observing. Safe to call repeatedly.
    public func start() {
        guard timer == nil else { return }
        lastObservedChangeCount = pasteboard.changeCount

        let timer = Timer(timeInterval: interval, repeats: true) { [weak self] _ in
            self?.checkForChanges()
        }
        timer.tolerance = interval / 2
        RunLoop.main.add(timer, forMode: .common)
        self.timer = timer
    }

    /// Stops observing and releases the timer.
    public func stop() {
        timer?.invalidate()
        timer = nil
    }

    /// Marks the current pasteboard contents as already observed.
    ///
    /// Call this immediately after this process writes to the pasteboard so the
    /// monitor does not react to its own write and create a feedback loop.
    public func acknowledgeCurrentContents() {
        lastObservedChangeCount = pasteboard.changeCount
    }

    /// Polls once. Exposed for deterministic testing; normally driven by `start()`.
    func checkForChanges() {
        guard isRunning else { return }
        let count = pasteboard.changeCount
        guard count != lastObservedChangeCount else { return }
        lastObservedChangeCount = count
        handler(pasteboard)
    }
}
