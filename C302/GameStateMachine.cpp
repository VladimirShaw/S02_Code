/**
 * =============================================================================
 * GameStateMachine - 游戏状态管理库 - 实现文件
 * 版本: 1.0
 * 创建日期: 2024-12-28
 * =============================================================================
 */

#include "GameStateMachine.h"

// ========================== 全局实例 ==========================
GameStateMachine gameStateMachine;

// ========================== 构造和初始化 ==========================
GameStateMachine::GameStateMachine() : 
    currentState(GAME_IDLE), 
    initialized(false),
    stateChangeCallback(nullptr),
    deviceControlCallback(nullptr) {
}

void GameStateMachine::begin() {
    currentState = GAME_IDLE;
    currentSessionId = "";
    initialized = true;
    
    #ifdef DEBUG
    Serial.println(F("GameStateMachine初始化完成"));
    #endif
}

// ========================== 状态管理 ==========================
void GameStateMachine::setState(GameState newState) {
    if (!initialized) return;
    
    GameState oldState = currentState;
    if (canTransitionTo(newState)) {
        currentState = newState;
        executeStateTransition(oldState, newState);
        
        #ifdef DEBUG
        Serial.print(F("状态: "));
        Serial.print(getStateString(oldState));
        Serial.print(F(" -> "));
        Serial.println(getStateString(newState));
        #endif
        
        // 调用回调
        if (stateChangeCallback) {
            stateChangeCallback(oldState, newState);
        }
    }
}

GameState GameStateMachine::getState() const {
    return currentState;
}

const String& GameStateMachine::getSessionId() const {
    return currentSessionId;
}

bool GameStateMachine::isInitialized() const {
    return initialized;
}

// ========================== 会话管理 ==========================
void GameStateMachine::setSessionId(const String& sessionId) {
    currentSessionId = sessionId;
}

bool GameStateMachine::validateSessionId(const String& sessionId) const {
    if (currentSessionId.length() == 0) return true; // 无会话时总是有效
    return currentSessionId == sessionId;
}

void GameStateMachine::generateNewSessionId() {
    currentSessionId = "SESSION_" + String(TimeManager::now());
}

void GameStateMachine::clearSession() {
    currentSessionId = "";
}

// ========================== 游戏命令处理 ==========================
bool GameStateMachine::processGameCommand(const String& command, const String& params) {
    if (!initialized) return false;
    
    String sessionId = extractSessionId(params);
    
    // INIT命令 - 只在IDLE状态接受
    if (command == "INIT" && (currentState == GAME_IDLE || currentState == GAME_ERROR)) {
        setState(GAME_INIT);
        clearSession();
        if (deviceControlCallback) deviceControlCallback(false); // 停止设备
        return true;
        
    // START命令 - 只在INIT状态接受
    } else if (command == "START" && currentState == GAME_INIT) {
        if (sessionId.length() > 0) {
            setSessionId(sessionId);
        } else {
            generateNewSessionId();
        }
        setState(GAME_PLAYING);
        if (deviceControlCallback) deviceControlCallback(true); // 启动设备
        return true;
        
    // STOP命令 - 需要验证session ID
    } else if (command == "STOP" && validateSessionId(sessionId)) {
        setState(GAME_IDLE);
        clearSession();
        if (deviceControlCallback) deviceControlCallback(false); // 停止设备
        return true;
        
    // PAUSE命令
    } else if (command == "PAUSE" && currentState == GAME_PLAYING && validateSessionId(sessionId)) {
        setState(GAME_PAUSED);
        if (deviceControlCallback) deviceControlCallback(false); // 暂停设备
        return true;
        
    // RESUME命令
    } else if (command == "RESUME" && currentState == GAME_PAUSED && validateSessionId(sessionId)) {
        setState(GAME_PLAYING);
        if (deviceControlCallback) deviceControlCallback(true); // 恢复设备
        return true;
        
    // EMERGENCY_STOP命令
    } else if (command == "EMERGENCY_STOP" && validateSessionId(sessionId)) {
        setState(GAME_ERROR);
        if (deviceControlCallback) deviceControlCallback(false); // 紧急停止设备
        return true;
        
    // SKIP_LEVEL命令 - 只在游戏进行中接受
    } else if (command == "SKIP_LEVEL" && currentState == GAME_PLAYING && validateSessionId(sessionId)) {
        // 跳关命令不改变游戏状态，只是通知上层处理
        #ifdef DEBUG
        Serial.println(F("处理跳关命令"));
        #endif
        return true;
    }
    
    return false; // 命令无效或状态不允许
}

// ========================== 状态查询 ==========================
bool GameStateMachine::canAcceptCommand(const String& command) const {
    if (command == "INIT") {
        return (currentState == GAME_IDLE || currentState == GAME_ERROR);
    } else if (command == "START") {
        return (currentState == GAME_INIT);
    } else if (command == "STOP") {
        return (currentState != GAME_IDLE);
    } else if (command == "PAUSE") {
        return (currentState == GAME_PLAYING);
    } else if (command == "RESUME") {
        return (currentState == GAME_PAUSED);
    } else if (command == "EMERGENCY_STOP") {
        return (currentState != GAME_IDLE && currentState != GAME_ERROR);
    } else if (command == "SKIP_LEVEL") {
        return (currentState == GAME_PLAYING);
    }
    return false;
}

bool GameStateMachine::isValidCommand(const String& command, const String& sessionId) const {
    return canAcceptCommand(command) && validateSessionId(sessionId);
}

// ========================== 回调设置 ==========================
void GameStateMachine::setStateChangeCallback(void (*callback)(GameState oldState, GameState newState)) {
    stateChangeCallback = callback;
}

void GameStateMachine::setDeviceControlCallback(void (*callback)(bool start)) {
    deviceControlCallback = callback;
}

// ========================== 辅助函数 ==========================
String GameStateMachine::getStateString(GameState state) {
    switch (state) {
        case GAME_IDLE: return F("IDLE");
        case GAME_INIT: return F("INIT");
        case GAME_PLAYING: return F("PLAYING");
        case GAME_PAUSED: return F("PAUSED");
        case GAME_ERROR: return F("ERROR");
        default: return F("UNKNOWN");
    }
}

String GameStateMachine::extractSessionId(const String& params) const {
    int start = params.indexOf("session_id=");
    if (start == -1) return "";
    start += 11;
    int end = params.indexOf(",", start);
    if (end == -1) end = params.length();
    return params.substring(start, end);
}

// ========================== 调试功能 ==========================
void GameStateMachine::printStatus() const {
    #ifdef DEBUG
    Serial.print(F("状态: "));
    Serial.print(getStateString(currentState));
    Serial.print(F(" 会话: "));
    Serial.println(currentSessionId);
    #endif
}

// ========================== 私有方法 ==========================
bool GameStateMachine::canTransitionTo(GameState newState) const {
    // 简化的状态转换验证
    return true; // 允许所有转换，具体逻辑在processGameCommand中处理
}

void GameStateMachine::executeStateTransition(GameState oldState, GameState newState) {
    // 状态转换时的特殊处理
    if (newState == GAME_IDLE || newState == GAME_ERROR) {
        // 进入空闲或错误状态时清理会话
        // 注意：这里不调用clearSession()避免递归
    }
} 