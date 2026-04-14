const COMMANDS: &[&str] = &[
    "ping",
    "start_recording",
    "stop_recording",
    "get_elapsed",
    "is_recording",
];

fn main() {
    // 1. 编译 C++ 录屏核心逻辑
    cc::Build::new()
        .cpp(true)
        .file("core/Bridge.cpp")
        .file("core/CaptureEngine.cpp")
        .file("core/GifEncoder.cpp")
        .file("core/Recorder.cpp")
        .file("core/VideoEncoder.cpp")
        .include("core")
        .compile("recorder_core");

    // Ensure re-compilation when C++ source changes
    println!("cargo:rerun-if-changed=core");

    // 2. 链接 Windows 构建必要的系统库
    println!("cargo:rustc-link-lib=mf");
    println!("cargo:rustc-link-lib=mfplat");
    println!("cargo:rustc-link-lib=mfuuid");
    println!("cargo:rustc-link-lib=mfreadwrite");
    println!("cargo:rustc-link-lib=d3d11");
    println!("cargo:rustc-link-lib=dxgi");
    println!("cargo:rustc-link-lib=shlwapi");
    println!("cargo:rustc-link-lib=user32");
    println!("cargo:rustc-link-lib=ole32");

    // 3. 运行 Tauri 插件构建器
    tauri_plugin::Builder::new(COMMANDS).build();
}
