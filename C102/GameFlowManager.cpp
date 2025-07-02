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
#include <string.h>  // for memset

// 外部全局实例
extern UniversalHarbingerClient harbingerClient;
extern BY_VoiceController_Unified voice;

// 全局实例
GameFlowManager gameFlowManager;

// ========================== 构造和初始化 ==========================
GameFlowManager::GameFlowManager() {
    // 初始化并行环节数组
    activeStageCount = 0;
    globalStopped = false;
    
    // 初始化兼容性变量
    currentStageId = "";
    stageStartTime = 0;
    stageRunning = false;
    jumpRequested = false;
    
    // 初始化所有环节槽位
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        stages[i] = StageState();
    }
}

void GameFlowManager::begin() {
    Serial.println(F("C102 GameFlowManager初始化完成（支持并行环节）"));
    Serial.print(F("最大并行环节数: "));
    Serial.println(MAX_PARALLEL_STAGES);
    
    // 初始化所有通道音量为默认值
    initializeAllVolumes();
}

// ========================== 私有辅助方法 ==========================
int GameFlowManager::findStageIndex(const String& stageId) {
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        if (stages[i].running && stages[i].stageId == stageId) {
            return i;
        }
    }
    return -1;
}

int GameFlowManager::findEmptySlot() {
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        if (!stages[i].running) {
            return i;
        }
    }
    return -1;
}

void GameFlowManager::updateCompatibilityVars() {
    // 更新兼容性变量，使其指向第一个活跃环节
    stageRunning = (activeStageCount > 0);
    
    if (activeStageCount > 0) {
        // 找到第一个运行中的环节
        for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
            if (stages[i].running) {
                currentStageId = stages[i].stageId;
                stageStartTime = stages[i].startTime;
                jumpRequested = stages[i].jumpRequested;
                break;
            }
        }
    } else {
        currentStageId = "";
        stageStartTime = 0;
        jumpRequested = false;
    }
}

// ========================== 环节控制 ==========================
bool GameFlowManager::startStage(const String& stageId) {
    // 标准化环节ID
    String normalizedId = normalizeStageId(stageId);
    
    // 检查环节是否已经在运行
    if (findStageIndex(normalizedId) >= 0) {
        Serial.print(F("⚠️ 环节已在运行: "));
        Serial.println(normalizedId);
        return false;
    }
    
    // 查找空闲槽位
    int slot = findEmptySlot();
    if (slot < 0) {
        Serial.print(F("❌ 无可用槽位，已达最大并行数: "));
        Serial.println(MAX_PARALLEL_STAGES);
        return false;
    }
    
    Serial.print(F("=== 启动C102音频环节[槽位"));
    Serial.print(slot);
    Serial.print(F("]: "));
    Serial.print(stageId);
    if (normalizedId != stageId) {
        Serial.print(F(" (标准化为: "));
        Serial.print(normalizedId);
        Serial.print(F(")"));
    }
    Serial.println(F(" ==="));
    
    // 重置全局停止标志
    globalStopped = false;
    
    // 初始化环节状态
    stages[slot].stageId = normalizedId;
    stages[slot].startTime = millis();
    stages[slot].running = true;
    stages[slot].jumpRequested = false;
    memset(&stages[slot].state, 0, sizeof(stages[slot].state));
    
    // 根据环节ID执行对应逻辑
    if (normalizedId == "000_0") {
        Serial.print(F("🎵 环节000_0：通道"));
        Serial.print(STAGE_000_0_CHANNEL);
        Serial.print(F("循环播放"));
        Serial.print(STAGE_000_0_SONG_ID);
        Serial.print(F("号音频("));
        Serial.print(STAGE_000_0_START);
        Serial.println(F("ms启动)"));
        
        // 初始化环节特定状态
        stages[slot].state.stage000.channelStarted = false;
        stages[slot].state.stage000.lastCheckTime = 0;
        
        Serial.println(F("⏳ 等待通道到达启动时间..."));
        activeStageCount++;
        updateCompatibilityVars();
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
        
        // 设置第2路初始音量
        voice.setVolume(STAGE_001_2_FADE_CHANNEL, STAGE_001_2_FADE_START_VOL);
        
        // 初始化环节特定状态
        stages[slot].state.stage001_2.channelStarted = false;
        stages[slot].state.stage001_2.lastVolumeUpdate = 0;
        stages[slot].state.stage001_2.currentVolume = STAGE_001_2_FADE_START_VOL;
        stages[slot].state.stage001_2.volumeUpdateComplete = false;
        
        Serial.println(F("⏳ 等待通道到达启动时间..."));
        activeStageCount++;
        updateCompatibilityVars();
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
        
        // 确保第2路音量为默认值（解决001_2环节淡出后的问题）
        resetChannelVolume(STAGE_002_0_CHANNEL2);
        Serial.print(F("🔊 确保通道"));
        Serial.print(STAGE_002_0_CHANNEL2);
        Serial.println(F("音量为默认值"));
        
        // 初始化环节特定状态
        stages[slot].state.stage002.channel1Started = false;
        stages[slot].state.stage002.channel2Started = false;
        stages[slot].state.stage002.multiJumpTriggered = false;
        
        Serial.println(F("⏳ 等待各通道到达启动时间..."));
        activeStageCount++;
        updateCompatibilityVars();
        return true;
    } else {
        Serial.print(F("❌ 未定义的C102环节: "));
        Serial.println(normalizedId);
        stages[slot].running = false;
        return false;
    }
}

