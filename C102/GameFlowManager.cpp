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
        Serial.println(F("🎵 环节000_0：第一路音频循环播放201号音频"));
        voice.playSong(1, 201);  // 第1路播放201号音频
        return true;
    } else if (normalizedId == "001_2") {
        Serial.println(F("🎵 环节001_2：第1路播放0001，第2路音量淡出"));
        voice.playSong(1, 1);    // 第1路播放0001号音频
        voice.setVolume(2, 0);   // 第2路音量淡出到0
        return true;
    } else if (normalizedId == "002_0") {
        Serial.println(F("🎵 环节002_0：第1路播放0002，第2路播放0201"));
        voice.playSong(1, 2);    // 第1路播放0002号音频
        voice.playSong(2, 201);  // 第2路播放0201号音频
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
    Serial.println(F("000_0 - 第一路音频循环播放201号音频"));
    Serial.println(F("001_2 - 第1路播放0001，第2路音量淡出"));
    Serial.println(F("002_0 - 第1路播放0002，第2路播放0201"));
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
        updateStep001();
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
    
    // 1秒后报告完成
    if (!jumpRequested && elapsed >= STAGE_000_0_REPORT_DELAY) {
        Serial.println(F("⏰ 环节000_0达到报告时间，通知完成"));
        notifyStageComplete("000_0", elapsed);
        return;  // 报告完成后停止处理，避免音频循环
    }
    
    // 继续播放音频（如果还没报告完成）
    if (!jumpRequested) {
        // 检查音频是否还在播放，如果停止了就重新播放
        // 注意：这里可能需要根据BY语音模块的实际API调整
        static unsigned long lastCheckTime = 0;
        if (elapsed - lastCheckTime > 2000) {  // 每2秒检查一次
            voice.playSong(1, 201);  // 重新播放201号音频
            lastCheckTime = elapsed;
            Serial.println(F("🔄 重新播放201号音频"));
        }
    }
}

void GameFlowManager::updateStep001() {
    unsigned long elapsed = getStageElapsedTime();
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 83.347秒后建议跳转到002_0
    if (!jumpRequested && elapsed >= STAGE_001_2_DURATION) {
        Serial.println(F("⏰ 环节001_2完成，建议跳转到002_0"));
        notifyStageComplete("001_2", "002_0", elapsed);
    }
}

void GameFlowManager::updateStep002() {
    unsigned long elapsed = getStageElapsedTime();
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 002环节默认60秒后报告完成（不指定下一步）
    if (!jumpRequested && elapsed >= STAGE_002_0_DURATION) {
        Serial.println(F("⏰ 环节002_0完成"));
        notifyStageComplete("002_0", elapsed);
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