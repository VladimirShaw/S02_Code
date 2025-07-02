/**
 * =============================================================================
 * GameFlowManager - C102音频控制器游戏流程管理器 - 实现文件
 * 版本: 2.0 - C102专用版本
 * 创建日期: 2025-01-03
 * =============================================================================
 */

#include "GameFlowManager.h"
#include "UniversalHarbingerClient.h"
#include "BY_VoiceController_Unified.h"

// 外部全局实例
extern UniversalHarbingerClient harbingerClient;
extern BY_VoiceController_Unified voice;

// 全局实例
GameFlowManager gameFlowManager;

// ========================== 构造和初始化 ==========================
GameFlowManager::GameFlowManager() {
    currentStageId = "";
    stageStartTime = 0;
    stageRunning = false;
    jumpRequested = false;
    globalStopped = false;
}

void GameFlowManager::begin() {
    Serial.println(F("C102 GameFlowManager初始化完成"));
}

// ========================== 环节控制 ==========================
bool GameFlowManager::startStage(const String& stageId) {
    // 标准化环节ID
    String normalizedId = normalizeStageId(stageId);
    
    Serial.print(F("=== 启动C102音频环节: "));
    Serial.print(stageId);
    if (normalizedId != stageId) {
        Serial.print(F(" (标准化为: "));
        Serial.print(normalizedId);
        Serial.print(F(")"));
    }
    Serial.println(F(" ==="));
    
    // 重置状态
    globalStopped = false;
    jumpRequested = false;
    
    // 更新环节管理信息
    currentStageId = normalizedId;
    stageStartTime = millis();
    stageRunning = true;
    
    // 根据环节ID执行对应逻辑
    if (normalizedId == "000_0") {
        Serial.print(F("🎵 环节000_0：通道"));
        Serial.print(STAGE_000_0_CHANNEL);
        Serial.print(F("循环播放"));
        Serial.print(STAGE_000_0_SONG_ID);
        Serial.print(F("号音频("));
        Serial.print(STAGE_000_0_START);
        Serial.println(F("ms启动)"));
        
        // 音频播放现在由updateStep000()方法根据START时间控制
        Serial.println(F("⏳ 等待通道到达启动时间..."));
        return true;
    } else if (normalizedId == "001_2") {
        Serial.print(F("🎵 环节001_2：通道"));
        Serial.print(STAGE_001_2_CHANNEL);
        Serial.print(F("播放"));
        Serial.print(STAGE_001_2_SONG_ID);
        Serial.print(F("("));
        Serial.print(STAGE_001_2_START);
        Serial.print(F("ms启动)，通道"));
        Serial.print(STAGE_001_2_FADE_CHANNEL);
        Serial.print(F("音量从"));
        Serial.print(STAGE_001_2_FADE_START_VOL);
        Serial.print(F("淡出到"));
        Serial.print(STAGE_001_2_FADE_END_VOL);
        Serial.print(F("("));
        Serial.print(STAGE_001_2_FADE_DURATION);
        Serial.println(F("ms)"));
        
        // 设置第2路初始音量为30
        voice.setVolume(STAGE_001_2_FADE_CHANNEL, STAGE_001_2_FADE_START_VOL);
        
        // 音频播放现在由updateStep001_2()方法根据START时间控制
        Serial.println(F("⏳ 等待通道到达启动时间..."));
        return true;
    } else if (normalizedId == "002_0") {
        Serial.print(F("🎵 环节002_0：通道"));
        Serial.print(STAGE_002_0_CHANNEL1);
        Serial.print(F("播放"));
        Serial.print(STAGE_002_0_SONG_ID1);
        Serial.print(F("("));
        Serial.print(STAGE_002_0_CHANNEL1_START);
        Serial.print(F("ms)，通道"));
        Serial.print(STAGE_002_0_CHANNEL2);
        Serial.print(F("播放"));
        Serial.print(STAGE_002_0_SONG_ID2);
        Serial.print(F("("));
        Serial.print(STAGE_002_0_CHANNEL2_START);
        Serial.println(F("ms)"));
        
        // 重置第2路音量（解决001_2环节淡出后的问题）
        voice.setVolume(STAGE_002_0_CHANNEL2, 20);  // 恢复正常音量
        Serial.print(F("🔊 重置通道"));
        Serial.print(STAGE_002_0_CHANNEL2);
        Serial.println(F("音量为20"));
        
        // 音频播放现在由updateStep002()方法根据START时间控制
        Serial.println(F("⏳ 等待各通道到达启动时间..."));
        
        return true;
    } else {
        Serial.print(F("❌ 未定义的C102环节: "));
        Serial.println(normalizedId);
        stageRunning = false;
        currentStageId = "";
        return false;
    }
}

