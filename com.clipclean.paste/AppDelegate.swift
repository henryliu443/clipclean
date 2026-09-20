import AppKit
import Carbon.HIToolbox
import SwiftUI

/// A resizable panel that can become key even though the app is a menu bar
/// accessory.
final class PanelWindow: NSPanel {
    override var canBecomeKey: Bool { true }
    override var canBecomeMain: Bool { true }
}

final class AppDelegate: NSObject, NSApplicationDelegate, NSWindowDelegate {

    private let controller = ClipboardController()
    private var statusItem: NSStatusItem?
    private var panel: PanelWindow?
    private var model: PanelModel?
    private var panelSizeBeforeSettings: NSSize?
    private var isResizingProgrammatically = false

    func applicationDidFinishLaunching(_ notification: Notification) {
        let model = PanelModel(controller: controller)
        self.model = model

        controller.onStateChange = { [weak self] in
            self?.model?.syncFromController()
            self?.refreshStatusItem()
        }

        model.onLanguageChange = { [weak self] in
            self?.refreshStatusItem()
        }

        model.onShowingSettingsChange = { [weak self] showing in
            // Deferred: resizing the window synchronously from a SwiftUI state
            // change re-enters layout and publishes during a view update.
            DispatchQueue.main.async { [weak self] in
                self?.resizePanelForSettings(showing)
            }
        }

        setUpStatusItem()
        setUpPanel(model: model)
        setUpHotKeys()

        if #available(macOS 26.0, *) {
            NSLog("[Clipclean] 系统检测: %@ → 采用 macOS 26+ 原生液态玻璃 (NSGlassEffectView)", ProcessInfo.processInfo.operatingSystemVersionString)
        } else {
            NSLog("[Clipclean] 系统检测: %@ → 采用 macOS 14–15 半透明材质 (NSVisualEffectView)", ProcessInfo.processInfo.operatingSystemVersionString)
        }

        NotificationCenter.default.addObserver(
            forName: NSApplication.didResignActiveNotification,
            object: nil,
            queue: .main
        ) { [weak self] _ in
            self?.panel?.orderOut(nil)
        }

