import Foundation

enum AppLanguage: String, CaseIterable, Identifiable {
    case english = "en"
    case chinese = "zh"

    var id: String { rawValue }

    var displayName: String {
        switch self {
        case .english: return "English"
        case .chinese: return "中文"
        }
    }

    /// Follows the system language on first launch.
    static var systemDefault: AppLanguage {
        let preferred = Locale.preferredLanguages.first ?? "en"
        return preferred.hasPrefix("zh") ? .chinese : .english
    }
}

/// All user-facing strings. Kept in one place so the language switch can swap
/// the whole interface at once.
struct Strings {

    let language: AppLanguage

    private func pick(_ english: String, _ chinese: String) -> String {
        language == .chinese ? chinese : english
    }

    var appName: String { pick("Clipboard Mode", "剪贴板模式") }

    var mode: String { pick("Mode", "模式") }
    var on: String { pick("On", "开") }
    var off: String { pick("Off", "关") }

    var plainTextMode: String { pick("Plain Text Mode", "纯文本模式") }

    var settings: String { pick("Settings", "设置") }
    var settingsEllipsis: String { pick("Settings…", "设置…") }

    var launchAtLogin: String { pick("Launch at Login", "开机自启") }
    var shortcuts: String { pick("Shortcuts", "快捷键") }
    var languageLabel: String { pick("Language", "语言") }

    var turnOn: String { pick("Turn Plain Text Mode on", "开启纯文本模式") }
    var turnOff: String { pick("Turn Plain Text Mode off", "关闭纯文本模式") }

    var quit: String { pick("Quit Clipboard Mode", "退出剪贴板模式") }

    var hotKeyHint: String { pick("F6 to turn on · F5 to turn off", "F6 开启 · F5 关闭") }
}