bool GameFlowManager::startMultipleStages(const String& stageIds) {
    Serial.print(F("=== 启动多个并行环节: "));
    Serial.print(stageIds);
    Serial.println(F(" ==="));
    
    int successCount = 0;
    int startPos = 0;
    
    // 解析逗号分隔的环节ID列表
    while (startPos < stageIds.length()) {
        int commaPos = stageIds.indexOf(',', startPos);
        String stageId;
        
        if (commaPos < 0) {
            // 最后一个环节
            stageId = stageIds.substring(startPos);
        } else {
            // 中间的环节
            stageId = stageIds.substring(startPos, commaPos);
        }
        
        // 去除前后空格
        stageId.trim();
        
        if (stageId.length() > 0) {
            if (startStage(stageId)) {
                successCount++;
            }
        }
        
        if (commaPos < 0) {
            break;
        }
        startPos = commaPos + 1;
    }
    
    Serial.print(F("✅ 成功启动"));
    Serial.print(successCount);
    Serial.print(F("个环节，当前活跃环节数: "));
    Serial.println(activeStageCount);
    
    return successCount > 0;
}

void GameFlowManager::stopStage(const String& stageId) {
    String normalizedId = normalizeStageId(stageId);
    int index = findStageIndex(normalizedId);
    
    if (index >= 0) {
        Serial.print(F("⏹️ 停止环节[槽位"));
        Serial.print(index);
        Serial.print(F("]: "));
        Serial.println(normalizedId);
        
        stages[index].running = false;
        stages[index].stageId = "";
        activeStageCount--;
        updateCompatibilityVars();
    }
}

void GameFlowManager::stopCurrentStage() {
    // 兼容旧接口：停止第一个活跃环节
    if (activeStageCount > 0) {
        for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
            if (stages[i].running) {
                Serial.print(F("⏹️ 结束当前环节[槽位"));
                Serial.print(i);
                Serial.print(F("]: "));
                Serial.println(stages[i].stageId);
                
                stages[i].running = false;
                stages[i].stageId = "";
                activeStageCount--;
                updateCompatibilityVars();
                break;
            }
        }
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
    
    // 重置所有环节状态
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        stages[i].running = false;
        stages[i].stageId = "";
    }
    activeStageCount = 0;
    updateCompatibilityVars();
    
    // 重置所有通道音量为默认值
    resetAllVolumes();
    
    Serial.println(F("✅ 所有C102音频效果已停止"));
}

// ========================== 状态查询 ==========================
const String& GameFlowManager::getCurrentStageId() const {
    return currentStageId;
}

bool GameFlowManager::isStageRunning() const {
    return stageRunning;
}