        NotificationCenter.default.addObserver(
            forName: NSApplication.didChangeScreenParametersNotification,
            object: nil,
            queue: .main
        ) { [weak self] _ in
            guard let self, let panel = self.panel, panel.isVisible else { return }
            self.positionPanel(panel)
        }
    }

    private func setUpStatusItem() {
        let item = NSStatusBar.system.statusItem(withLength: NSStatusItem.variableLength)
        item.button?.imagePosition = .imageOnly
        item.button?.target = self
        item.button?.action = #selector(statusItemClicked)
        item.button?.sendAction(on: [.leftMouseUp, .rightMouseUp])
        statusItem = item
        refreshStatusItem()
    }

    private func setUpPanel(model: PanelModel) {
        let panel = PanelWindow(
            contentRect: NSRect(x: 0, y: 0, width: 240, height: 300),
            styleMask: [.borderless, .resizable],
            backing: .buffered,
            defer: false
        )
        panel.title = "Clipboard Mode"
        panel.isMovableByWindowBackground = false
        panel.isOpaque = false
        panel.backgroundColor = .clear
        // A borderless window's shadow is drawn behind the window, and it shows
        // through the translucent glass as a dark rim. Draw the shadow in
        // SwiftUI instead.
        panel.hasShadow = false
        panel.isReleasedWhenClosed = false
        panel.hidesOnDeactivate = false
        panel.level = .floating
        panel.collectionBehavior = [.moveToActiveSpace, .fullScreenAuxiliary]
        panel.contentMinSize = NSSize(width: 220, height: 280)
        panel.delegate = self

        let hosting = NSHostingView(rootView: ClipboardPanelView(model: model))

        // Native Liquid Glass on macOS 26+, a translucent material below.
        // Doing the background in AppKit avoids the SwiftUI glass container's
        // rectangular backdrop leaking out behind the rounded panel.
        let background: NSView
        if #available(macOS 26.0, *) {
            let glass = NSGlassEffectView()
            glass.style = .regular
            glass.cornerRadius = 14
            glass.contentView = hosting
            background = glass
        } else {
            let effect = NSVisualEffectView()
            effect.material = .hudWindow
            effect.blendingMode = .behindWindow
            effect.state = .active
            effect.wantsLayer = true
            effect.layer?.cornerRadius = 14
            effect.layer?.masksToBounds = true
            effect.addSubview(hosting)
            background = effect
        }

        panel.contentView = background
        hosting.frame = background.bounds
        hosting.autoresizingMask = [.width, .height]

        self.panel = panel
    }

    /// F6 turns Plain Text Mode on, F5 turns it off.
    private func setUpHotKeys() {
        let off = HotKeyCenter.shared.register(keyCode: UInt32(kVK_F5)) { [weak self] in
            self?.controller.setEnabled(false)
        }
        let on = HotKeyCenter.shared.register(keyCode: UInt32(kVK_F6)) { [weak self] in
            self?.controller.setEnabled(true)
        }
        if !on || !off {
            FileHandle.standardError.write(
                Data("Clipboard Mode: could not register F5/F6 hot keys\n".utf8)
            )
        }
    }

    // MARK: - Status item

    @objc private func statusItemClicked() {
        if NSApp.currentEvent?.type == .rightMouseUp {
            showContextMenu()
        } else {
            togglePanel()
        }
    }

    @objc private func togglePanel() {
        guard let panel else { return }
        if panel.isVisible {
            panel.orderOut(nil)
        } else {
            showPanel(panel)
        }
    }

    private func showPanel(_ panel: NSWindow) {
        let frame = targetFrame(for: panel.frame.size)
        panel.setFrame(frame, display: true)
        panel.makeKeyAndOrderFront(nil)
        NSApp.activate()
    }

    private func showContextMenu() {
        guard let statusItem else { return }
        statusItem.menu = makeContextMenu()
        statusItem.button?.performClick(nil)
        statusItem.menu = nil
    }

    /// Mode ▸ On / Off, separator, Settings…, Quit.
    private func makeContextMenu() -> NSMenu {
        let strings = model?.strings ?? Strings(language: .english)
        let menu = NSMenu()

        let modeItem = NSMenuItem(title: strings.mode, action: nil, keyEquivalent: "")
        let modeMenu = NSMenu()

        let onItem = NSMenuItem(title: strings.on, action: #selector(enableMode), keyEquivalent: "")
        onItem.target = self
        onItem.state = controller.isEnabled ? .on : .off
        modeMenu.addItem(onItem)

        let offItem = NSMenuItem(title: strings.off, action: #selector(disableMode), keyEquivalent: "")
        offItem.target = self
        offItem.state = controller.isEnabled ? .off : .on
        modeMenu.addItem(offItem)

        modeItem.submenu = modeMenu
        menu.addItem(modeItem)

        menu.addItem(.separator())

        let settingsItem = NSMenuItem(
            title: strings.settingsEllipsis,
            action: #selector(openSettings),
            keyEquivalent: ""
        )
        settingsItem.target = self
        menu.addItem(settingsItem)

        let quitItem = NSMenuItem(
            title: strings.quit,
            action: #selector(NSApplication.terminate(_:)),
            keyEquivalent: "q"
        )
        quitItem.target = NSApp
        menu.addItem(quitItem)

        return menu
    }

    @objc private func enableMode() {
        controller.setEnabled(true)
    }

    @objc private func disableMode() {
        controller.setEnabled(false)
    }

    @objc private func openSettings() {
        guard let panel else { return }
        model?.setShowingSettings(true)
        if !panel.isVisible {
            showPanel(panel)
        }
    }

    /// The Settings page needs more room than the main view, so grow the panel
    /// when it opens and restore the previous size when it closes.
    private func resizePanelForSettings(_ showing: Bool) {
        guard let panel else { return }

        let targetSize: NSSize
        if showing {
            panelSizeBeforeSettings = panel.frame.size
            let expandedWidth = min(max(panel.frame.width, 340), 340)
            let expandedHeight = min(max(panel.frame.height, 340), 340)
            targetSize = NSSize(width: expandedWidth, height: expandedHeight)
        } else {
            targetSize = panelSizeBeforeSettings ?? NSSize(width: 240, height: 300)
            panelSizeBeforeSettings = nil
        }

        let newFrame = targetFrame(for: targetSize)
        isResizingProgrammatically = true
        panel.setFrame(newFrame, display: true)
        DispatchQueue.main.async { [weak self] in
            self?.isResizingProgrammatically = false
        }
    }

    /// Computes the window frame so the top edge sits directly under the menu
    /// bar and is horizontally centred on the status item.
    private func targetFrame(for size: NSSize) -> NSRect {
        guard let button = statusItem?.button, let buttonWindow = button.window else {
            return NSRect(origin: panel?.frame.origin ?? .zero, size: size)
        }
        let buttonRect = buttonWindow.convertToScreen(button.convert(button.bounds, to: nil))

        var origin = NSPoint(
            x: buttonRect.midX - size.width / 2,
            y: buttonRect.minY - size.height
        )

        if let screen = buttonWindow.screen ?? NSScreen.main {
            let visible = screen.visibleFrame
            origin.x = min(max(origin.x, visible.minX + 8), visible.maxX - size.width - 8)
            origin.y = max(origin.y, visible.minY + 8)
        }

        return NSRect(origin: origin, size: size)
    }

    private func positionPanel(_ panel: NSWindow) {
        let frame = targetFrame(for: panel.frame.size)
        if panel.frame.origin != frame.origin {
            panel.setFrameOrigin(frame.origin)
        }
    }

    func windowDidResize(_ notification: Notification) {
        guard !isResizingProgrammatically, self.panel != nil else { return }
        DispatchQueue.main.async { [weak self] in
            guard let self, !self.isResizingProgrammatically, let panel = self.panel else { return }
            self.positionPanel(panel)
        }
    }

    func windowShouldClose(_ sender: NSWindow) -> Bool {
        sender.orderOut(nil)
        return false
    }

    private func refreshStatusItem() {
        let enabled = controller.isEnabled
        let symbolName = enabled ? "doc.plaintext" : "doc.richtext"
        let image = NSImage(systemSymbolName: symbolName, accessibilityDescription: "Clipboard Mode")
        image?.isTemplate = true

        let strings = model?.strings ?? Strings(language: .english)
        let state = enabled ? strings.on : strings.off

        statusItem?.button?.image = image
        statusItem?.button?.toolTip = "\(strings.appName) — \(strings.plainTextMode): \(state)"
    }
}
