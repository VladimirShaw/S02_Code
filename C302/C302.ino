/**
 * =============================================================================
 * Arduino Template - 主程序
 * 版本: 1.0
 * 创建日期: 2024-12-28
 * 
 * 功能:
 * - PWM控制和渐变效果
 * - 串口命令处理
 * - 游戏状态管理
 * - 系统监控
 * =============================================================================
 */

#include "MillisPWM.h"
#include "CommandProcessor.h"
#include "GameStageStateMachine.h"
#include "GameFlowManager.h"
#include "TimeManager.h"
#include "DigitalIOController.h"
#include "ArduinoSystemHelper.h"
#include "UniversalHarbingerClient.h"
#include "GameProtocolHandler.h"
#include "C302_SimpleConfig.h"

// ========================== 配置 ==========================
#define SERIAL_BAUDRATE 115200
#define DEBUG 1
#define CONTROLLER_ID "C302"
#define ENABLE_NETWORK true  // 网络开关

void setup() {
    // 初始化串口
    Serial.begin(SERIAL_BAUDRATE);
    while (!Serial && millis() < 3000); // 等待串口就绪，最多3秒
  
    // 初始化各个组件
    MPWM_BEGIN();              // PWM系统 (核心)
    DIO_BEGIN();               // 数字IO控制器
  commandProcessor.begin();  // 命令处理器
    gameStageManager.begin();  // 游戏环节状态机
    gameFlowManager.begin();   // 游戏流程管理器
    gameProtocolHandler.begin(); // 游戏协议处理器
    
    // 网络初始化（可选）
    if (ENABLE_NETWORK) {
        Serial.println(F("初始化网络系统..."));
        systemHelper.begin(CONTROLLER_ID, 0);  // 无硬件设备配置
        systemHelper.setMessageCallback(onNetworkMessage);  // 必须在initNetwork之前设置
        
        IPAddress serverIP(192, 168, 10, 10);
        if (systemHelper.initNetwork(serverIP, 9000)) {
            Serial.println(F("网络初始化成功"));
  } else {
            Serial.println(F("网络初始化失败"));
  }
}

    // ========================== C302硬件初始化 ==========================
    Serial.println(F("初始化C302硬件..."));
    
    // 使用配置文件中的硬件初始化函数
    initC302Hardware();
    
    Serial.print(F("C302硬件初始化完成 - "));
    Serial.print(C302_DEVICE_COUNT);
    Serial.print(F("个设备 + 25个按键 ("));
    Serial.print(C302_DEVICE_COUNT + 25);
    Serial.println(F("个引脚)"));
    Serial.println(F("所有组件初始化完成"));

}

void loop() {
// ========================== 串口命令处理 ==========================
    if (Serial.available()) {
  String command = Serial.readStringUntil('\n');
  command.trim();
  
        if (command.length() > 0) {
            Serial.print(F(">>> "));
            Serial.println(command);
            
            // 直接交给命令处理器处理
            bool processed = commandProcessor.processCommand(command);
            
            if (!processed) {
                Serial.println(F("未知命令，输入 'help' 查看帮助"));
            }
      }
    }
    
    // ========================== 系统更新 ==========================
    MPWM_UPDATE();                    // PWM更新 (必须调用)
    DIO_UPDATE();                     // 数字IO更新 (必须调用)
    
    // 网络更新（如果启用）
    if (ENABLE_NETWORK) {
        systemHelper.checkNetworkHealth();
        harbingerClient.handleAllNetworkOperations();
    }
    
    // ========================== 游戏流程执行 ==========================
    gameFlowManager.update();  // 游戏流程更新（按键检测等）
}

// ========================== 网络消息回调 ==========================
void onNetworkMessage(String message) {
    Serial.print(F("收到网络消息: "));
    Serial.println(message);
    
    // 将GAME消息委托给专用处理器
    if (message.indexOf("[GAME]") != -1) {
        gameProtocolHandler.processGameMessage(message);
    } else {
        // 简单处理其他消息类型
        if (message.indexOf("REGISTER_CONFIRM") != -1) {
            Serial.println(F("设备注册确认"));
        } else if (message.indexOf("HEARTBEAT_ACK") != -1) {
            Serial.println(F("心跳确认"));
        }
    }
}