void GameFlowManager::stopCurrentStage() {
    if (stageRunning) {
        Serial.print(F("⏹️ 结束当前环节: "));
        Serial.println(currentStageId);
        
        stageRunning = false;
        currentStageId = "";
        stageStartTime = 0;
        jumpRequested = false;
    }
}

void GameFlowManager::stopAllStages() {
    Serial.println(F("🛑 强制停止所有C102音频环节"));
    
    // 设置全局停止标志
    globalStopped = true;
    
    // 停止所有音频通道
    for (int channel = 1; channel <= 4; channel++) {
        voice.stop(channel);
        delay(50);  // 给每个停止命令一些时间执行
    }
    
    // 二次确认停止
    delay(200);
    for (int channel = 1; channel <= 4; channel++) {
        voice.stop(channel);
    }
    
    // 重置环节状态
    stageRunning = false;
    currentStageId = "";
    stageStartTime = 0;
    jumpRequested = false;
    
    Serial.println(F("✅ 所有C102音频效果已停止"));
}

// ========================== 状态查询 ==========================
const String& GameFlowManager::getCurrentStageId() const {
    return currentStageId;
}

bool GameFlowManager::isStageRunning() const {
    return stageRunning;
}

unsigned long GameFlowManager::getStageElapsedTime() const {
    if (stageRunning) {
        return millis() - stageStartTime;
    }
    return 0;
}

// ========================== 环节列表 ==========================
bool GameFlowManager::isValidStageId(const String& stageId) {
    String normalizedId = normalizeStageId(stageId);
    return (normalizedId == "000_0" || 
            normalizedId == "001_2" || 
            normalizedId == "002_0");
}

void GameFlowManager::printAvailableStages() {
    Serial.println(F("=== C102可用音频环节列表 ==="));
    
    Serial.print(F("000_0 - 通道"));
    Serial.print(STAGE_000_0_CHANNEL);
    Serial.print(F("循环播放"));
    Serial.print(STAGE_000_0_SONG_ID);
    Serial.print(F("号音频("));
    Serial.print(STAGE_000_0_COMPLETE_TIME);
    Serial.println(F("ms后完成)"));
    
    Serial.print(F("001_2 - 通道"));
    Serial.print(STAGE_001_2_CHANNEL);
    Serial.print(F("播放"));
    Serial.print(STAGE_001_2_SONG_ID);
    Serial.print(F("，通道"));
    Serial.print(STAGE_001_2_FADE_CHANNEL);
    Serial.print(F("音量"));
    Serial.print(STAGE_001_2_FADE_START_VOL);
    Serial.print(F("→"));
    Serial.print(STAGE_001_2_FADE_END_VOL);
    Serial.print(F("("));
    Serial.print(STAGE_001_2_FADE_DURATION);
    Serial.print(F("ms)，"));
    Serial.print(STAGE_001_2_DURATION/1000);
    Serial.println(F("秒后完成)"));
    
    Serial.print(F("002_0 - 通道"));
    Serial.print(STAGE_002_0_CHANNEL1);
    Serial.print(F("播放"));
    Serial.print(STAGE_002_0_SONG_ID1);
    Serial.print(F("，通道"));
    Serial.print(STAGE_002_0_CHANNEL2);
    Serial.print(F("播放"));
    Serial.print(STAGE_002_0_SONG_ID2);
    Serial.print(F("("));
    Serial.print(STAGE_002_0_DURATION/1000);
    Serial.println(F("秒后完成)"));
    
    Serial.println(F("=============================="));
}

// ========================== 更新和调试功能 ==========================
void GameFlowManager::update() {
    if (!stageRunning) {
        return;
    }
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 根据当前环节更新状态
    if (currentStageId == "000_0") {
        updateStep000();
    } else if (currentStageId == "001_2") {
        updateStep001_2();
    } else if (currentStageId == "002_0") {
        updateStep002();
    }
}

void GameFlowManager::printStatus() {
    Serial.println(F("=== C102 GameFlowManager状态 ==="));
    Serial.print(F("当前环节: "));
    Serial.println(stageRunning ? currentStageId : "无");
    Serial.print(F("运行时间: "));
    Serial.print(getStageElapsedTime());
    Serial.println(F("ms"));
    Serial.print(F("跳转请求: "));
    Serial.println(jumpRequested ? "是" : "否");
    Serial.print(F("全局停止: "));
    Serial.println(globalStopped ? "是" : "否");
    Serial.println(F("================================"));
}

