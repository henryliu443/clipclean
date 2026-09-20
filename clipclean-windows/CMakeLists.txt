cmake_minimum_required(VERSION 3.16)
project(Clipclean LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "This project targets Windows (MSVC or MinGW-w64).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CLIPCLEAN_SOURCES
    src/main.cpp
    src/app.cpp
    src/core/plain_text.cpp
    src/core/strings.cpp
    src/win32/autostart.cpp
    src/win32/clipboard.cpp
    src/win32/clipboard_monitor.cpp
    src/win32/hotkeys.cpp
    src/win32/settings_store.cpp
    src/win32/tray_icon.cpp
    src/win32/win_util.cpp
    src/ui/panel_view.cpp
    src/ui/panel_window.cpp
    src/ui/switch_control.cpp
    src/ui/theme.cpp
    res/app.rc
)

add_executable(Clipclean WIN32 ${CLIPCLEAN_SOURCES})

target_include_directories(Clipclean PRIVATE src res)

target_compile_definitions(Clipclean PRIVATE
    UNICODE
    _UNICODE
    WINVER=0x0601
    _WIN32_WINNT=0x0601
)

target_link_libraries(Clipclean PRIVATE
    user32
    gdi32
    shell32
    advapi32
    ole32
    comctl32
)

if(MSVC)
    set_property(TARGET Clipclean PROPERTY MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(Clipclean PRIVATE /utf-8 /W3)
    # The manifest is embedded by app.rc; stop the linker adding a second one.
    target_link_options(Clipclean PRIVATE /MANIFEST:NO)
else()
    target_compile_options(Clipclean PRIVATE -Wall -Wextra -finput-charset=UTF-8)
    target_link_options(Clipclean PRIVATE -static -municode -mwindows)
endif()

# Pure-logic unit tests for the clipboard text transformer.
add_executable(plain_text_tests
    tests/test_plain_text.cpp
    src/core/plain_text.cpp
)
target_include_directories(plain_text_tests PRIVATE src)
if(MSVC)
    target_compile_options(plain_text_tests PRIVATE /utf-8)
endif()

enable_testing()
add_test(NAME plain_text COMMAND plain_text_tests)
