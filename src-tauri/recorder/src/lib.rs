use tauri::{plugin::{Builder, TauriPlugin}, Runtime};
use std::io::Write;

// 导入 C++ 桥接函数
extern "C" {
    fn recorder_start(filename: *const u16, format: i32) -> i32;
    fn recorder_stop();
    fn recorder_is_recording() -> bool;
    fn recorder_get_elapsed() -> i32;
}

#[tauri::command]
async fn ping() {
    println!("[Rust-Plugin] Pong! Recorder plugin is active.");
    let _ = std::io::stdout().flush();
}

#[tauri::command]
async fn start_recording(format: i32, save_dir: Option<String>) -> Result<String, String> {
    println!("[Rust-Plugin] Received start_recording. format: {}, save_dir: {:?}", format, save_dir);
    let _ = std::io::stdout().flush();
    
    // Default to Video directory if not provided
    let path = if let Some(dir) = save_dir {
        std::path::PathBuf::from(dir)
    } else {
        std::path::PathBuf::from("C:\\Users\\Public\\Videos")
    };

    if !path.exists() {
        std::fs::create_dir_all(&path).map_err(|e| e.to_string())?;
    }

    let timestamp = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_secs();
    
    let ext = if format == 1 { "gif" } else { "mp4" };
    let filename = path.join(format!("recording_{}.{}", timestamp, ext));
    let filename_str = filename.to_str().ok_or("Invalid path")?;
    
    let wide_filename: Vec<u16> = filename_str.encode_utf16().chain(std::iter::once(0)).collect();
    
    unsafe {
        let hr = recorder_start(wide_filename.as_ptr(), format);
        if hr < 0 { return Err(format!("无法启动录制: 0x{:08X}", hr as u32)); }
    }
    
    Ok(filename_str.to_string())
}

#[tauri::command]
fn stop_recording() {
    println!("[Rust-Plugin] Received stop_recording.");
    let _ = std::io::stdout().flush();
    unsafe { recorder_stop(); }
}

#[tauri::command]
fn get_elapsed() -> i32 {
    unsafe { recorder_get_elapsed() }
}

#[tauri::command]
fn is_recording() -> bool {
    unsafe { recorder_is_recording() }
}

pub fn init<R: Runtime>() -> TauriPlugin<R> {
    Builder::new("recorder")
        .invoke_handler(tauri::generate_handler![
            ping,
            start_recording,
            stop_recording,
            get_elapsed,
            is_recording
        ])
        .build()
}
