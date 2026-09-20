# Clipclean

[English](README.md) | **中文**

一个极小的剪贴板工具：把复制内容的富文本剥离，粘贴出来永远是纯文本。
原生支持 **macOS** 与 **Windows**。

开启 **纯文本模式** 后，每次复制都会被还原为纯文本表示：字体、颜色等富文本
格式被丢弃，而 Unicode、emoji、制表符与换行全部保留。关闭时，剪贴板行为与
系统原本完全一致。

两个平台都只驻留在菜单栏 / 系统托盘：没有 Dock 图标、没有任务栏窗口、没有
安装程序。

## 平台

| | macOS | Windows |
| --- | --- | --- |
| 源码 | [`com.clipclean.paste/`](com.clipclean.paste) | [`clipclean-windows/`](clipclean-windows) |
| 要求 | macOS 14.0 及以上（Apple Silicon 或 Intel） | Windows 7 SP1 及以上（32 / 64 位） |
| 技术 | SwiftUI / AppKit（Xcode 26+） | Win32 / C++17（MSVC 或 MinGW，无 .NET） |
| 发行物 | `build/Clipclean-<版本>.dmg` | `clipclean-windows/dist/Clipclean.exe` |
| 背景 | macOS 26+ 液态玻璃，14–15 半透明材质 | 纯色面板，Win10/11 跟随系统深浅色 |

## 功能

- 菜单栏（macOS）/ 托盘（Windows）图标，右键菜单：模式 ▸ 开 / 关、设置、退出
- 左键打开带大滑块的小面板
- 全局热键：`F6` 开启纯文本模式，`F5` 关闭
- 开机自启（macOS）/ 随 Windows 启动（Windows）
- 设置内可切换 English / 中文
- Windows 10 / 11 上面板跟随系统深浅色；Windows 7 / 8 始终提供手动白 / 黑开关
- 无网络访问，无剪贴板历史

## 截图

macOS：

| 主面板 | 设置 | 右键菜单 |
| --- | --- | --- |
| ![主面板](com.clipclean.paste/Screenshots/01-main.png) | ![设置](com.clipclean.paste/Screenshots/02-settings.png) | ![右键菜单](com.clipclean.paste/Screenshots/03-menu.png) |

## 行为

**纯文本模式：开**

- 监听剪贴板变化 —— macOS 用 run-loop 定时轮询 `changeCount`，Windows 用
  `AddClipboardFormatListener`。
- 依次取可用的文本：纯文本 → RTF → HTML。
- 只把该文本写回剪贴板，因此「只有 RTF / HTML」的内容同样会被还原。
- Unicode、emoji、制表符与换行均保留，含代理对（astral 平面字符）。
- 忽略自身写入造成的变化，因此绝不会循环。
- 纯图片剪贴板、文件（URL）复制保持原样。
- 已是纯文本的剪贴板不会被重写。

**纯文本模式：关**

- 不读取、不写入剪贴板。

## 快捷键

| 按键 | 动作 |
| --- | --- |
| `F6` | 开启纯文本模式 |
| `F5` | 关闭纯文本模式 |
| `Esc` | 退出设置 |

两个平台的全局热键都不需要辅助功能或输入监控权限。

> **macOS 笔记本：** 顶排默认是媒体键，单独按 F5/F6 可能到不了应用。可按住
> `Fn`（`Fn+F6` / `Fn+F5`），或在 *系统设置 → 键盘 → 键盘快捷键… → 功能键*
> 里勾选「将 F1、F2 等键用作标准功能键」。

## 安装

- **macOS** —— 下载 `Clipclean-<版本>.dmg`，打开后把 **Clipclean** 拖进
  **应用程序**。
- **Windows** —— 直接运行 `clipclean-windows/dist/Clipclean.exe`。无需安装，
  放在任意位置即可，可在设置里勾选「随 Windows 启动」。

Releases 里的 macOS 构建是本地签名、未公证的，首次打开系统可能提示。右键
应用选 **打开**，或在 *系统设置 → 隐私与安全性* 里放行。

## 构建

### macOS

```sh
open com.clipclean.paste.xcodeproj   # 然后 ⌘R
./scripts/package-dmg.sh             # build/Clipclean-<版本>.dmg
```

签名与公证见 [`com.clipclean.paste/README.zh-CN.md`](com.clipclean.paste/README.zh-CN.md)。

### Windows

```bat
cmake -S clipclean-windows -B clipclean-windows/build -G "Visual Studio 18 2026"
cmake --build clipclean-windows/build --config Release
```

MinGW 与测试见 [`clipclean-windows/README.md`](clipclean-windows/README.md)。

## 目录结构

```
com.clipclean.paste/      macOS 版 —— Swift 源码与 Xcode 工程
  Screenshots/            README 截图
  scripts/package-dmg.sh
clipclean-windows/        Windows 版 —— Win32 / C++17 源码与 CMake 工程
  dist/                   预编译的 Clipclean.exe
  res/                    图标、manifest、版本信息
  tests/                  纯逻辑单元测试
```

## 有意不做的

没有剪贴板历史、搜索、云同步、AI、OCR，也没有多余设置。

## 许可证 / License

Apache-2.0. 详见 `LICENSE` / See `LICENSE`.