bool GameFlowManager::isStageRunning(const String& stageId) {
    String normalizedId = normalizeStageId(stageId);
    int index = findStageIndex(normalizedId);
    return (index >= 0 && stages[index].running);
}

unsigned long GameFlowManager::getStageElapsedTime() const {
    if (stageRunning) {
        return millis() - stageStartTime;
    }
    return 0;
}

unsigned long GameFlowManager::getStageElapsedTime(const String& stageId) {
    String normalizedId = normalizeStageId(stageId);
    int index = findStageIndex(normalizedId);
    if (index >= 0 && stages[index].running) {
        return millis() - stages[index].startTime;
    }
    return 0;
}

int GameFlowManager::getActiveStageCount() const {
    return activeStageCount;
}

void GameFlowManager::getActiveStages(String stages[], int maxCount) {
    int count = 0;
    for (int i = 0; i < MAX_PARALLEL_STAGES && count < maxCount; i++) {
        if (this->stages[i].running) {
            stages[count++] = this->stages[i].stageId;
        }
    }
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
    if (activeStageCount == 0) {
        return;
    }
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 更新所有运行中的环节
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        if (stages[i].running) {
            const String& stageId = stages[i].stageId;
            
            // 根据环节ID调用对应的更新方法
            if (stageId == "000_0") {
                updateStep000(i);
            } else if (stageId == "001_2") {
                updateStep001_2(i);
            } else if (stageId == "002_0") {
                updateStep002(i);
            }
        }
    }
    
    // 更新兼容性变量
    updateCompatibilityVars();
}

void GameFlowManager::printStatus() {
    Serial.println(F("=== C102 GameFlowManager状态 ==="));
    Serial.print(F("活跃环节数: "));
    Serial.print(activeStageCount);
    Serial.print(F("/"));
    Serial.println(MAX_PARALLEL_STAGES);
    Serial.print(F("全局停止: "));
    Serial.println(globalStopped ? "是" : "否");
    
    if (activeStageCount > 0) {
        Serial.println(F("--- 运行中的环节 ---"));
        for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
            if (stages[i].running) {
                Serial.print(F("[槽位"));
                Serial.print(i);
                Serial.print(F("] "));
                Serial.print(stages[i].stageId);
                Serial.print(F(" - 运行时间: "));
                Serial.print(millis() - stages[i].startTime);
                Serial.print(F("ms"));
                if (stages[i].jumpRequested) {
                    Serial.print(F(" [已请求跳转]"));
                }
                Serial.println();
            }
        }
    } else {
        Serial.println(F("当前无运行环节"));
    }
    
    Serial.println(F("================================"));
}

// ========================== 环节跳转请求 ==========================
void GameFlowManager::requestStageJump(const String& nextStage) {
    // 兼容旧接口：使用第一个活跃环节
    if (activeStageCount > 0) {
        for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
            if (stages[i].running && !stages[i].jumpRequested) {
                requestMultiStageJump(stages[i].stageId, nextStage);
                break;
            }
        }
    }
}

void GameFlowManager::requestMultiStageJump(const String& currentStep, const String& nextSteps) {
    Serial.print(F("📤 请求从"));
    Serial.print(currentStep);
    Serial.print(F("跳转到环节: "));
    Serial.println(nextSteps);
    
    // 发送STEP_COMPLETE消息，支持多个next_step
    String message = "$[GAME]@C102{^STEP_COMPLETE^(current_step=\"" + currentStep + 
                    "\",next_step=\"" + nextSteps + 
                    "\",duration=" + String(getStageElapsedTime(currentStep)) + 
                    ",error_count=0)}#";
    
    harbingerClient.sendMessage(message);
    Serial.print(F("📡 发送消息: "));
    Serial.println(message);
    
    // 标记该环节已请求跳转
    int index = findStageIndex(currentStep);
    if (index >= 0) {
        stages[index].jumpRequested = true;
    }
}

// ========================== 私有方法实现 ==========================

