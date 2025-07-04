/**
 * =============================================================================
 * C101 主程序 - 4路语音控制器
 * 版本: 2.0
 * 创建日期: 2024-12-28
 * 更新日期: 2025-01-03
 * 
 * 功能:
 * - PWM控制和渐变效果
 * - 串口命令处理
 * - 游戏状态管理
 * - 4路BY语音模块控制
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
#include "BY_VoiceController_Unified.h"  // 统一的BY语音控制器
#include "C101_SimpleConfig.h"            // C101控制器配置

// ========================== 配置 ==========================
#define SERIAL_BAUDRATE 115200
#define DEBUG 1
#define CONTROLLER_ID "C101"
#define ENABLE_NETWORK true   // 网络开关
#define ENABLE_VOICE true     // 语音控制开关

// ========================== 全局实例 ==========================
BY_VoiceController_Unified voice;  // 统一语音控制器实例

void setup() {
    // 初始化串口
    Serial.begin(SERIAL_BAUDRATE);
    while (!Serial && millis() < 3000); // 等待串口就绪，最多3秒

    Serial.println(F("=== C101 4路语音控制器启动 ==="));

    // 初始化各个组件
    MPWM_BEGIN();              // PWM系统 (核心)
    DIO_BEGIN();               // 数字IO控制器
    commandProcessor.begin();  // 命令处理器
    gameStageManager.begin();  // 游戏环节状态机
    gameFlowManager.begin();   // 游戏流程管理器
    // gameFlowManager.setStagePrefix(C101_STAGE_PREFIX);  // C101版本不需要前缀设置
    gameProtocolHandler.begin(); // 游戏协议处理器
    
    // ========================== 统一语音控制器初始化 ==========================
    if (ENABLE_VOICE) {
        Serial.println(F("=== 统一语音控制器配置 ==="));
        
        // 使用配置文件中的引脚定义
        voice.setSoftSerialPins(C101_SOFT_TX_PIN, C101_SOFT_RX_PIN);  // 使用配置文件的定义
        voice.setBusyPins(C101_BUSY_PINS[0], C101_BUSY_PINS[1], 
                          C101_BUSY_PINS[2], C101_BUSY_PINS[3]);      // 使用配置文件的数组
        
        // 初始化系统
        if (voice.begin()) {
            Serial.println(F("✅ 统一语音控制器初始化成功"));
        } else {
            Serial.println(F("❌ 统一语音控制器初始化失败"));
        }
        
        Serial.println(F("语音命令格式：c1p(播放) c1s(停止) c1:11(播放歌曲11)"));
        Serial.println(F("测试命令：c1:11, c2:11, c3:11, c4:11"));
    }
    
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
    
    // ========================== 硬件初始化 ==========================
    Serial.println(F("初始化硬件..."));
    
    // 使用配置文件的硬件初始化函数
    initC101Hardware();  // 初始化C101特定的硬件引脚
    Serial.println(F("✓ 4路音频模块硬件配置完成"));
    
    // 设置GameStateMachine的设备控制回调
    gameStateMachine.setDeviceControlCallback(onGameStateChange);
    Serial.println(F("✓ 游戏状态回调设置完成"));
    
    // 8. 通信客户端初始化
    if (harbingerClient.begin(CONTROLLER_ID, "Arduino")) {
        Serial.println(F("✓ 通信客户端初始化成功"));
        
        // 连接到服务器
        IPAddress serverIP(C101_SERVER_IP_1, C101_SERVER_IP_2, C101_SERVER_IP_3, C101_SERVER_IP_4);
        if (harbingerClient.connect(serverIP, C101_SERVER_PORT)) {
            Serial.println(F("✓ 服务器连接成功"));
        } else {
            Serial.println(F("✗ 服务器连接失败"));
        }
    } else {
        Serial.println(F("✗ 通信客户端初始化失败"));
    }
    
    // 9. 协议处理器初始化
    gameProtocolHandler.begin();
    Serial.println(F("✓ 游戏协议处理器初始化完成"));
    
    Serial.println(F("所有组件初始化完成"));
    Serial.println(F("输入 'help' 查看所有命令"));
}

void loop() {
    // ========================== 紧急开门高优先级检测 ==========================
    // 在所有其他处理之前，立即检测紧急开门信号
    static bool lastEmergencyState = HIGH;
    bool currentEmergencyState = digitalRead(C101_DOOR_CARD_COM_PIN);
    if (lastEmergencyState == HIGH && currentEmergencyState == LOW) {
        // 立即触发紧急开门，不等待gameFlowManager.update()
        digitalWrite(C101_DOOR_LOCK_PIN, LOW);  // Pin26解锁（断电）
        Serial.println(F("🚨⚡ 高优先级紧急开门触发！"));
    }
    lastEmergencyState = currentEmergencyState;
    
    // ========================== 串口命令处理 ==========================
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
            Serial.print(F(">>> "));
            Serial.println(command);
            
            // 统一交给CommandProcessor处理所有命令
            bool processed = commandProcessor.processCommand(command);
            
            if (!processed) {
                Serial.println(F("未知命令，输入 'help' 查看帮助"));
            }
        }
    }
    
    // ========================== 系统更新 ==========================
    MPWM_UPDATE();                    // PWM更新 (必须调用)
    DIO_UPDATE();                     // 数字IO更新 (必须调用)
    
    // 语音控制器状态更新
    if (ENABLE_VOICE) {
        voice.update();  // 统一状态监控 (所有通道)
    }
    
    // 4. 网络更新（如果启用）
    if (ENABLE_NETWORK) {
        systemHelper.checkNetworkHealth();
        harbingerClient.handleAllNetworkOperations();
    }
    
    // 5. 更新游戏流程管理器
    gameFlowManager.update();
    
    // 6. 系统监控（移除不存在的update方法）
    // systemHelper.update();  // 注释掉不存在的方法
}

// ========================== 辅助函数 ==========================
// 所有命令处理现在统一由CommandProcessor处理

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

// ========================== 游戏状态变化回调 ==========================
void onGameStateChange(bool start) {
    Serial.print(F("🎮 游戏状态变化: "));
    Serial.println(start ? "启动" : "停止");
    
    if (start) {
        // 游戏启动时的处理
        Serial.println(F("游戏启动，初始化设备状态"));
        // 门锁和灯光状态已在GameFlowManager.begin()中初始化
        // 这里可以添加游戏启动时的其他特殊处理
    } else {
        // 游戏停止时的处理
        Serial.println(F("🏁 游戏停止，设置结束状态"));
        gameFlowManager.resetDoorAndLightState();
        
        // 停止所有音频
        if (ENABLE_VOICE) {
            voice.stopAll();
            Serial.println(F("🔇 所有音频已停止"));
        }
        
        // 停止所有游戏环节
        gameFlowManager.stopAllStages();
        Serial.println(F("🛑 所有游戏环节已停止"));
        
        Serial.println(F("✅ 游戏结束状态设置完成"));
    }
}





