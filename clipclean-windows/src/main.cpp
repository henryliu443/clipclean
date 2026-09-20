#include <windows.h>

#include "app.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Clipclean.SingleInstance");
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        if (HWND existing =
                FindWindowW(L"ClipcleanMessageWindow", L"Clipclean")) {
            PostMessageW(existing, clipclean::kShowPanelMessage, 0, 0);
        }
        CloseHandle(mutex);
        return 0;
    }

    clipclean::App app;
    if (!app.initialize(instance)) {
        if (mutex) CloseHandle(mutex);
        return 1;
    }

    const int code = app.run();
    if (mutex) CloseHandle(mutex);
    return code;
}
