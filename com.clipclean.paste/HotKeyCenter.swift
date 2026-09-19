import Carbon.HIToolbox
import Foundation

/// Registers system-wide hot keys using the Carbon Hot Key API.
///
/// Carbon hot keys do not require Accessibility or Input Monitoring
/// permissions, which keeps the app free of unnecessary permission prompts.
final class HotKeyCenter {

    static let shared = HotKeyCenter()

    private static let signature: OSType = 0x434C4D44 // 'CLMD'

    private var actions: [UInt32: () -> Void] = [:]
    private var hotKeyRefs: [UInt32: EventHotKeyRef] = [:]
    private var eventHandler: EventHandlerRef?
    private var nextID: UInt32 = 1

    private init() {
        var eventType = EventTypeSpec(
            eventClass: OSType(kEventClassKeyboard),
            eventKind: UInt32(kEventHotKeyPressed)
        )

        InstallEventHandler(
            GetApplicationEventTarget(),
            { _, event, _ -> OSStatus in
                guard let event else { return OSStatus(eventNotHandledErr) }

                var hotKeyID = EventHotKeyID()
                let status = GetEventParameter(
                    event,
                    EventParamName(kEventParamDirectObject),
                    EventParamType(typeEventHotKeyID),
                    nil,
                    MemoryLayout<EventHotKeyID>.size,
                    nil,
                    &hotKeyID
                )
                guard status == noErr else { return status }

                HotKeyCenter.shared.actions[hotKeyID.id]?()
                return noErr
            },
            1,
            &eventType,
            nil,
            &eventHandler
        )
    }

    /// Registers a hot key. Returns `false` when the system refuses it
    /// (for example because another app already owns the combination).
    @discardableResult
    func register(keyCode: UInt32, modifiers: UInt32 = 0, action: @escaping () -> Void) -> Bool {
        let id = nextID
        nextID += 1

        let hotKeyID = EventHotKeyID(signature: Self.signature, id: id)
        var ref: EventHotKeyRef?
        let status = RegisterEventHotKey(
            keyCode,
            modifiers,
            hotKeyID,
            GetApplicationEventTarget(),
            0,
            &ref
        )

        guard status == noErr, let ref else { return false }

        actions[id] = action
        hotKeyRefs[id] = ref
        return true
    }
}