// 环节完成通知
void GameFlowManager::notifyStageComplete(const String& currentStep, const String& nextStep, unsigned long duration) {
    // 查找环节索引
    int index = findStageIndex(currentStep);
    if (index >= 0 && stages[index].jumpRequested) {
        return;  // 避免重复通知
    }
    
    String message = "$[GAME]@C102{^STEP_COMPLETE^(current_step=\"" + currentStep + 
                    "\",next_step=\"" + nextStep + 
                    "\",duration=" + String(duration) + 
                    ",error_count=0)}#";
    
    harbingerClient.sendMessage(message);
    Serial.print(F("📡 环节完成通知: "));
    Serial.println(message);
    
    // 标记该环节已请求跳转
    if (index >= 0) {
        stages[index].jumpRequested = true;
    }
}

void GameFlowManager::notifyStageComplete(const String& currentStep, unsigned long duration) {
    // 查找环节索引
    int index = findStageIndex(currentStep);
    if (index >= 0 && stages[index].jumpRequested) {
        return;  // 避免重复通知
    }
    
    String message = "$[GAME]@C102{^STEP_COMPLETE^(current_step=\"" + currentStep + 
                    "\",duration=" + String(duration) + 
                    ",error_count=0)}#";
    
    harbingerClient.sendMessage(message);
    Serial.print(F("📡 环节完成通知: "));
    Serial.println(message);
    
    // 标记该环节已请求跳转
    if (index >= 0) {
        stages[index].jumpRequested = true;
    }
}

// C102音频环节更新方法
void GameFlowManager::updateStep000(int index) {
    if (index < 0 || index >= MAX_PARALLEL_STAGES || !stages[index].running) {
        return;
    }
    
    unsigned long elapsed = millis() - stages[index].startTime;
    StageState& stage = stages[index];
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 检查通道是否到启动时间
    if (!stage.state.stage000.channelStarted && elapsed >= STAGE_000_0_START) {
        voice.playSong(STAGE_000_0_CHANNEL, STAGE_000_0_SONG_ID);
        stage.state.stage000.channelStarted = true;
        Serial.print(F("🎵 [槽位"));
        Serial.print(index);
        Serial.print(F("] "));
        Serial.print(elapsed);
        Serial.print(F("ms: 通道"));
        Serial.print(STAGE_000_0_CHANNEL);
        Serial.print(F("开始播放"));
        Serial.println(STAGE_000_0_SONG_ID);
    }
    
    // 1000ms后跳转到下一环节
    if (!stage.jumpRequested && elapsed >= STAGE_000_0_COMPLETE_TIME) {
        Serial.print(F("⏰ [槽位"));
        Serial.print(index);
        Serial.print(F("] 环节000_0完成，跳转到"));
        Serial.println(STAGE_000_0_NEXT_STAGE);
        notifyStageComplete("000_0", STAGE_000_0_NEXT_STAGE, elapsed);
        // 继续音频循环，等待服务器下一步指令
    }
    
    // 持续检查音频状态，如果停止了就重新播放（只在播放稳定期后开始检测）
    if (stage.state.stage000.channelStarted && elapsed >= STAGE_000_0_STABLE_TIME) {
        if (elapsed - stage.state.stage000.lastCheckTime >= STAGE_000_0_CHECK_INTERVAL) {
            // 检查音频状态，只有空闲时才重新播放
            if (!voice.isBusy(STAGE_000_0_CHANNEL)) {
                voice.playSong(STAGE_000_0_CHANNEL, STAGE_000_0_SONG_ID);
                Serial.print(F("🔄 [槽位"));
                Serial.print(index);
                Serial.print(F("] 通道"));
                Serial.print(STAGE_000_0_CHANNEL);
                Serial.print(F("音频播放完成，重新播放"));
                Serial.println(STAGE_000_0_SONG_ID);
            }
            stage.state.stage000.lastCheckTime = elapsed;
        }
    }
}