// ========================== 环节跳转请求 ==========================
void GameFlowManager::requestStageJump(const String& nextStage) {
    if (jumpRequested) {
        Serial.println(F("⚠️ 跳转请求已发送，避免重复"));
        return;
    }
    
    jumpRequested = true;
    Serial.print(F("📤 请求跳转到环节: "));
    Serial.println(nextStage);
    
    // 发送STEP_COMPLETE消息
    String message = "$[GAME]@C102{^STEP_COMPLETE^(current_step=\"" + currentStageId + 
                    "\",next_step=\"" + nextStage + 
                    "\",duration=" + String(getStageElapsedTime()) + 
                    ",error_count=0)}#";
    
    harbingerClient.sendMessage(message);
    Serial.print(F("📡 发送消息: "));
    Serial.println(message);
}

// ========================== 私有方法实现 ==========================

// 环节完成通知
void GameFlowManager::notifyStageComplete(const String& currentStep, const String& nextStep, unsigned long duration) {
    if (jumpRequested) {
        return;  // 避免重复通知
    }
    
    jumpRequested = true;
    String message = "$[GAME]@C102{^STEP_COMPLETE^(current_step=\"" + currentStep + 
                    "\",next_step=\"" + nextStep + 
                    "\",duration=" + String(duration) + 
                    ",error_count=0)}#";
    
    harbingerClient.sendMessage(message);
    Serial.print(F("📡 环节完成通知: "));
    Serial.println(message);
}

void GameFlowManager::notifyStageComplete(const String& currentStep, unsigned long duration) {
    if (jumpRequested) {
        return;  // 避免重复通知
    }
    
    jumpRequested = true;
    String message = "$[GAME]@C102{^STEP_COMPLETE^(current_step=\"" + currentStep + 
                    "\",duration=" + String(duration) + 
                    ",error_count=0)}#";
    
    harbingerClient.sendMessage(message);
    Serial.print(F("📡 环节完成通知: "));
    Serial.println(message);
}

// C102音频环节更新方法
void GameFlowManager::updateStep000() {
    unsigned long elapsed = getStageElapsedTime();
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 非阻塞延迟启动逻辑
    static bool channelStarted = false;
    
    // 检查通道是否到启动时间
    if (!channelStarted && elapsed >= STAGE_000_0_START) {
        voice.playSong(STAGE_000_0_CHANNEL, STAGE_000_0_SONG_ID);
        channelStarted = true;
        Serial.print(F("🎵 "));
        Serial.print(elapsed);
        Serial.print(F("ms: 通道"));
        Serial.print(STAGE_000_0_CHANNEL);
        Serial.print(F("开始播放"));
        Serial.println(STAGE_000_0_SONG_ID);
    }
    
    // 1000ms后跳转到下一环节
    if (!jumpRequested && elapsed >= STAGE_000_0_COMPLETE_TIME) {
        // 重置静态变量，为下次启动准备
        channelStarted = false;
        
        Serial.print(F("⏰ 环节000_0完成，跳转到"));
        Serial.println(STAGE_000_0_NEXT_STAGE);
        notifyStageComplete("000_0", STAGE_000_0_NEXT_STAGE, elapsed);
        // 继续音频循环，等待服务器下一步指令
    }
    
    // 持续检查音频状态，如果停止了就重新播放（只在启动后）
    if (channelStarted) {
        static unsigned long lastCheckTime = 0;
        
        if (elapsed - lastCheckTime >= STAGE_000_0_CHECK_INTERVAL) {
            // 检查音频状态，只有空闲时才重新播放
            if (!voice.isBusy(STAGE_000_0_CHANNEL)) {
                voice.playSong(STAGE_000_0_CHANNEL, STAGE_000_0_SONG_ID);
                Serial.print(F("🔄 通道"));
                Serial.print(STAGE_000_0_CHANNEL);
                Serial.print(F("音频播放完成，重新播放"));
                Serial.println(STAGE_000_0_SONG_ID);
            }
            lastCheckTime = elapsed;
        }
    }
}

