import SwiftUI

/// The Liquid Glass panel shown from the menu bar item.
struct ClipboardPanelView: View {

    @ObservedObject var model: PanelModel

    var body: some View {
        Group {
            if model.showingSettings {
                SettingsView(model: model)
            } else {
                MainView(model: model)
            }
        }
        .padding(16)
        .frame(minWidth: 260, minHeight: 280)
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .top)
        .glassEffect(.regular, in: RoundedRectangle(cornerRadius: 14, style: .continuous))
    }
}

private struct MainView: View {

    @ObservedObject var model: PanelModel

    private var isOn: Binding<Bool> {
        Binding(
            get: { model.isEnabled },
            set: { model.setEnabled($0) }
        )
    }

    var body: some View {
        let strings = model.strings

        VStack(spacing: 0) {
            HStack {
                Spacer()
                Button {
                    model.setShowingSettings(true)
                } label: {
                    Image(systemName: "gearshape")
                        .font(.system(size: 14, weight: .semibold))
                        .frame(width: 32, height: 32)
                        .contentShape(Circle())
                }
                .buttonStyle(.plain)
                .glassEffect(.regular, in: Circle())
                .contentShape(Circle())
                .help(strings.settings)
            }

            Spacer(minLength: 12)

            VStack(spacing: 12) {
                Image(systemName: model.isEnabled ? "doc.plaintext.fill" : "doc.richtext")
                    .font(.system(size: 40))
                    .foregroundStyle(model.isEnabled ? Color.accentColor : Color.secondary)

                Text(strings.plainTextMode)
                    .font(.headline)

                BigSwitch(isOn: isOn, label: strings.plainTextMode)

                Text(model.isEnabled ? strings.on : strings.off)
                    .font(.subheadline.weight(.medium))
                    .foregroundStyle(model.isEnabled ? Color.accentColor : Color.secondary)

                Text(strings.hotKeyHint)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .padding(.top, 2)
            }

            Spacer(minLength: 12)
        }
    }
}

/// A large, circular-knob switch that slides left and right.
private struct BigSwitch: View {

    @Binding var isOn: Bool
    let label: String

    private let trackWidth: CGFloat = 132
    private let knobSize: CGFloat = 58
    private let inset: CGFloat = 5

    private var travel: CGFloat {
        (trackWidth - knobSize) / 2 - inset
    }

    var body: some View {
        Button {
            isOn.toggle()
        } label: {
            Capsule()
                .fill(isOn ? Color.accentColor : Color.secondary.opacity(0.28))
                .frame(width: trackWidth, height: knobSize + inset * 2)
                .overlay {
                    Circle()
                        .fill(.white)
                        .frame(width: knobSize, height: knobSize)
                        .shadow(color: .black.opacity(0.18), radius: 2, y: 1)
                        .offset(x: isOn ? travel : -travel)
                }
                .animation(.spring(response: 0.3, dampingFraction: 0.75), value: isOn)
        }
        .buttonStyle(.plain)
        .accessibilityLabel(label)
    }
}

private struct SettingsView: View {

    @ObservedObject var model: PanelModel

    var body: some View {
        let strings = model.strings

        VStack(alignment: .leading, spacing: 16) {
            HStack {
                Button {
                    model.setShowingSettings(false)
                } label: {
                    Image(systemName: "chevron.left")
                        .font(.system(size: 13, weight: .bold))
                        .frame(width: 32, height: 32)
                        .contentShape(Circle())
                }
                .buttonStyle(.plain)
                .glassEffect(.regular, in: Circle())
                .contentShape(Circle())
                .keyboardShortcut(.escape, modifiers: [])

                Text(strings.settings)
                    .font(.headline)
                Spacer()
            }

            Toggle(isOn: Binding(
                get: { model.launchAtLogin },
                set: { model.setLaunchAtLogin($0) }
            )) {
                Text(strings.launchAtLogin)
            }
            .toggleStyle(.switch)

            VStack(alignment: .leading, spacing: 8) {
                Text(strings.shortcuts)
                    .font(.subheadline.weight(.semibold))
                shortcutRow("F6", strings.turnOn)
                shortcutRow("F5", strings.turnOff)
            }

            VStack(alignment: .leading, spacing: 8) {
                Text(strings.languageLabel)
                    .font(.subheadline.weight(.semibold))
                Picker("", selection: Binding(
                    get: { model.language },
                    set: { model.setLanguage($0) }
                )) {
                    ForEach(AppLanguage.allCases) { language in
                        Text(language.displayName).tag(language)
                    }
                }
                .pickerStyle(.segmented)
                .labelsHidden()
            }

            Divider()

            Button {
                NSApp.terminate(nil)
            } label: {
                Text(strings.quit)
                    .frame(maxWidth: .infinity)
            }
        }
    }

    private func shortcutRow(_ key: String, _ description: String) -> some View {
        HStack(spacing: 8) {
            Text(key)
                .font(.system(.caption, design: .rounded).weight(.semibold))
                .padding(.horizontal, 8)
                .padding(.vertical, 3)
                .glassEffect(.regular, in: RoundedRectangle(cornerRadius: 6))
            Text(description)
                .font(.caption)
            Spacer()
        }
    }
}
