<template>
  <el-config-provider>
    <div class="container">
      <div class="main-panel">
        <div class="header">
          <div class="brand">
            <span class="brand-title">极速录屏</span>
            <span class="status-dot" :class="{ 'is-recording': isRecording }"></span>
          </div>
          <el-tag :type="isRecording ? 'danger' : 'success'" size="small" effect="plain">
            {{ isRecording ? '录制中' : '就绪' }}
          </el-tag>
        </div>
        
        <div class="stats">
          <div class="timer">{{ timerDisplay }}</div>
        </div>

        <div class="config-section">
          <el-radio-group v-model="format" :disabled="isRecording" size="small" class="format-group">
            <el-radio-button label="MP4">MP4 (H.264)</el-radio-button>
            <el-radio-button label="GIF">GIF 动画</el-radio-button>
          </el-radio-group>

          <div class="path-selector">
            <el-input 
              v-model="saveDir" 
              placeholder="默认保存路径" 
              readonly 
              size="small"
              :disabled="isRecording"
            >
              <template #append>
                <el-button @click="selectDir" :disabled="isRecording">浏览</el-button>
              </template>
            </el-input>
          </div>
        </div>

        <div class="actions">
          <el-button 
            :type="isRecording ? 'danger' : 'primary'" 
            class="record-btn"
            :icon="isRecording ? 'VideoPause' : 'VideoPlay'"
            @click="toggleRecording"
          >
            {{ isRecording ? '停止录制' : '开始录制' }}
          </el-button>
        </div>
      </div>
    </div>
  </el-config-provider>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue';
import { invoke } from '@tauri-apps/api/core';
import { open } from '@tauri-apps/plugin-dialog';
import { videoDir } from '@tauri-apps/api/path';
import { ElMessage } from 'element-plus';

const isRecording = ref(false);
const format = ref('MP4');
const elapsedSeconds = ref(0);
const saveDir = ref('');
let intervalId: any = null;

onMounted(async () => {
  try {
    saveDir.value = await videoDir();
    console.log('Video Directory resolved:', saveDir.value);
  } catch (e) {
    ElMessage.error(`获取路径失败: ${e}`);
    return;
  }

  try {
    // 增加启动延迟，避开初始化竞态
    await new Promise(r => setTimeout(r, 500));
    
    // IPC 心跳测试
    await invoke('plugin:recorder|ping');
  } catch (e) {
    console.error('IPC 连接失败', e);
    // 尝试输出更详细的错误原因
    const errorMsg = typeof e === 'object' ? JSON.stringify(e) : e;
    ElMessage.error(`IPC 握手失败: ${errorMsg}`);
  }
});

const timerDisplay = computed(() => {
  const m = Math.floor(elapsedSeconds.value / 60);
  const s = elapsedSeconds.value % 60;
  return `${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
});

async function selectDir() {
  try {
    const selected = await open({
      directory: true,
      multiple: false,
      title: '选择保存目录'
    });
    if (selected) {
      saveDir.value = selected as string;
    }
  } catch (err) {
    console.error(err);
  }
}

async function toggleRecording() {
  if (!isRecording.value) {
    try {
      console.log('Attempting to start recording with saveDir:', saveDir.value);
      const filename = await invoke('plugin:recorder|start_recording', { 
        format: format.value === 'GIF' ? 1 : 0,
        saveDir: saveDir.value || null
      });
      isRecording.value = true;
      elapsedSeconds.value = 0;
      intervalId = setInterval(async () => {
        elapsedSeconds.value = await invoke('plugin:recorder|get_elapsed');
      }, 1000);
    } catch (err) {
      console.error('Start recording failed:', err);
      ElMessage.error(`录制启动失败: ${err}`);
    }
  } else {
    try {
      await invoke('plugin:recorder|stop_recording');
      isRecording.value = false;
      if (intervalId) {
        clearInterval(intervalId);
        intervalId = null;
      }
    } catch (err) {
      console.error('Stop recording failed:', err);
      ElMessage.error(`停止失败: ${err}`);
    }
  }
}

onUnmounted(() => {
  if (intervalId) clearInterval(intervalId);
});
</script>

<style scoped>
:global(body) {
  margin: 0;
  padding: 0;
  overflow: hidden;
  user-select: none;
  font-family: "Helvetica Neue", Helvetica, "PingFang SC", "Hiragino Sans GB", "Microsoft YaHei", "\5FAE\8F6F\9605\9ED1", Arial, sans-serif;
}

.container {
  width: 100vw;
  height: 100vh;
  display: flex;
  background-color: #f8fafc;
}

.main-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  padding: 20px;
  background: white;
}

.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.brand {
  display: flex;
  align-items: center;
  gap: 8px;
}

.brand-title {
  font-weight: 700;
  font-size: 16px;
  color: #1e293b;
}

.status-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background-color: #67c23a;
}

.status-dot.is-recording {
  background-color: #f56c6c;
  box-shadow: 0 0 8px #f56c6c;
  animation: pulse 1.5s infinite;
}

@keyframes pulse {
  0% { opacity: 1; }
  50% { opacity: 0.4; }
  100% { opacity: 1; }
}

.stats {
  display: flex;
  justify-content: center;
  align-items: center;
  background: #f1f5f9;
  border-radius: 12px;
  padding: 30px 0;
  margin-bottom: 24px;
}

.timer {
  font-size: 64px;
  font-family: 'JetBrains Mono', 'Courier New', monospace;
  font-weight: 700;
  color: #0f172a;
  letter-spacing: -2px;
}

.config-section {
  display: flex;
  flex-direction: column;
  gap: 16px;
  margin-bottom: 24px;
}

.format-group {
  width: 100%;
  display: flex;
}

.format-group :deep(.el-radio-button) {
  flex: 1;
}

.format-group :deep(.el-radio-button__inner) {
  width: 100%;
}

.path-selector :deep(.el-input-group__append) {
  background-color: #f1f5f9;
  color: #475569;
  border: 1px solid #e2e8f0;
  border-left: none;
}

.actions {
  margin-top: auto;
}

.record-btn {
  width: 100%;
  height: 50px;
  font-size: 18px;
  font-weight: 600;
  border-radius: 12px;
  letter-spacing: 1px;
}

:deep(.el-radio-button__inner) {
  background: #f8fafc;
}
</style>
