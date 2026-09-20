import SwiftUI

extension View {

    /// Liquid Glass on macOS 26+, and a translucent material on older systems.
    @ViewBuilder
    func clipcleanGlass<S: Shape>(in shape: S) -> some View {
        if #available(macOS 26.0, *) {
            self.glassEffect(.regular, in: shape)
        } else {
            self.background(.ultraThinMaterial, in: shape)
        }
    }
}