void GameFlowManager::updateStep001_2(int index) {
    if (index < 0 || index >= MAX_PARALLEL_STAGES || !stages[index].running) {
        return;
    }
    
    unsigned long elapsed = millis() - stages[index].startTime;
    StageState& stage = stages[index];
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 检查通道是否到启动时间
    if (!stage.state.stage001_2.channelStarted && elapsed >= STAGE_001_2_START) {
        voice.playSong(STAGE_001_2_CHANNEL, STAGE_001_2_SONG_ID);
        stage.state.stage001_2.channelStarted = true;
        Serial.print(F("🎵 [槽位"));
        Serial.print(index);
        Serial.print(F("] "));
        Serial.print(elapsed);
        Serial.print(F("ms: 通道"));
        Serial.print(STAGE_001_2_CHANNEL);
        Serial.print(F("开始播放"));
        Serial.println(STAGE_001_2_SONG_ID);
    }
    
    // 音量淡出控制：3秒内从30减到0，每100ms减1
    if (elapsed <= STAGE_001_2_FADE_DURATION) {
        if (!stage.state.stage001_2.volumeUpdateComplete && 
            elapsed - stage.state.stage001_2.lastVolumeUpdate >= STAGE_001_2_FADE_INTERVAL) {
            // 计算当前应该的音量：30 - (elapsed / 100)
            int targetVolume = STAGE_001_2_FADE_START_VOL - (elapsed / STAGE_001_2_FADE_INTERVAL);
            if (targetVolume < STAGE_001_2_FADE_END_VOL) {
                targetVolume = STAGE_001_2_FADE_END_VOL;
            }
            
            if (stage.state.stage001_2.currentVolume != targetVolume) {
                stage.state.stage001_2.currentVolume = targetVolume;
                voice.setVolume(STAGE_001_2_FADE_CHANNEL, targetVolume);
                stage.state.stage001_2.lastVolumeUpdate = elapsed;
                
                Serial.print(F("🔊 [槽位"));
                Serial.print(index);
                Serial.print(F("] 通道"));
                Serial.print(STAGE_001_2_FADE_CHANNEL);
                Serial.print(F("音量调整为"));
                Serial.print(targetVolume);
                Serial.print(F("("));
                Serial.print(elapsed);
                Serial.println(F("ms)"));
            }
            
            if (targetVolume == STAGE_001_2_FADE_END_VOL) {
                stage.state.stage001_2.volumeUpdateComplete = true;
            }
        }
    } else if (elapsed >= STAGE_001_2_FADE_DURATION && 
               elapsed < STAGE_001_2_FADE_DURATION + 200 && 
               stage.state.stage001_2.volumeUpdateComplete) {
        // 3秒后停止第2路音频（只执行一次）
        voice.stop(STAGE_001_2_FADE_CHANNEL);
        stage.state.stage001_2.volumeUpdateComplete = false; // 防止重复执行
        Serial.print(F("⏹️ [槽位"));
        Serial.print(index);
        Serial.print(F("] 通道"));
        Serial.print(STAGE_001_2_FADE_CHANNEL);
        Serial.println(F("音频停止"));
    }
    
    // 83.347秒后跳转到下一环节
    if (!stage.jumpRequested && elapsed >= STAGE_001_2_DURATION) {
        // 重置第2通道音量为默认值
        resetChannelVolume(STAGE_001_2_FADE_CHANNEL);
        
        if (strlen(STAGE_001_2_NEXT_STAGE) > 0) {
            Serial.print(F("⏰ [槽位"));
            Serial.print(index);
            Serial.print(F("] 环节001_2完成，跳转到"));
            Serial.println(STAGE_001_2_NEXT_STAGE);
            notifyStageComplete("001_2", STAGE_001_2_NEXT_STAGE, elapsed);
        } else {
            Serial.print(F("⏰ [槽位"));
            Serial.print(index);
            Serial.println(F("] 环节001_2完成"));
            notifyStageComplete("001_2", elapsed);
        }
    }
}

