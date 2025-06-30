/**
 * =============================================================================
 * GameStageStateMachine - 游戏环节状态机 - 实现文件
 * 版本: 1.0
 * 创建日期: 2025-01-03
 * =============================================================================
 */

#include "GameStageStateMachine.h"

// 全局实例
GameStageStateMachine gameStageManager;

// ========================== 构造和初始化 ==========================
GameStageStateMachine::GameStageStateMachine() : 
    currentStage("IDLE"),
    currentSessionId(""),
    stageStartTime(0),
    initialized(false),
    stageChangeCallback(nullptr) {
}

void GameStageStateMachine::begin() {
    currentStage = "IDLE";
    currentSessionId = "";
    stageStartTime = millis();
    initialized = true;
    
    #ifdef DEBUG
    Serial.println(F("GameStageStateMachine初始化完成"));
    #endif
}

// ========================== 状态管理 ==========================
void GameStageStateMachine::setStage(const String& stage) {
    if (!initialized) return;
    
    if (currentStage != stage) {
        String oldStage = currentStage;
        currentStage = stage;
        stageStartTime = millis();
        
        #ifdef DEBUG
        Serial.print(F("环节变更: "));
        Serial.print(oldStage);
        Serial.print(F(" -> "));
        Serial.println(stage);
        #endif
        
        // 触发回调
        if (stageChangeCallback) {
            stageChangeCallback(oldStage, stage);
        }
    }
}

const String& GameStageStateMachine::getStage() const {
    return currentStage;
}

bool GameStageStateMachine::isStage(const String& stage) const {
    return currentStage == stage;
}

// ========================== 会话管理 ==========================
void GameStageStateMachine::setSessionId(const String& sessionId) {
    currentSessionId = sessionId;
    
    #ifdef DEBUG
    Serial.print(F("设置会话ID: "));
    Serial.println(sessionId);
    #endif
}

const String& GameStageStateMachine::getSessionId() const {
    return currentSessionId;
}

bool GameStageStateMachine::hasSession() const {
    return currentSessionId.length() > 0;
}

void GameStageStateMachine::clearSession() {
    currentSessionId = "";
    setStage("IDLE");  // 清除会话时回到IDLE状态
    
    #ifdef DEBUG
    Serial.println(F("会话已清除"));
    #endif
}

// ========================== 时间管理 ==========================
unsigned long GameStageStateMachine::getStageStartTime() const {
    return stageStartTime;
}

unsigned long GameStageStateMachine::getStageElapsedTime() const {
    if (!initialized) return 0;
    return millis() - stageStartTime;
}

// ========================== 状态查询 ==========================
bool GameStageStateMachine::isIdle() const {
    return currentStage == "IDLE";
}

bool GameStageStateMachine::isPlaying() const {
    return currentStage != "IDLE" && currentStage != "ERROR";
}

bool GameStageStateMachine::isInitialized() const {
    return initialized;
}

// ========================== 回调设置 ==========================
void GameStageStateMachine::setStageChangeCallback(void (*callback)(const String& oldStage, const String& newStage)) {
    stageChangeCallback = callback;
}

// ========================== 调试功能 ==========================
void GameStageStateMachine::printStatus() const {
    Serial.println(F("=== GameStageStateMachine 状态 ==="));
    Serial.print(F("当前环节: "));
    Serial.println(currentStage);
    Serial.print(F("会话ID: "));
    Serial.println(currentSessionId.length() > 0 ? currentSessionId : F("无"));
    Serial.print(F("环节时长: "));
    Serial.print(getStageElapsedTime() / 1000.0);
    Serial.println(F("秒"));
    Serial.println(F("================================"));
} 