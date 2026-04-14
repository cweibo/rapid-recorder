#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_recorder::init()) // 挂载解耦后的本地录屏插件
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
