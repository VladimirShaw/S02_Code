/**
 * =============================================================================
 * 通用游戏协议 - UniversalGameProtocol.h
 * 创建日期: 2025-01-03
 * 描述信息: 通用的游戏协议处理库，支持所有游戏命令
 * =============================================================================
 */

#ifndef UNIVERSAL_GAME_PROTOCOL_H
#define UNIVERSAL_GAME_PROTOCOL_H

#include <Arduino.h>

// ========================== 游戏状态 ==========================
enum GameState {
    GAME_IDLE = 0,
    GAME_INIT = 1,
    GAME_PLAYING = 2,
    GAME_PAUSED = 3,
    GAME_ERROR = 255
};

// ========================== 游戏类型 ==========================
enum GameType {
    GAME_UNKNOWN = 0,
    GAME_RAINVEIL = 1,      // 雨声游戏 (A_开头)
    GAME_SPARKLE = 2        // 灵火之森游戏 (C_开头)
};

// ========================== 回调函数类型 ==========================
typedef void (*GameCommandCallback)(const String& command, const String& params);
typedef void (*GameStateChangeCallback)(GameState oldState, GameState newState);

// ========================== UniversalGameProtocol类 ==========================
class UniversalGameProtocol {
private:
    // 游戏状态
    GameState currentState;
    GameType currentGameType;
    String currentSessionId;
    int currentLevel;
    
    // 回调函数
    GameCommandCallback commandCallback;
    GameStateChangeCallback stateChangeCallback;
    
    // 内部方法
    GameType parseGameType(const String& sessionId);
    void changeState(GameState newState);
    void handleGameCommand(const String& command, const String& params);
    
public:
    // 构造函数和析构函数
    UniversalGameProtocol();
    ~UniversalGameProtocol();
    
    // 初始化
    void begin();
    
    // 回调设置
    void setCommandCallback(GameCommandCallback callback);
    void setStateChangeCallback(GameStateChangeCallback callback);
    
    // 消息处理
    void processGameMessage(const String& message);
    bool parseGameMessage(const String& message, String& command, String& params);
    
    // 状态查询
    GameState getCurrentState() const;
    GameType getCurrentGameType() const;
    String getCurrentSessionId() const;
    int getCurrentLevel() const;
    
    // 游戏控制
    void initGame(const String& sessionId);
    void startGame(const String& sessionId, int level = 1);
    void pauseGame();
    void resumeGame();
    void stopGame();
    void emergencyStop();
    void skipLevel();
    
    // 工具方法
    String gameStateToString(GameState state) const;
    String gameTypeToString(GameType type) const;
    void printStatus();
};

// 全局实例
extern UniversalGameProtocol gameProtocol;

#endif // UNIVERSAL_GAME_PROTOCOL_H 