void GameFlowManager::updateStep001_2() {
    unsigned long elapsed = getStageElapsedTime();
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 非阻塞延迟启动逻辑
    static bool channelStarted = false;
    
    // 检查通道是否到启动时间
    if (!channelStarted && elapsed >= STAGE_001_2_START) {
        voice.playSong(STAGE_001_2_CHANNEL, STAGE_001_2_SONG_ID);
        channelStarted = true;
        Serial.print(F("🎵 "));
        Serial.print(elapsed);
        Serial.print(F("ms: 通道"));
        Serial.print(STAGE_001_2_CHANNEL);
        Serial.print(F("开始播放"));
        Serial.println(STAGE_001_2_SONG_ID);
    }
    
    // 音量淡出控制：3秒内从30减到0，每100ms减1
    if (elapsed <= STAGE_001_2_FADE_DURATION) {
        static unsigned long lastVolumeUpdate = 0;
        static int currentVolume = STAGE_001_2_FADE_START_VOL;
        static bool volumeUpdateComplete = false;
        
        if (!volumeUpdateComplete && elapsed - lastVolumeUpdate >= STAGE_001_2_FADE_INTERVAL) {
            // 计算当前应该的音量：30 - (elapsed / 100)
            int targetVolume = STAGE_001_2_FADE_START_VOL - (elapsed / STAGE_001_2_FADE_INTERVAL);
            if (targetVolume < STAGE_001_2_FADE_END_VOL) {
                targetVolume = STAGE_001_2_FADE_END_VOL;
            }
            
            if (currentVolume != targetVolume) {
                currentVolume = targetVolume;
                voice.setVolume(STAGE_001_2_FADE_CHANNEL, currentVolume);
                lastVolumeUpdate = elapsed;
                
                Serial.print(F("🔊 通道"));
                Serial.print(STAGE_001_2_FADE_CHANNEL);
                Serial.print(F("音量调整为"));
                Serial.print(currentVolume);
                Serial.print(F("("));
                Serial.print(elapsed);
                Serial.println(F("ms)"));
            }
        }
    } else if (elapsed == STAGE_001_2_FADE_DURATION + 100) {
        // 3秒后停止第2路音频
        voice.stop(STAGE_001_2_FADE_CHANNEL);
        Serial.print(F("⏹️ 通道"));
        Serial.print(STAGE_001_2_FADE_CHANNEL);
        Serial.println(F("音频停止"));
    }
    
    // 83.347秒后跳转到下一环节
    if (!jumpRequested && elapsed >= STAGE_001_2_DURATION) {
        // 重置静态变量，为下次启动准备
        channelStarted = false;
        
        if (strlen(STAGE_001_2_NEXT_STAGE) > 0) {
            Serial.print(F("⏰ 环节001_2完成，跳转到"));
            Serial.println(STAGE_001_2_NEXT_STAGE);
            notifyStageComplete("001_2", STAGE_001_2_NEXT_STAGE, elapsed);
        } else {
            Serial.println(F("⏰ 环节001_2完成"));
            notifyStageComplete("001_2", elapsed);
        }
    }
}

void GameFlowManager::updateStep002() {
    unsigned long elapsed = getStageElapsedTime();
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 非阻塞延迟启动逻辑
    static bool channel1Started = false;
    static bool channel2Started = false;
    
    // 检查通道1是否到启动时间
    if (!channel1Started && elapsed >= STAGE_002_0_CHANNEL1_START) {
        voice.playSong(STAGE_002_0_CHANNEL1, STAGE_002_0_SONG_ID1);
        channel1Started = true;
        Serial.print(F("🎵 "));
        Serial.print(elapsed);
        Serial.print(F("ms: 通道"));
        Serial.print(STAGE_002_0_CHANNEL1);
        Serial.print(F("开始播放"));
        Serial.println(STAGE_002_0_SONG_ID1);
    }
    
    // 检查通道2是否到启动时间
    if (!channel2Started && elapsed >= STAGE_002_0_CHANNEL2_START) {
        voice.playSong(STAGE_002_0_CHANNEL2, STAGE_002_0_SONG_ID2);
        channel2Started = true;
        Serial.print(F("🎵 "));
        Serial.print(elapsed);
        Serial.print(F("ms: 通道"));
        Serial.print(STAGE_002_0_CHANNEL2);
        Serial.print(F("开始播放"));
        Serial.println(STAGE_002_0_SONG_ID2);
    }
    
    // 60秒后跳转到下一环节或报告完成
    if (!jumpRequested && elapsed >= STAGE_002_0_DURATION) {
        // 重置静态变量，为下次启动准备
        channel1Started = false;
        channel2Started = false;
        
        if (strlen(STAGE_002_0_NEXT_STAGE) > 0) {
            Serial.print(F("⏰ 环节002_0完成，跳转到"));
            Serial.println(STAGE_002_0_NEXT_STAGE);
            notifyStageComplete("002_0", STAGE_002_0_NEXT_STAGE, elapsed);
        } else {
            Serial.println(F("⏰ 环节002_0完成"));
            notifyStageComplete("002_0", elapsed);
        }
    }
}

// 工具方法
String GameFlowManager::normalizeStageId(const String& stageId) {
    String normalized = stageId;
    
    // 移除引号
    normalized.replace("\"", "");
    
    // C102音频格式保持下划线格式：000_0, 001_2, 002_0
    // 不需要转换，直接返回
    
    Serial.print(F("🔧 环节ID标准化: "));
    Serial.print(stageId);
    Serial.print(F(" -> "));
    Serial.println(normalized);
    
    return normalized;
}