/**
 * =============================================================================
 * GameStateMachine - 游戏状态管理库
 * 版本: 1.0
 * 创建日期: 2024-12-28
 * 
 * 功能:
 * - 游戏状态管理和转换
 * - 会话ID验证
 * - 状态转换回调
 * - 内存优化设计
 * =============================================================================
 */

#ifndef GAME_STATE_MACHINE_H
#define GAME_STATE_MACHINE_H

#include <Arduino.h>
#include "UniversalGameProtocol.h"
#include "TimeManager.h"

/**
 * @brief 游戏状态机类
 * 管理游戏状态转换和会话验证
 */
class GameStateMachine {
private:
    GameState currentState;
    String currentSessionId;
    bool initialized;
    
    // 回调函数指针
    void (*stateChangeCallback)(GameState oldState, GameState newState);
    void (*deviceControlCallback)(bool start);
    
public:
    // ========================== 构造和初始化 ==========================
    GameStateMachine();
    void begin();
    
    // ========================== 状态管理 ==========================
    void setState(GameState newState);
    GameState getState() const;
    const String& getSessionId() const;
    bool isInitialized() const;
    
    // ========================== 会话管理 ==========================
    void setSessionId(const String& sessionId);
    bool validateSessionId(const String& sessionId) const;
    void generateNewSessionId();
    void clearSession();
    
    // ========================== 游戏命令处理 ==========================
    bool processGameCommand(const String& command, const String& params);
    
    // ========================== 状态查询 ==========================
    bool canAcceptCommand(const String& command) const;
    bool isValidCommand(const String& command, const String& sessionId) const;
    
    // ========================== 回调设置 ==========================
    void setStateChangeCallback(void (*callback)(GameState oldState, GameState newState));
    void setDeviceControlCallback(void (*callback)(bool start));
    
    // ========================== 辅助函数 ==========================
    static String getStateString(GameState state);
    String extractSessionId(const String& params) const;
    
    // ========================== 调试功能 ==========================
    void printStatus() const;
    
private:
    // 内部状态转换逻辑
    bool canTransitionTo(GameState newState) const;
    void executeStateTransition(GameState oldState, GameState newState);
};

// ========================== 全局实例 ==========================
extern GameStateMachine gameStateMachine;

#endif // GAME_STATE_MACHINE_H 