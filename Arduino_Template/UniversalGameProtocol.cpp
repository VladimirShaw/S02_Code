/**
 * =============================================================================
 * 通用游戏协议实现 - UniversalGameProtocol.cpp
 * 创建日期: 2025-01-03
 * 描述信息: 通用的游戏协议处理库实现
 * =============================================================================
 */

#include "UniversalGameProtocol.h"

// ========================== 全局实例 ==========================
UniversalGameProtocol gameProtocol;

// ========================== 构造函数和析构函数 ==========================
UniversalGameProtocol::UniversalGameProtocol() {
    currentState = GAME_IDLE;
    currentGameType = GAME_UNKNOWN;
    currentSessionId = "";
    currentLevel = 0;
    commandCallback = nullptr;
    stateChangeCallback = nullptr;
}

UniversalGameProtocol::~UniversalGameProtocol() {
    // 析构函数
}

// ========================== 初始化 ==========================
void UniversalGameProtocol::begin() {
    currentState = GAME_IDLE;
    currentGameType = GAME_UNKNOWN;
    currentSessionId = "";
    currentLevel = 0;
}

// ========================== 回调设置 ==========================
void UniversalGameProtocol::setCommandCallback(GameCommandCallback callback) {
    this->commandCallback = callback;
}

void UniversalGameProtocol::setStateChangeCallback(GameStateChangeCallback callback) {
    this->stateChangeCallback = callback;
}

// ========================== 消息处理 ==========================
void UniversalGameProtocol::processGameMessage(const String& message) {
    String command, params;
    if (!parseGameMessage(message, command, params)) {
        return;
    }
    
    handleGameCommand(command, params);
}

bool UniversalGameProtocol::parseGameMessage(const String& message, String& command, String& params) {
    // 解析格式: $[GAME]@DEVICE_ID{^COMMAND^(params)}#
    int startPos = message.indexOf("{^");
    if (startPos == -1) return false;
    
    int endPos = message.indexOf("^(", startPos + 2);
    if (endPos == -1) return false;
    
    command = message.substring(startPos + 2, endPos);
    
    int paramsStart = endPos + 2;
    int paramsEnd = message.lastIndexOf(")}#");
    if (paramsEnd == -1) return false;
    
    params = message.substring(paramsStart, paramsEnd);
    
    return true;
}

void UniversalGameProtocol::handleGameCommand(const String& command, const String& params) {
    if (command == "INIT") {
        // 解析session_id
        String sessionId = "";
        int sessionPos = params.indexOf("session_id=");
        if (sessionPos != -1) {
            int start = sessionPos + 11;
            int end = params.indexOf(",", start);
            if (end == -1) end = params.length();
            sessionId = params.substring(start, end);
        }
        initGame(sessionId);
        
    } else if (command == "START") {
        // 解析session_id和level
        String sessionId = "";
        int level = 1;
        
        int sessionPos = params.indexOf("session_id=");
        if (sessionPos != -1) {
            int start = sessionPos + 11;
            int end = params.indexOf(",", start);
            if (end == -1) end = params.length();
            sessionId = params.substring(start, end);
        }
        
        int levelPos = params.indexOf("level=");
        if (levelPos != -1) {
            int start = levelPos + 6;
            int end = params.indexOf(",", start);
            if (end == -1) end = params.length();
            level = params.substring(start, end).toInt();
        }
        
        startGame(sessionId, level);
        
    } else if (command == "STOP") {
        stopGame();
        
    } else if (command == "PAUSE") {
        pauseGame();
        
    } else if (command == "RESUME") {
        resumeGame();
        
    } else if (command == "EMERGENCY_STOP") {
        emergencyStop();
        
    } else if (command == "SKIP_LEVEL") {
        skipLevel();
        
    } else {
        // 未知命令
    }
    
    // 触发命令回调
    if (commandCallback) {
        commandCallback(command, params);
    }
}

// ========================== 状态查询 ==========================
GameState UniversalGameProtocol::getCurrentState() const {
    return currentState;
}

GameType UniversalGameProtocol::getCurrentGameType() const {
    return currentGameType;
}

String UniversalGameProtocol::getCurrentSessionId() const {
    return currentSessionId;
}

int UniversalGameProtocol::getCurrentLevel() const {
    return currentLevel;
}

// ========================== 游戏控制 ==========================
void UniversalGameProtocol::initGame(const String& sessionId) {
    currentSessionId = sessionId;
    currentGameType = parseGameType(sessionId);
    currentLevel = 0;
    changeState(GAME_INIT);
}

void UniversalGameProtocol::startGame(const String& sessionId, int level) {
    currentSessionId = sessionId;
    currentGameType = parseGameType(sessionId);
    currentLevel = level;
    changeState(GAME_PLAYING);
}

void UniversalGameProtocol::pauseGame() {
    if (currentState == GAME_PLAYING) {
        changeState(GAME_PAUSED);
    }
}

void UniversalGameProtocol::resumeGame() {
    if (currentState == GAME_PAUSED) {
        changeState(GAME_PLAYING);
    }
}

void UniversalGameProtocol::stopGame() {
    changeState(GAME_IDLE);
    currentSessionId = "";
    currentLevel = 0;
    currentGameType = GAME_UNKNOWN;
}

void UniversalGameProtocol::emergencyStop() {
    changeState(GAME_ERROR);
}

void UniversalGameProtocol::skipLevel() {
    if (currentState == GAME_PLAYING) {
        currentLevel++;
    }
}

// ========================== 内部方法 ==========================
GameType UniversalGameProtocol::parseGameType(const String& sessionId) {
    if (sessionId.startsWith("A_")) {
        return GAME_RAINVEIL;  // 雨声游戏
    } else if (sessionId.startsWith("C_")) {
        return GAME_SPARKLE;   // 灵火之森游戏
    } else {
        return GAME_UNKNOWN;
    }
}

void UniversalGameProtocol::changeState(GameState newState) {
    if (currentState != newState) {
        GameState oldState = currentState;
        currentState = newState;
        
        // 触发状态变更回调
        if (stateChangeCallback) {
            stateChangeCallback(oldState, newState);
        }
    }
}

// ========================== 工具方法 ==========================
String UniversalGameProtocol::gameStateToString(GameState state) const {
    switch (state) {
        case GAME_IDLE: return "IDLE";
        case GAME_INIT: return "INIT";
        case GAME_PLAYING: return "PLAYING";
        case GAME_PAUSED: return "PAUSED";
        case GAME_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

String UniversalGameProtocol::gameTypeToString(GameType type) const {
    switch (type) {
        case GAME_RAINVEIL: return "雨声";
        case GAME_SPARKLE: return "灵火之森";
        case GAME_UNKNOWN: return "未知";
        default: return "未定义";
    }
}

void UniversalGameProtocol::printStatus() {
    Serial.println(F("========== 游戏状态 =========="));
    Serial.print(F("当前状态: "));
    Serial.println(gameStateToString(currentState));
    Serial.print(F("游戏类型: "));
    Serial.println(gameTypeToString(currentGameType));
    Serial.print(F("会话ID: "));
    Serial.println(currentSessionId.length() > 0 ? currentSessionId : F("无"));
    Serial.print(F("当前关卡: "));
    Serial.println(currentLevel);
    Serial.println(F("=============================="));
} 