#include "GameProtocolHandler.h"
#include "GameStageStateMachine.h"
#include "GameFlowManager.h"
#include "UniversalHarbingerClient.h"

// 外部全局实例
extern UniversalHarbingerClient harbingerClient;
extern GameFlowManager gameFlowManager;
extern GameStageStateMachine gameStageManager;

// 全局实例
GameProtocolHandler gameProtocolHandler;

void GameProtocolHandler::begin() {
    // 不再管理自己的状态，使用GameStageStateMachine
    Serial.println(F("GameProtocolHandler初始化完成"));
}

void GameProtocolHandler::processGameMessage(const String& message) {
    Serial.print(F("处理GAME消息: "));
    Serial.println(message);
    
    // 解析协议格式: $[GAME]@DEVICE_ID{^COMMAND^(params)}#
    int commandStart = message.indexOf("^") + 1;
    int commandEnd = message.indexOf("^", commandStart);
    
    if (commandStart <= 0 || commandEnd <= commandStart) {
        Serial.println(F("GAME消息格式错误"));
        return;
    }
    
    String command = message.substring(commandStart, commandEnd);
    
    // 提取参数部分
    int paramsStart = message.indexOf("(");
    int paramsEnd = message.lastIndexOf(")");
    String params = "";
    if (paramsStart > 0 && paramsEnd > paramsStart) {
        params = message.substring(paramsStart + 1, paramsEnd);
    }
    
    Serial.print(F("GAME命令: "));
    Serial.print(command);
    Serial.print(F(" 参数: "));
    Serial.println(params);
    
    // 处理不同的GAME命令
    if (command == "INIT") {
        handleInit(params);
    } else if (command == "START") {
        handleStart(params);
    } else if (command == "STOP") {
        handleStop(params);
    } else if (command == "STEP") {
        handleStep(params);
    } else {
        Serial.print(F("未知GAME命令: "));
        Serial.println(command);
    }
}

void GameProtocolHandler::handleInit(const String& params) {
    String mode = extractParam(params, "mode");
    String difficulty = extractParam(params, "difficulty");
    
    if (mode.length() == 0) mode = "normal";
    if (difficulty.length() == 0) difficulty = "normal";
    
    Serial.print(F("游戏初始化: mode="));
    Serial.print(mode);
    Serial.print(F(" difficulty="));
    Serial.println(difficulty);
    
    // 使用GameStageStateMachine重置状态
    gameStageManager.clearSession();
    gameStageManager.setStage("INIT_COMPLETE");
    
    // 🎯 INIT命令处理完成后，自动启动000_0环节（初始化环节）
    Serial.println(F("🚀 INIT命令完成，自动启动000_0环节..."));
    if (gameFlowManager.startStage("000_0")) {
        Serial.println(F("✅ 000_0环节自动启动成功"));
    } else {
        Serial.println(F("❌ 000_0环节自动启动失败"));
    }
    
    // 发送GAME响应
    harbingerClient.sendGAMEResponse("INIT", "result=success,mode=" + mode + ",difficulty=" + difficulty);
}

void GameProtocolHandler::handleStart(const String& params) {
    String sessionId = extractParam(params, "session_id");
    String level = extractParam(params, "level");
    String mode = extractParam(params, "mode");
    String stage = extractParam(params, "stage");  // 直接从参数获取环节名
    
    if (level.length() == 0) level = "1";
    if (mode.length() == 0) mode = "normal";
    
    Serial.print(F("游戏开始: session="));
    Serial.print(sessionId);
    Serial.print(F(" level="));
    Serial.print(level);
    Serial.print(F(" mode="));
    Serial.print(mode);
    Serial.print(F(" stage="));
    Serial.println(stage);
    
    // 使用GameStageStateMachine设置会话
    gameStageManager.setSessionId(sessionId);
    
    // 如果有指定环节，则设置；否则保持当前状态
    if (stage.length() > 0) {
        gameStageManager.setStage(stage);
    }
    
    // 🎯 START命令处理完成后，先停止当前环节，再启动001_1环节（游戏开始环节，C101专用）
    Serial.println(F("🛑 停止当前环节，准备启动001_1环节..."));
    gameFlowManager.stopStage("000_0");  // 明确停止000_0环节
    
    Serial.println(F("🚀 START命令完成，自动启动001_1环节..."));
    if (gameFlowManager.startStage("001_1")) {
        Serial.println(F("✅ 001_1环节自动启动成功"));
    } else {
        Serial.println(F("❌ 001_1环节自动启动失败"));
    }
    
    // 发送GAME响应
    String result = "result=success,session_id=" + sessionId + ",level=" + level + ",mode=" + mode;
    if (stage.length() > 0) {
        result += ",stage=" + stage;
    }
    harbingerClient.sendGAMEResponse("START", result);
}

