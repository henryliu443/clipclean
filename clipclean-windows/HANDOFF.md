# Windows 版构建交接说明

给在 Windows 上打开本项目的自己/协作者。目标是 **Win7 SP1 及以上** 的原生
Win32 (C++17) 版 Clipclean，无 .NET 依赖，单个小 exe。

## 现状

- 源码已全部写好，目录结构见 `README.md`。
- **纯逻辑层已在本机用 clang 编译并通过测试**（`core/plain_text`、`core/strings`）。
- **Win32 部分已在 VS 2026 (MSVC 19.51) 下编译通过**：`Clipclean.exe` 能启动并常驻
  托盘，`plain_text_tests` 全绿。过程中修了 3 处，见「已修复的 MSVC 问题」。
- 图标已生成（`res/*.ico`，含 16/32/64/128/256 多尺寸），资源与 manifest 已就位。

## 已修复的 MSVC 问题

从「没编译过」到「编译 + 启动 + HiDPI + 文本处理/开关核心验证全绿」一共 8 处：

1. `src/ui/panel_window.cpp` — 用了 `systemUsesDarkMode()` 却没包含声明它的头文件。
   已加 `#include "win32/win_util.h"`。
2. `src/ui/theme.cpp` — `for (HFONT* f : { ... })` 这种大括号初始化列表在 MSVC 下
   推导不出 `std::initializer_list`（C3312）。已改成先声明显式数组再遍历。
3. `src/app.cpp` — **启动即退出（exit 1）** 的 bug：`messageWindowProc` 在
   `WM_NCCREATE` 时没有给 `messageWindow_` 赋值，而 `handleMessage` 的 default
   分支会调用 `DefWindowProcW(messageWindow_, ...)`；窗口创建期间该成员还是
   `nullptr`，导致 `CreateWindowExW` 失败（`GetLastError` = 1400，
   `ERROR_INVALID_WINDOW_HANDLE`）。已按 `panel_window.cpp` 里既有的写法，在
   `WM_NCCREATE` 里补上 `self->messageWindow_ = hwnd;`。
4. **HiDPI**：进程设了 `PER_MONITOR_AWARE_V2`，但界面所有尺寸/字体都是写死的
   **物理像素**，200% 屏上面板只有一半大、字也只有一半（240×300 物理 = 120×150
   逻辑）。现在统一按 DPI 缩放：`win_util` 新增 `systemDpi()` / `windowDpi()`，
   `Theme` 增加 `dpi` 与 `px(v)`（`MulDiv(v, dpi, 96)`），`PanelView` 的布局、
   图标、开关、复选框、字体以及 `PanelWindow` 的窗口尺寸/圆角全部走 `px()`；
   并处理 `WM_DPICHANGED`（换显示器时重建字体并跟随系统建议尺寸）。
   验证：200% 下面板 480×600 物理（=240×300 逻辑），设置页 600×800。
5. **构建系统（重要，容易踩）**：中文 locale 下 `cl.exe` 输出 `注意: 包含文件:`，
   CMake 认不出这个前缀 → **头文件依赖根本没被记录**。改了某个 `.h` 后只重编显式
   改过的 `.cpp`，其余 include 该头的目标不重编 → 结构体布局不一致、内存错乱
   （症状：窗口尺寸变成 0x7FFF）。**修法：设置 `VSLANG=1033` 让编译器输出英文**，
   然后清空重建：
   ```bat
   set VSLANG=1033
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   ```
   改过头文件后若拿不准，直接删掉 build 目录重建最省事。
6. **RTF `\uN` 高区/代理对丢字符**（`src/core/plain_text.cpp`）：原来
   `appendCodePoint(static_cast<unsigned>(static_cast<short>(param)))`，当码点 >
   0x7FFF（如 `！`=U+FF01）或 RTF 的负值形式（`\u-255`、emoji 代理对的负数）时会
   变成巨大无符号数被丢弃。已改为取低 16 位
   `static_cast<unsigned short>(param)`，正/负/代理对三种形式都能解码。
7. **只有 RTF/HTML（无纯文本）时不改写**（`src/core/plain_text.cpp`）：
   `requiresPlainTextRewrite` 原来 `if (!hasPlainText) return false;`，导致
   clipboard 里只有 RTF 或 HTML 时**静默跳过**。现在无纯文本时，仅对 RTF/HTML
   这类可无损转文本的格式返回 true；图片(CF_DIB/CF_BITMAP)、文件(CF_HDROP) 仍跳过。
8. **`stripHtml` 标签匹配过宽**（`src/core/plain_text.cpp`）：原来用
   `tag.rfind("/h",0)==0` 等前缀判断，把 `</html>` 当成标题、`</pre>` 当成段落、
   `</link>` 当成列表项，多输出换行。改为按完整标签名匹配（`isNamedTag`）。
9. **语言分段控件中间的"一条线"**（`src/ui/panel_view.cpp`）：选中半块与未选中半块
   直接相接，直边看起来像一条分割线。改成 iOS 风格**内缩圆角胶囊**（选中侧缩进
   `px(2)` 浮在轨道上），中间不再有直线接缝。

## 核心验证（实测）

