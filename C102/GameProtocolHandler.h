#ifndef GAMEPROTOCOLHANDLER_H
#define GAMEPROTOCOLHANDLER_H

#include <Arduino.h>

class GameProtocolHandler {
public:
    // 初始化
    void begin();
    
    // 消息处理
    void processGameMessage(const String& message);
    
    // 游戏环节启动方法（已废弃，使用GameFlowManager）
    void startGameStage(const String& stageId);
    
private:
    // 内部处理方法
    void handleInit(const String& params);
    void handleStart(const String& params);
    void handleStop(const String& params);
    void handleStep(const String& params);
    
    // 工具方法
    String extractParam(const String& params, const String& paramName);
};

// 全局实例
extern GameProtocolHandler gameProtocolHandler;

#endif // GAMEPROTOCOLHANDLER_H 