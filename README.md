# 极速录屏 (Rapid Recorder) v0.0.1

基于 Tauri v2 + Vue 3 + C++ 开发的高性能 Windows 录屏工具。

## 核心特性

- **高性能录制**：采用 Windows Media Foundation (MF) 框架，支持 H.264 硬件加速编码。
- **高画质 GIF**：内置 WIC (Windows Imaging Component) 编码引擎，自动执行平滑降采样（1280px 宽度）。
- **零延迟捕获**：使用 DXGI Desktop Duplication API 实现超低延迟的屏幕像素获取。
- **现代 UI**：基于 Vue 3 + Element Plus 构建的毛玻璃质感侧边栏界面。
- **混合架构**：Tauri v2 插件化设计，业务逻辑与 C++ 采集引擎物理级隔离。

## 技术栈

- **Frontend**: Vue 3, Vite, Element Plus, TypeScript
- **Backend**: Rust (Tauri v2), C++ (MSVC)
- **APIs**: DXGI, Media Foundation (MF), WIC, Win32
- **IPC**: Tauri v2 ACL 权限受控通信

## 开始使用

### 开发环境配置
- Windows 10/11
- Node.js (推荐 18+)
- Rust (推荐 1.75+)
- Visual Studio (包含 C++ 桌面开发组件)

### 安装依赖
```powershell
pnpm install
```

### 启动开发模式
```powershell
pnpm tauri dev
```

### 构建正式版本
```powershell
pnpm tauri build
```

## 开发者说明

项目采用了 **`tauri-plugin-recorder`** 插件化设计，C++ 源码位于 `src-tauri/recorder/core` 下。

### 目录结构
- `src-tauri/recorder`: 录屏核心插件 (FFI 桥接层)
- `src-tauri/recorder/core`: C++ 采集与编码引擎 (MediaFoundation 实现)
- `src`: Vue 3 组件化 UI

## 版本日志

### v0.0.1 (Current)
- [x] 完成 Tauri v1 -> v2 架构迁移。
- [x] 实现 MP4 (H.264) 硬件加速录制。
- [x] 实现 GIF 自动降采样录制。
- [x] 修复 DXR 模式下的像素跨度与取向问题。
- [x] 完成 ACL 指令级权限授权。

## 许可证
MIT