void GameFlowManager::updateStep002(int index) {
    if (index < 0 || index >= MAX_PARALLEL_STAGES || !stages[index].running) {
        return;
    }
    
    unsigned long elapsed = millis() - stages[index].startTime;
    StageState& stage = stages[index];
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 检查通道1是否到启动时间
    if (!stage.state.stage002.channel1Started && elapsed >= STAGE_002_0_CHANNEL1_START) {
        voice.playSong(STAGE_002_0_CHANNEL1, STAGE_002_0_SONG_ID1);
        stage.state.stage002.channel1Started = true;
        Serial.print(F("🎵 [槽位"));
        Serial.print(index);
        Serial.print(F("] "));
        Serial.print(elapsed);
        Serial.print(F("ms: 通道"));
        Serial.print(STAGE_002_0_CHANNEL1);
        Serial.print(F("开始播放"));
        Serial.println(STAGE_002_0_SONG_ID1);
    }
    
    // 检查通道2是否到启动时间
    if (!stage.state.stage002.channel2Started && elapsed >= STAGE_002_0_CHANNEL2_START) {
        voice.playSong(STAGE_002_0_CHANNEL2, STAGE_002_0_SONG_ID2);
        stage.state.stage002.channel2Started = true;
        Serial.print(F("🎵 [槽位"));
        Serial.print(index);
        Serial.print(F("] "));
        Serial.print(elapsed);
        Serial.print(F("ms: 通道"));
        Serial.print(STAGE_002_0_CHANNEL2);
        Serial.print(F("开始播放"));
        Serial.println(STAGE_002_0_SONG_ID2);
    }
    
    // 30秒时触发多环节跳转
    if (!stage.state.stage002.multiJumpTriggered && elapsed >= STAGE_002_0_MULTI_JUMP_TIME) {
        stage.state.stage002.multiJumpTriggered = true;
        Serial.print(F("🚀 [槽位"));
        Serial.print(index);
        Serial.print(F("] 30秒时触发多环节跳转: "));
        Serial.println(STAGE_002_0_MULTI_JUMP_STAGES);
        requestMultiStageJump("002_0", STAGE_002_0_MULTI_JUMP_STAGES);
    }
    
    // 60秒后跳转到下一环节或报告完成
    if (!stage.jumpRequested && elapsed >= STAGE_002_0_DURATION) {
        if (strlen(STAGE_002_0_NEXT_STAGE) > 0) {
            Serial.print(F("⏰ [槽位"));
            Serial.print(index);
            Serial.print(F("] 环节002_0完成，跳转到"));
            Serial.println(STAGE_002_0_NEXT_STAGE);
            notifyStageComplete("002_0", STAGE_002_0_NEXT_STAGE, elapsed);
        } else {
            Serial.print(F("⏰ [槽位"));
            Serial.print(index);
            Serial.println(F("] 环节002_0完成"));
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

// ========================== 音量管理方法 ==========================
void GameFlowManager::initializeAllVolumes() {
    Serial.println(F("🔊 初始化所有通道音量..."));
    for (int channel = 1; channel <= TOTAL_CHANNELS; channel++) {
        voice.setVolume(channel, DEFAULT_VOLUME);
        Serial.print(F("🔊 通道"));
        Serial.print(channel);
        Serial.print(F("音量设置为"));
        Serial.println(DEFAULT_VOLUME);
        delay(50); // 避免命令发送过快
    }
    Serial.println(F("✅ 所有通道音量初始化完成"));
}

void GameFlowManager::resetChannelVolume(int channel) {
    if (channel >= 1 && channel <= TOTAL_CHANNELS) {
        voice.setVolume(channel, DEFAULT_VOLUME);
        Serial.print(F("🔊 重置通道"));
        Serial.print(channel);
        Serial.print(F("音量为"));
        Serial.println(DEFAULT_VOLUME);
    }
}

void GameFlowManager::resetAllVolumes() {
    Serial.println(F("🔊 重置所有通道音量..."));
    for (int channel = 1; channel <= TOTAL_CHANNELS; channel++) {
        voice.setVolume(channel, DEFAULT_VOLUME);
        Serial.print(F("🔊 通道"));
        Serial.print(channel);
        Serial.print(F("音量重置为"));
        Serial.println(DEFAULT_VOLUME);
        delay(50); // 避免命令发送过快
    }
    Serial.println(F("✅ 所有通道音量重置完成"));
}