- 文本处理集成（原生 Win32 剪贴板 API，非 .NET）：**11/11 PASS**
  （关模式保留 RTF；开模式 RTF/HTML 转纯文本含 only 情况；CJK+emoji 经 RTF `\u`
  正/负/代理对与 HTML UTF-8 均不丢；图片/文件不动；纯文本不重写；无自循环）。
- 开关控制集成（`test-switch.ps1`）：**7/7 PASS**
  （默认关、F6 开、F5 关、滑块点击、重启持久化、托盘左键开关面板、滑块像素位置与
  实际行为一致）。
- `plain_text_tests` 单元测试全绿（含 `\uN` 正负/代理对、HTML 实体/astral、
  `</html>`/`</pre>`/`</link>` 等边界）。

## 面板行为约定

- **位置固定**：面板停在工作区右下角 / 托盘图标附近，**不跟随光标**（之前用
  `GetCursorPos()` 当锚点，会弹在指针下面）。不提供拖动。
- **不自动弹出**：启动时不显示面板，只有以下操作会打开——
  左键点托盘图标、右键菜单里的「设置…」、或收到 `kShowPanelMessage`（`WM_APP+2`）。
- **首次启动**：只在托盘弹一次气泡（`TrayIcon::showBalloon`，`Strings::firstRunTitle/
  firstRunMessage`），并通过创建注册表键把「已问候」落盘，之后不再弹。
- **默认不接管剪贴板**：`plainTextModeEnabled` 默认 `false`，监控器不启动，
  不动任何复制内容；要用纯文本模式得手动开（面板滑块 / F6 / 右键菜单）。

## 已验证

- Debug 与 Release 都构建成功；Release `Clipclean.exe` 约 279 KB。
- `plain_text_tests` 全部通过（`plain_text: all checks passed`）。
- exe 启动后常驻托盘不再崩溃，托盘 / 面板 / 全局热键均已注册。

## 环境

- Visual Studio 2026（本机装的是 18.10.1）
- 工作负载：**使用 C++ 的桌面开发**（`.NET 桌面开发` 不需要）
- 需要 CMake（VS 自带 CMake 组件即可）

> 注意：VS 自带的 cmake / ninja / MSVC **不在系统 PATH 上**。要么用「开发者命令提示符」，
> 要么直接调 VS 目录里的：
> - cmake：`…\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`
> - ninja：`…\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe`
> - 编译前先跑 `…\VC\Auxiliary\Build\vcvars64.bat` 注入 `cl.exe` 的环境变量。

## 构建方式 A：VS 2026 直接打开文件夹（推荐）

1. VS → **文件 → 打开 → 文件夹**，选择本 `windows/` 目录。
2. VS 会自动识别 `CMakeLists.txt` 并配置。
3. 在“选择启动项”里选 **Clipclean**，点生成/运行（或 Ctrl+F5）。

## 构建方式 B：命令行

```bat
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

若生成器名字不对，用 `cmake --help` 看列表；也可以换 Ninja：

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

产物：`build\Release\Clipclean.exe`（或 `build\Clipclean.exe`）。
运行后出现在**系统托盘**，左键开面板、右键出菜单。

## 测试

```bat
cmake --build build --config Release --target plain_text_tests
ctest --test-dir build -C Release
```

## 编译时重点关注（可能报错的地方）

按可能性排序，报错时优先看这些：

1. `src/win32/clipboard.cpp` — 用了 `strnlen`。MSVC 一般有；若报未声明，
   改成 `strnlen_s` 或手写循环。
2. `src/win32/win_util.cpp` — 用了 `RTL_OSVERSIONINFOW` / `PRTL_OSVERSIONINFOW`。
   若头文件没声明，改成手动定义结构体。
3. `src/win32/tray_icon.cpp` — `NOTIFYICONDATA_V2_SIZE` 已用 `#ifdef` 保护，
   正常不会出问题。
4. `res/app.rc` + `1 24 "app.manifest"` — MSVC 端已在 CMake 里加
   `/MANIFEST:NO` 防止 manifest 冲突；若仍报 manifest 重复，检查这条。
5. 中文字符串 — MSVC 已加 `/utf-8`，MinGW 加了 `-finput-charset=UTF-8`。
6. MinGW 链接入口 — 已加 `-municode -mwindows`。

## 反馈时请附上

- 完整编译报错（含文件名:行号）
- 用的哪种方式（VS 打开文件夹 / 命令行 / 生成器名）
- 如果 exe 能跑起来：托盘图标、面板、F5/F6、设置里的外观开关是否正常

## 功能清单（对照验收）

- [ ] 托盘图标 + 右键菜单：模式 ▸ 开 / 关、设置…、退出
- [ ] 左键点托盘开/关面板
- [ ] 面板大滑块切换纯文本模式
- [ ] `F6` 开、`F5` 关（全局热键）
- [ ] 纯文本模式：复制富文本后，剪贴板只剩纯文本（含 RTF/HTML 兜底解析）
- [ ] 图片/文件复制不被改写
- [ ] 设置：开机自启、外观（深色模式 + 跟随系统）、语言中英切换
- [ ] 外观：Win10/11 有“跟随系统”勾选；Win7/8 只有手动白/黑
- [ ] 面板随系统深浅色实时变化（Win10/11）
