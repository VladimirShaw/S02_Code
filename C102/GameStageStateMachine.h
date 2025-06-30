/**
 * =============================================================================
 * GameStageStateMachine - 游戏环节状态机
 * 版本: 1.0
 * 创建日期: 2025-01-03
 * 
 * 功能:
 * - 管理游戏环节状态（IDLE, 072-4, 073-1等）
 * - 管理会话ID
 * - 提供状态查询和变更接口
 * - 不执行任何游戏逻辑，只管理状态
 * =============================================================================
 */

#ifndef GAME_STAGE_STATE_MACHINE_H
#define GAME_STAGE_STATE_MACHINE_H

#include <Arduino.h>

class GameStageStateMachine {
private:
    String currentStage;           // 当前环节（如 "IDLE", "072-4", "073-1"）
    String currentSessionId;       // 当前会话ID
    unsigned long stageStartTime;  // 环节开始时间
    bool initialized;              // 是否已初始化
    
    // 状态变更回调
    void (*stageChangeCallback)(const String& oldStage, const String& newStage);
    
public:
    // ========================== 构造和初始化 ==========================
    GameStageStateMachine();
    void begin();
    
    // ========================== 状态管理 ==========================
    void setStage(const String& stage);
    const String& getStage() const;
    bool isStage(const String& stage) const;
    
    // ========================== 会话管理 ==========================
    void setSessionId(const String& sessionId);
    const String& getSessionId() const;
    bool hasSession() const;
    void clearSession();
    
    // ========================== 时间管理 ==========================
    unsigned long getStageStartTime() const;
    unsigned long getStageElapsedTime() const;
    
    // ========================== 状态查询 ==========================
    bool isIdle() const;
    bool isPlaying() const;  // 非IDLE状态
    bool isInitialized() const;
    
    // ========================== 回调设置 ==========================
    void setStageChangeCallback(void (*callback)(const String& oldStage, const String& newStage));
    
    // ========================== 调试功能 ==========================
    void printStatus() const;
};

// 全局实例
extern GameStageStateMachine gameStageManager;

#endif // GAME_STAGE_STATE_MACHINE_H 