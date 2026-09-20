#include "win32/clipboard_monitor.h"

namespace clipclean {
namespace {

using AddClipboardFormatListenerFn = BOOL(WINAPI*)(HWND);
using RemoveClipboardFormatListenerFn = BOOL(WINAPI*)(HWND);

AddClipboardFormatListenerFn addListenerFn() {
    static AddClipboardFormatListenerFn fn = reinterpret_cast<
        AddClipboardFormatListenerFn>(GetProcAddress(
        GetModuleHandleW(L"user32.dll"), "AddClipboardFormatListener"));
    return fn;
}

RemoveClipboardFormatListenerFn removeListenerFn() {
    static RemoveClipboardFormatListenerFn fn = reinterpret_cast<
        RemoveClipboardFormatListenerFn>(GetProcAddress(
        GetModuleHandleW(L"user32.dll"), "RemoveClipboardFormatListener"));
    return fn;
}

} // namespace

bool ClipboardMonitor::start(HWND owner) {
    if (owner_) return true;
    owner_ = owner;

    if (addListenerFn() && addListenerFn()(owner)) {
        usingListener_ = true;
        return true;
    }

    nextViewer_ = SetClipboardViewer(owner);
    return true;
}

void ClipboardMonitor::stop() {
    if (!owner_) return;

    if (usingListener_) {
        if (removeListenerFn()) removeListenerFn()(owner_);
    } else {
        // Passing the next viewer (possibly null) removes us from the chain.
        ChangeClipboardChain(owner_, nextViewer_);
    }

    owner_ = nullptr;
    nextViewer_ = nullptr;
    usingListener_ = false;
}

void ClipboardMonitor::onChangeChain(HWND remove, HWND next) {
    if (nextViewer_ == remove) {
        nextViewer_ = next;
    } else if (nextViewer_) {
        SendMessageW(nextViewer_, WM_CHANGECBCHAIN, reinterpret_cast<WPARAM>(remove),
                     reinterpret_cast<LPARAM>(next));
    }
}

void ClipboardMonitor::onDrawClipboard() {
    if (handler_) handler_();
    if (nextViewer_) {
        SendMessageW(nextViewer_, WM_DRAWCLIPBOARD, 0, 0);
    }
}

void ClipboardMonitor::onClipboardUpdate() {
    if (handler_) handler_();
}

} // namespace clipclean
