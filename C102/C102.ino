/**
 * =============================================================================
 * C102 主程序 - 4路语音控制器
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
#include "C102_SimpleConfig.h"            // C102控制器配置

// ========================== 配置 ==========================
#define SERIAL_BAUDRATE 115200
#define DEBUG 1
#define CONTROLLER_ID "C102"
#define ENABLE_NETWORK true   // 网络开关
#define ENABLE_VOICE true     // 语音控制开关

// ========================== 全局实例 ==========================
BY_VoiceController_Unified voice;  // 统一语音控制器实例

void setup() {
    // 初始化串口
    Serial.begin(SERIAL_BAUDRATE);
    while (!Serial && millis() < 3000); // 等待串口就绪，最多3秒

    Serial.println(F("=== C102 4路语音控制器启动 ==="));

    // 初始化各个组件
    MPWM_BEGIN();              // PWM系统 (核心)
    DIO_BEGIN();               // 数字IO控制器
    commandProcessor.begin();  // 命令处理器
    gameStageManager.begin();  // 游戏环节状态机
    gameFlowManager.begin();   // 游戏流程管理器
    // gameFlowManager.setStagePrefix(C102_STAGE_PREFIX);  // C102版本不需要前缀设置
    gameProtocolHandler.begin(); // 游戏协议处理器
    
    // ========================== 统一语音控制器初始化 ==========================
    if (ENABLE_VOICE) {
        Serial.println(F("=== 统一语音控制器配置 ==="));
        
        // 使用配置文件中的引脚定义
        voice.setSoftSerialPins(C102_SOFT_TX_PIN, C102_SOFT_RX_PIN);  // 使用配置文件的定义
        voice.setBusyPins(C102_BUSY_PINS[0], C102_BUSY_PINS[1], 
                          C102_BUSY_PINS[2], C102_BUSY_PINS[3]);      // 使用配置文件的数组
        
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
    initC102Hardware();  // 初始化C102特定的硬件引脚
    Serial.println(F("✓ 4路音频模块硬件配置完成"));
    
    Serial.println(F("所有组件初始化完成"));
    Serial.println(F("输入 'help' 查看所有命令"));
}

void loop() {
    // ========================== 串口命令处理 ==========================
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
            Serial.print(F(">>> "));
            Serial.println(command);
            
            bool processed = false;
            
            // 1. 优先处理语音控制命令
            if (ENABLE_VOICE && isVoiceCommand(command)) {
                Serial.print(F("🎵 识别为语音命令: "));
                Serial.println(command);
                voice.processSerialCommand(command);  // 统一处理所有通道
                processed = true;
            }
            // 2. 其他命令交给原有命令处理器
            else {
                Serial.print(F("🔧 交给系统命令处理器: "));
                Serial.println(command);
                processed = commandProcessor.processCommand(command);
            }
            
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
    
    // 网络更新（如果启用）
    if (ENABLE_NETWORK) {
        systemHelper.checkNetworkHealth();
        harbingerClient.handleAllNetworkOperations();
    }
    
    // ========================== 游戏流程执行 ==========================
    gameFlowManager.update();  // 游戏流程更新
}

// ========================== 语音命令识别 ==========================
bool isVoiceCommand(String command) {
    Serial.print(F("🔍 检查命令: '"));
    Serial.print(command);
    Serial.print(F("' 长度: "));
    Serial.println(command.length());
    
    // 识别语音控制命令格式 c1-c4
    if (command.length() >= 2 && command[0] == 'c' && 
        command[1] >= '1' && command[1] <= '4') {
        Serial.println(F("✅ 匹配语音通道命令格式"));
        return true;  // c1p, c2s, c3:1234 等
    }
    
    // 批量控制命令
    if (command == "stopall" || command == "playall" || command.startsWith("volall:")) {
        Serial.println(F("✅ 匹配批量控制命令"));
        return true;
    }
    
    // 测试命令
    if (command == "test1" || command == "test201" || command == "testall") {
        Serial.println(F("✅ 匹配测试命令"));
        return true;
    }
    
    // 语音系统命令
    if (command == "status" || command == "s" || command == "help" || command == "h") {
        Serial.println(F("✅ 匹配系统命令"));
        return true;
    }
    
    Serial.println(F("❌ 不是语音命令"));
    return false;
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





