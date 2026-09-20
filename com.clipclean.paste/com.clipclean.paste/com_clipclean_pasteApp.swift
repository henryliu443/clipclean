//
//  com_clipclean_pasteApp.swift
//  com.clipclean.paste
//

import SwiftUI

@main
struct com_clipclean_pasteApp: App {

    @NSApplicationDelegateAdaptor(AppDelegate.self) private var appDelegate

    var body: some Scene {
        Settings {
            EmptyView()
        }
    }
}
