import AppKit
import Combine
import ServiceManagement

/// Bridges `ClipboardController` to SwiftUI and exposes the settings the glass
/// panel needs.
final class PanelModel: ObservableObject {

    private static let languageDefaultsKey = "appLanguage"

    @Published var isEnabled: Bool
    @Published var launchAtLogin: Bool
    @Published var showingSettings: Bool = false
    @Published var language: AppLanguage

    var strings: Strings { Strings(language: language) }

    /// Called after the language changes so AppKit-side text can be refreshed.
    var onLanguageChange: (() -> Void)?

    /// Called when the panel switches between the main view and Settings so the
    /// window can be resized.
    var onShowingSettingsChange: ((Bool) -> Void)?

    private let controller: ClipboardController
    private let defaults: UserDefaults

    init(controller: ClipboardController, defaults: UserDefaults = .standard) {
        self.controller = controller
        self.defaults = defaults
        self.isEnabled = controller.isEnabled
        self.launchAtLogin = SMAppService.mainApp.status == .enabled

        if let stored = defaults.string(forKey: Self.languageDefaultsKey).flatMap(AppLanguage.init(rawValue:)) {
            self.language = stored
        } else {
            self.language = AppLanguage.systemDefault
        }
    }

    func setEnabled(_ enabled: Bool) {
        guard enabled != isEnabled else { return }
        DispatchQueue.main.async { [weak self] in
            guard let self else { return }
            self.isEnabled = enabled
            self.controller.setEnabled(enabled)
        }
    }

    /// Called when the mode changes from outside SwiftUI (for example a hot key).
    func syncFromController() {
        let current = controller.isEnabled
        guard current != isEnabled else { return }
        DispatchQueue.main.async { [weak self] in
            guard let self else { return }
            guard current != self.isEnabled else { return }
            self.isEnabled = current
        }
    }

    func setLanguage(_ language: AppLanguage) {
        guard language != self.language else { return }
        // Defer to next runloop turn so @Published does not fire while the
        // segmented Picker's internal view update is in progress.
        DispatchQueue.main.async { [weak self] in
            guard let self else { return }
            self.language = language
            self.defaults.set(language.rawValue, forKey: Self.languageDefaultsKey)
            self.onLanguageChange?()
        }
    }

    func setShowingSettings(_ showing: Bool) {
        guard showing != showingSettings else { return }
        // Defer to next runloop turn to break the synchronous view-update cycle
        // and prevent layout recursion.
        DispatchQueue.main.async { [weak self] in
            guard let self else { return }
            self.showingSettings = showing
            self.onShowingSettingsChange?(showing)
        }
    }

    func setLaunchAtLogin(_ enabled: Bool) {
        do {
            if enabled {
                try SMAppService.mainApp.register()
            } else {
                try SMAppService.mainApp.unregister()
            }
        } catch {
            NSLog("Clipboard Mode: could not change launch at login: \(error)")
        }
        let status = SMAppService.mainApp.status == .enabled
        DispatchQueue.main.async { [weak self] in
            self?.launchAtLogin = status
        }
    }
}