void GameProtocolHandler::handleStop(const String& params) {
    String reason = extractParam(params, "reason");
    if (reason.length() == 0) reason = "manual";
    
    Serial.print(F("游戏停止: reason="));
    Serial.println(reason);
    
    // 停止所有游戏环节
    gameFlowManager.stopAllStages();
    
    // 使用GameStageStateMachine清除会话
    gameStageManager.clearSession();  // 这会自动设置为IDLE状态
    
    // 发送STOP响应
    harbingerClient.sendGAMEResponse("STOP", "result=success,reason=" + reason);
}

void GameProtocolHandler::handleStep(const String& params) {
    String sessionId = extractParam(params, "session_id");
    String stepId = extractParam(params, "step_id");
    
    Serial.print(F("游戏步骤: session="));
    Serial.print(sessionId);
    Serial.print(F(" step="));
    Serial.println(stepId);
    
    // 验证会话ID
    if (sessionId.length() == 0) {
        Serial.println(F("错误: 缺少session_id"));
        harbingerClient.sendGAMEResponse("STEP_COMPLETE", "result=ERROR,message=missing_session_id");
        return;
    }
    
    // 验证会话是否匹配
    if (gameStageManager.getSessionId() != sessionId) {
        Serial.println(F("错误: 会话ID不匹配"));
        harbingerClient.sendGAMEResponse("STEP_COMPLETE", "result=ERROR,message=session_mismatch");
        return;
    }
    
    // 检查是否包含多个环节（逗号分隔）
    bool isMultipleSteps = (stepId.indexOf(',') >= 0);
    
    if (isMultipleSteps) {
        Serial.println(F("🎯 检测到多环节并行指令"));
        
        // 使用GameStageStateMachine跳转到第一个环节
        int firstComma = stepId.indexOf(',');
        String firstStep = stepId.substring(0, firstComma);
        firstStep.trim();
        gameStageManager.setStage(firstStep);
        
        // 启动多个并行环节
        if (gameFlowManager.startMultipleStages(stepId)) {
            Serial.print(F("✅ 成功启动多个并行环节: "));
            Serial.println(stepId);
            
            // 发送STEP_COMPLETE确认响应
            String result = "result=OK,session_id=" + sessionId + ",step_id=" + stepId + ",parallel=true";
            harbingerClient.sendGAMEResponse("STEP_COMPLETE", result);
        } else {
            Serial.print(F("❌ 启动并行环节失败: "));
            Serial.println(stepId);
            
            // 发送错误响应
            String result = "result=ERROR,session_id=" + sessionId + ",step_id=" + stepId + ",message=parallel_start_failed";
            harbingerClient.sendGAMEResponse("STEP_COMPLETE", result);
        }
    } else {
        // 单个环节处理（保持原有逻辑）
        gameStageManager.setStage(stepId);
        
        // 启动具体的游戏环节
        if (gameFlowManager.startStage(stepId)) {
            Serial.print(F("✅ 成功跳转到环节: "));
            Serial.println(stepId);
            
            // 发送STEP_COMPLETE确认响应
            String result = "result=OK,session_id=" + sessionId + ",step_id=" + stepId;
            harbingerClient.sendGAMEResponse("STEP_COMPLETE", result);
        } else {
            Serial.print(F("ℹ️ 环节无需跳转: "));
            Serial.print(stepId);
            Serial.println(F(" (不是此Arduino负责的环节)"));
            
            // 发送STEP_COMPLETE确认响应（正常情况，不是错误）
            String result = "result=OK,session_id=" + sessionId + ",step_id=" + stepId + ",message=not_responsible";
            harbingerClient.sendGAMEResponse("STEP_COMPLETE", result);
        }
    }
}



// ========================== 游戏环节启动 ==========================
// 此方法已废弃 - 游戏流程现在由GameFlowManager管理
void GameProtocolHandler::startGameStage(const String& stageId) {
    Serial.println(F("⚠️  警告：startGameStage已废弃，请使用GameFlowManager"));
    Serial.print(F("建议使用: gameFlowManager.startStage(\""));
    Serial.print(stageId);
    Serial.println(F("\")"));
}

// ========================== 工具方法 ==========================
String GameProtocolHandler::extractParam(const String& params, const String& paramName) {
    String searchStr = paramName + "=";
    int startPos = params.indexOf(searchStr);
    if (startPos == -1) return "";
    
    startPos += searchStr.length();
    int endPos = params.indexOf(",", startPos);
    if (endPos == -1) endPos = params.length();
    
    return params.substring(startPos, endPos);
} 