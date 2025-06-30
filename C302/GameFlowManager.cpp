/**
 * =============================================================================
 * GameFlowManager - 游戏流程管理器 - 实现文件
 * 版本: 1.0
 * 创建日期: 2025-01-03
 * =============================================================================
 */

#include "GameFlowManager.h"
#include "MillisPWM.h"
#include "DigitalIOController.h"
#include "UniversalHarbingerClient.h"
#include "GameStageStateMachine.h"
#include "SimpleGameStage.h"

// 外部全局实例
extern UniversalHarbingerClient harbingerClient;
extern GameStageStateMachine gameStageManager;

// 全局实例
GameFlowManager gameFlowManager;

// 静态成员变量定义
volatile bool GameFlowManager::pin25Triggered = false;
bool GameFlowManager::lastPin25State = HIGH;

// 遗迹地图游戏按键监控变量定义
volatile bool GameFlowManager::buttonPressed[25] = {false};
bool GameFlowManager::lastButtonState[25] = {HIGH};

// 遗迹地图游戏状态变量定义
int GameFlowManager::lastPressedButton = 0;     // 0表示还没有按过任何按键
int GameFlowManager::errorCount = 0;            // 错误次数从0开始
int GameFlowManager::successCount = 0;          // 成功次数从0开始 (新增)
bool GameFlowManager::gameActive = false;       // 游戏默认非激活状态
int GameFlowManager::currentLevel = 1;          // 默认从Level 1开始
String GameFlowManager::lastCompletionSource = ""; // 上次完成来源

// 刷新步骤循环追踪变量定义
bool GameFlowManager::lastRefreshWas5 = false;  // 默认从-5开始，所以初始为false（下次是-5）

// 频闪状态机变量定义
bool GameFlowManager::strobeActive = false;
bool GameFlowManager::strobeState = false;
unsigned long GameFlowManager::strobeNextTime = 0;
unsigned long GameFlowManager::strobeEndTime = 0;

// 矩阵旋转系统变量定义
int GameFlowManager::currentRotation = 0;       // 默认无旋转
int GameFlowManager::lastRotation = -1;         // 初始化为-1表示无历史

// ========================== 构造和初始化 ==========================
GameFlowManager::GameFlowManager() {
    currentStageId = "";
    stageStartTime = 0;
    stageRunning = false;
    stagePrefix = "072-";  // 默认前缀
}

void GameFlowManager::begin() {
    Serial.println(F("GameFlowManager初始化完成"));
}

// ========================== 环节前缀配置 ==========================
void GameFlowManager::setStagePrefix(const String& prefix) {
    stagePrefix = prefix;
    Serial.print(F("🔧 环节ID前缀设置为: "));
    Serial.println(prefix);
}

const String& GameFlowManager::getStagePrefix() const {
    return stagePrefix;
}

String GameFlowManager::buildStageId(const String& suffix) {
    return stagePrefix + suffix;
}

// ========================== 环节控制 ==========================
bool GameFlowManager::startStage(const String& stageId) {
    // 标准化环节ID
    String normalizedId = normalizeStageId(stageId);
    
    Serial.print(F("=== 启动游戏环节: "));
    Serial.print(stageId);
    if (normalizedId != stageId) {
        Serial.print(F(" (标准化为: "));
        Serial.print(normalizedId);
        Serial.print(F(")"));
    }
    Serial.println(F(" ==="));
    
    // 不停止当前状态，保持所有效果的连续性
    // 只更新环节管理信息
    currentStageId = normalizedId;
    stageStartTime = millis();
    stageRunning = true;
    
    // 根据环节ID执行对应逻辑
    if (normalizedId == "072-0") {
        defineStage072_0();
        return true;
    } else if (normalizedId == "072-0.5") {
        defineStage072_0_5();
        return true;
    } else if (normalizedId == "072-1") {
        defineStage072_1();
        return true;
    } else if (normalizedId == "072-2") {
        defineStage072_2();
        return true;
    } else if (normalizedId == "072-3") {
        defineStage072_3();
        return true;
    } else if (normalizedId == "072-4") {
        defineStage072_4();
        return true;
    } else if (normalizedId == "072-5") {
        defineStage072_5();
        recordRefreshStage(normalizedId);  // 记录刷新步骤
        return true;
    } else if (normalizedId == "072-6") {
        defineStage072_6();
        recordRefreshStage(normalizedId);  // 记录刷新步骤
        return true;
    } else if (normalizedId == "072-7") {
        defineStage072_7();
        return true;
    } else if (normalizedId == "072-8") {
        defineStage072_8();
        return true;
    } else if (normalizedId == "072-9") {
        defineStage072_9();
        return true;
    } else if (normalizedId == "080-0") {
        defineStage080_0();
        return true;
    } else {
        Serial.print(F("❌ 未定义的环节: "));
        Serial.println(normalizedId);
        stageRunning = false;
        currentStageId = "";
        return false;
    }
}

void GameFlowManager::stopCurrentStage() {
    if (stageRunning) {
        Serial.print(F("⏹️ 结束当前环节: "));
        Serial.println(currentStageId);
        Serial.println(F("💡 保持所有输出状态，不清除任何效果"));
        
        // 重置输入状态
        pin25Triggered = false;
        lastPin25State = HIGH;
        
        stageRunning = false;
        currentStageId = "";
        stageStartTime = 0;
    }
}

void GameFlowManager::stopAllStages() {
    Serial.println(F("🛑 强制停止所有游戏环节和输出效果"));
    
    // 停止所有PWM效果
    MillisPWM::stopAll();
    
    // 停止所有数字IO效果
    DigitalIOController::stopAllOutputs();
    
    // 重置环节状态
    stageRunning = false;
    currentStageId = "";
    stageStartTime = 0;
    
    // 重置输入状态
    pin25Triggered = false;
    lastPin25State = HIGH;
    
    Serial.println(F("✅ 所有效果已清除"));
}

// ========================== 状态查询 ==========================
const String& GameFlowManager::getCurrentStageId() const {
    return currentStageId;
}

bool GameFlowManager::isStageRunning() const {
    return stageRunning;
}

unsigned long GameFlowManager::getStageElapsedTime() const {
    if (!stageRunning) return 0;
    return millis() - stageStartTime;
}

// ========================== 环节列表 ==========================
bool GameFlowManager::isValidStageId(const String& stageId) {
    String normalizedId = normalizeStageId(stageId);
    return (normalizedId == "072-0" || 
            normalizedId == "072-0.5" || 
            normalizedId == "072-1" ||
            normalizedId == "072-2" ||
            normalizedId == "072-3" ||
            normalizedId == "072-4" ||
            normalizedId == "072-5" ||
            normalizedId == "072-6" ||
            normalizedId == "072-7" ||
            normalizedId == "072-8" ||
            normalizedId == "072-9" ||
            normalizedId == "080-0");
}

void GameFlowManager::printAvailableStages() {
    Serial.println(F("=== C302遗迹地图游戏环节 ==="));
    Serial.println(F("072-0    - 游戏初始化 (蜡烛灯点亮)"));
    Serial.println(F("072-0.5  - 准备阶段 (根据Level设置初始状态)"));
    Serial.println(F("072-1    - 第一次正确庆祝 (12秒后跳转刷新)"));
    Serial.println(F("072-2    - 第二次正确庆祝 (10秒后跳转刷新)"));
    Serial.println(F("072-3    - 第三次正确庆祝 (10秒后跳转刷新)"));
    Serial.println(F("072-4    - 第3关 (按键序列3)"));
    Serial.println(F("072-5    - 刷新光效1 (1秒后跳转目标)"));
    Serial.println(F("072-6    - 刷新光效2 (1秒后跳转目标)"));
    Serial.println(F("072-7    - 错误效果1 (16秒后跳转刷新)"));
    Serial.println(F("072-8    - 错误效果2 (12秒后跳转刷新)"));
    Serial.println(F("072-9    - 错误效果3 (9秒后跳转刷新)"));
    Serial.println(F("080-0    - 最终胜利 (胜利庆祝)"));
    Serial.println();
    Serial.println(F("胜利条件: 累计成功3次 → 080-0"));
    Serial.println(F("Level顺序: 1→2→4→3→4→3... (正确进级)"));
    Serial.println(F("错误规则: Level1/2错误保持原Level，Level3/4错误相互切换"));
    Serial.println(F("支持格式: 072-0, 072_0, stage_072_0"));
}

// ========================== 更新和调试功能 ==========================
void GameFlowManager::update() {
    // 第一步：检查所有输入状态，设置全局标记
    checkInputs();
    
    // 第二步：处理频闪状态机
    if (strobeActive) {
        unsigned long now = millis();
        if (now >= strobeNextTime) {
            if (now >= strobeEndTime) {
                // 频闪结束
                strobeActive = false;
                MillisPWM::setBrightness(22, 0);  // 左侧蜡烛灭
                MillisPWM::setBrightness(23, 0);  // 右侧蜡烛灭
                Serial.println(F("🕯️ 蜡烛频闪结束"));
            } else {
                // 切换频闪状态
                strobeState = !strobeState;
                int brightness = strobeState ? 255 : 0;
                MillisPWM::setBrightness(22, brightness);  // 左侧蜡烛
                MillisPWM::setBrightness(23, brightness);  // 右侧蜡烛
                
                // 计算下次切换时间
                strobeNextTime = now + (strobeState ? CANDLE_STROBE_ON_TIME : CANDLE_STROBE_OFF_TIME);
            }
        }
    }
    
    // 第三步：处理所有输入事件
    processInputEvents();
    
    // 第四步：更新时刻表系统（用于072-7/8/9的定时效果）
    gameStage.update();
}

void GameFlowManager::checkInputs() {
    // 检查引脚25按键状态（只在072-0环节中监听）
    if (stageRunning && currentStageId == "072-0") {
        bool currentState = digitalRead(25);
        
        // 检测下降沿（按键按下）
        if (lastPin25State == HIGH && currentState == LOW) {
            pin25Triggered = true;  // 设置全局标记
        }
        
        lastPin25State = currentState;
    }
    
    // 遗迹地图游戏按键监控（在072-0.5环节中监听）
    if (stageRunning && currentStageId == "072-0.5") {
        for (int i = 0; i < 25; i++) {
            int buttonNumber = i + 1;  // 按键编号1-25
            int inputPin = getButtonInputPin(buttonNumber);  // 获取输入引脚
            
            bool currentState = digitalRead(inputPin);
            
            // 检测下降沿（按键按下）
            if (lastButtonState[i] == HIGH && currentState == LOW) {
                buttonPressed[i] = true;  // 设置按键按下标记
            }
            
            lastButtonState[i] = currentState;
        }
    }
}

void GameFlowManager::processInputEvents() {
    // 处理引脚25按键事件
    if (pin25Triggered) {
        pin25Triggered = false;  // 清除标记
        
        Serial.println(F("🔘 检测到引脚25按键按下"));
        Serial.println(F("📤 环节完成通知: 072-0 → 072-0.5"));
        
        // 发送STEP_COMPLETE消息，通知服务器环节完成
        unsigned long duration = getStageElapsedTime();
        notifyStageComplete("072-0", "072-0.5", duration);
        
        Serial.println(F("✅ 环节完成通知已发送"));
    }
    
    // 处理遗迹地图游戏按键事件
    for (int i = 0; i < 25; i++) {
        if (buttonPressed[i]) {
            buttonPressed[i] = false;  // 清除标记
            
            int buttonNumber = i + 1;  // 按键编号1-25
            Serial.print(F("🔘 检测到按键按下: "));
            Serial.println(buttonNumber);
            
            // 处理遗迹地图游戏逻辑
            handleMapButtonPress(buttonNumber);
        }
    }
}

void GameFlowManager::printStatus() {
    Serial.println(F("=== 游戏流程状态 ==="));
    Serial.print(F("当前环节: "));
    if (stageRunning) {
        Serial.print(currentStageId);
        Serial.print(F(" (运行中, "));
        Serial.print(getStageElapsedTime());
        Serial.println(F("ms)"));
    } else {
        Serial.println(F("无"));
    }
    
    Serial.print(F("当前Level: "));
    Serial.println(currentLevel);
    Serial.print(F("成功次数: "));
    Serial.print(successCount);
    Serial.println(F("/3"));
    Serial.print(F("错误次数: "));
    Serial.println(errorCount);
    Serial.print(F("游戏状态: "));
    Serial.println(gameActive ? F("激活") : F("非激活"));
}

// ========================== 环节跳转请求 ==========================
void GameFlowManager::requestStageJump(const String& nextStage) {
    Serial.print(F("📤 请求环节跳转: "));
    Serial.print(currentStageId);
    Serial.print(F(" → "));
    Serial.println(nextStage);
    
    // 发送STEP_COMPLETE消息给服务器
    unsigned long duration = getStageElapsedTime();
    notifyStageComplete(currentStageId, nextStage, duration);
}

// ========================== 具体环节定义 ==========================
void GameFlowManager::defineStage072_0() {
    Serial.println(F("📍 环节 072-0：游戏初始化"));
    
    // ========================== 基础游戏状态重置 ==========================
    Serial.println(F("🔄 初始化游戏系统"));
    
    // 重置错误计数和成功计数
    errorCount = 0;
    successCount = 0;  // 重置成功计数
    
    // 重置Level到初始状态
    currentLevel = 1;  // 重置到Level 1
    Serial.println(F("🎯 Level重置为1"));
    
    // 矩阵旋转系统：保持历史，不重置
    // 这样可以确保每次072-0.5都从其他方向中选择
    Serial.println(F("🔄 矩阵旋转系统保持历史"));
    
    // 停止游戏状态（等待进入072-0.5时激活）
    gameActive = false;
    
    // 重置刷新循环（从-5开始）
    resetRefreshCycle();
    
    Serial.println(F("✅ 游戏系统初始化完成"));
    
    // 点亮两个蜡烛灯 (C03LK01, C03LK02)
    Serial.println(F("  - 蜡烛灯点亮 (Pin22, Pin23)"));
    MillisPWM::setBrightness(22, 255);  // C03LK01
    MillisPWM::setBrightness(23, 255);  // C03LK02
    
    Serial.println(F("✅ 环节 072-0 启动完成 (蜡烛灯点亮)"));
}

void GameFlowManager::defineStage072_0_5() {
    Serial.print(F("📍 环节 072-0.5：准备阶段 (Level "));
    Serial.print(currentLevel);
    Serial.println(F(")"));
    
    // 停止动态效果，保持静态状态
    stopDynamicEffects();
    
    // ========================== 完整游戏状态初始化 ==========================
    resetGameState();
    
    // 激活游戏状态
    gameActive = true;
    
    // ========================== 矩阵旋转系统 ==========================
    // 生成新的随机旋转方向（确保与上次不同）
    int rotation = generateRandomRotation();
    
    // 对当前Level应用旋转
    applyRotationToLevel(currentLevel, rotation);
    
    Serial.print(F("✅ 环节 072-0.5 启动完成 (Level "));
    Serial.print(currentLevel);
    Serial.print(F(" 准备阶段，"));
    const char* rotationNames[] = {"原始", "90°", "180°", "270°"};
    Serial.print(rotationNames[rotation]);
    Serial.println(F("旋转)"));
}

/**
 * @brief 环节 072-1：第一次胜利庆祝 - 温和护眼效果
 * 设计理念：温和但有仪式感的庆祝，让玩家感受到成就感
 */
void GameFlowManager::defineStage072_1() {
    Serial.println(F("🎉 环节 072-1：第一次胜利庆祝"));
    
    // 清空之前的时刻表
    gameStage.clearStage();
    
    // 停止游戏状态
    gameActive = false;
    
    // 设置完成来源为成功
    setCompletionSource("success");
    
    Serial.println(F("  - 温和庆祝：护眼光效"));
    
    // ========================== 第一次胜利：简单庆祝效果 ==========================
    // 简单的庆祝效果，光效完成后立即熄灭所有灯
    gameStage.instant(0, -2, LED_ON);       // 开始全亮
    gameStage.instant(500, -1, LED_OFF);    // 500ms后全灭
    gameStage.instant(1000, -2, LED_ON);    // 1000ms全亮
    gameStage.instant(1500, -1, LED_OFF);   // 1500ms后全灭
    gameStage.instant(2000, -2, LED_ON);    // 2000ms全亮
    gameStage.instant(2500, -1, LED_OFF);   // 2500ms光效完成，立即熄灭所有灯
    // 然后等待到12000ms才跳转
    
    // 跳转到刷新步骤
    String nextRefreshStage = getNextRefreshStage();
    gameStage.jumpToStage(STAGE_072_1_DURATION, nextRefreshStage);
    
    // 启动时刻表
    gameStage.startStage(1);
    
    Serial.print(F("✅ 环节 072-1 启动完成 (第一次胜利庆祝，12秒后跳转"));
    Serial.print(nextRefreshStage);
    Serial.println(F(")"));
}

/**
 * @brief 环节 072-2：第二次胜利庆祝 - 适中护眼光效
 * 设计理念：比第一次稍快，但仍然护眼
 */
void GameFlowManager::defineStage072_2() {
    Serial.println(F("🌟 环节 072-2：第二次胜利庆祝"));
    
    // 清空之前的时刻表
    gameStage.clearStage();
    
    // 停止游戏状态
    gameActive = false;
    
    // 设置完成来源为成功
    setCompletionSource("success");
    
    Serial.println(F("  - 适中庆祝：护眼光效"));
    
    // ========================== 第二次胜利：适中庆祝效果 ==========================
    // 适中的庆祝效果，光效完成后立即熄灭所有灯
    gameStage.instant(0, -2, LED_ON);       // 开始全亮
    gameStage.instant(300, -1, LED_OFF);    // 300ms后全灭
    gameStage.instant(600, -2, LED_ON);     // 600ms全亮
    gameStage.instant(900, -1, LED_OFF);    // 900ms后全灭
    gameStage.instant(1200, -2, LED_ON);    // 1200ms全亮
    gameStage.instant(1500, -1, LED_OFF);   // 1500ms后全灭
    gameStage.instant(1800, -2, LED_ON);    // 1800ms全亮
    gameStage.instant(2100, -1, LED_OFF);   // 2100ms光效完成，立即熄灭所有灯
    // 然后等待到10000ms才跳转
    
    // 跳转到刷新步骤
    String nextRefreshStage = getNextRefreshStage();
    gameStage.jumpToStage(STAGE_072_2_DURATION, nextRefreshStage);
    
    // 启动时刻表
    gameStage.startStage(2);
    
    Serial.print(F("✅ 环节 072-2 启动完成 (第二次胜利庆祝，10秒后跳转"));
    Serial.print(nextRefreshStage);
    Serial.println(F(")"));
}

/**
 * @brief 环节 072-3：第三次胜利庆祝 - 绚丽但护眼
 * 设计理念：最绚丽但仍然护眼，然后跳转到最终胜利
 */
void GameFlowManager::defineStage072_3() {
    Serial.println(F("💫 环节 072-3：第三次胜利庆祝"));
    
    // 清空之前的时刻表
    gameStage.clearStage();
    
    // 停止游戏状态
    gameActive = false;
    
    // 设置完成来源为成功
    setCompletionSource("success");
    
    Serial.println(F("  - 绚丽庆祝：护眼光效"));
    
    // ========================== 第三次胜利：绚丽庆祝效果 ==========================
    // 绚丽的庆祝效果，光效完成后立即熄灭所有灯
    gameStage.instant(0, -2, LED_ON);       // 第1次爆发
    gameStage.instant(200, -1, LED_OFF);    // 200ms后全灭
    gameStage.instant(400, -2, LED_ON);     // 第2次爆发
    gameStage.instant(600, -1, LED_OFF);    // 600ms后全灭
    gameStage.instant(800, -2, LED_ON);     // 第3次爆发
    gameStage.instant(1000, -1, LED_OFF);   // 1000ms后全灭
    gameStage.instant(1200, -2, LED_ON);    // 第4次爆发
    gameStage.instant(1400, -1, LED_OFF);   // 1400ms后全灭
    gameStage.instant(1600, -2, LED_ON);    // 第5次爆发
    gameStage.instant(1800, -1, LED_OFF);   // 1800ms光效完成，立即熄灭所有灯
    // 然后等待到10000ms才跳转
    
    // 跳转到最终胜利
    gameStage.jumpToStage(STAGE_072_3_DURATION, "080-0");
    
    // 启动时刻表
    gameStage.startStage(3);
    
    Serial.println(F("✅ 环节 072-3 启动完成 (第三次胜利庆祝，10秒后跳转080-0)"));
}

void GameFlowManager::defineStage072_4() {
    Serial.println(F("📍 环节 072-4：第3关"));
    
    // 点亮第3关按键组合
    Serial.println(F("  - 第3关按键组合点亮"));
    MillisPWM::setBrightness(36, 255);  // C03IL07
    MillisPWM::setBrightness(40, 255);  // C03IL09
    MillisPWM::setBrightness(44, 255);  // C03IL11
    MillisPWM::setBrightness(48, 255);  // C03IL13
    
    Serial.println(F("✅ 环节 072-4 启动完成 (第3关)"));
}

void GameFlowManager::defineStage072_5() {
    Serial.println(F("📍 环节 072-5：迷宫副本光效1"));
    
    // 清空之前的时刻表
    gameStage.clearStage();
    
    // 首先关闭所有按键
    for (int i = 1; i <= 25; i++) {
        int pin = getButtonPin(i);
        MillisPWM::setBrightness(pin, 0);
    }
    
    Serial.println(F("  - 开始1秒轮播光效序列"));
    
    // 根据图片时间轴实现轮播序列
    // 0-200ms: 遗迹地图白色方形按键1按键亮
    gameStage.duration(0, 200, getButtonPin(1), PWM_SET, 255);
    
    // 100-300ms: 遗迹地图2、6白色方形按键1按键亮  
    gameStage.duration(100, 200, getButtonPin(2), PWM_SET, 255);
    gameStage.duration(100, 200, getButtonPin(6), PWM_SET, 255);
    
    // 200-400ms: 遗迹地图3、7、11白色方形按键1按键亮
    gameStage.duration(200, 200, getButtonPin(3), PWM_SET, 255);
    gameStage.duration(200, 200, getButtonPin(7), PWM_SET, 255);
    gameStage.duration(200, 200, getButtonPin(11), PWM_SET, 255);
    
    // 300-500ms: 遗迹地图4、8、12、16白色方形按键1按键亮
    gameStage.duration(300, 200, getButtonPin(4), PWM_SET, 255);
    gameStage.duration(300, 200, getButtonPin(8), PWM_SET, 255);
    gameStage.duration(300, 200, getButtonPin(12), PWM_SET, 255);
    gameStage.duration(300, 200, getButtonPin(16), PWM_SET, 255);
    
    // 400-600ms: 遗迹地图5、9、13、17、21白色方形按键1按键亮
    gameStage.duration(400, 200, getButtonPin(5), PWM_SET, 255);
    gameStage.duration(400, 200, getButtonPin(9), PWM_SET, 255);
    gameStage.duration(400, 200, getButtonPin(13), PWM_SET, 255);
    gameStage.duration(400, 200, getButtonPin(17), PWM_SET, 255);
    gameStage.duration(400, 200, getButtonPin(21), PWM_SET, 255);
    
    // 500-700ms: 遗迹地图10、14、18、22白色方形按键1按键亮
    gameStage.duration(500, 200, getButtonPin(10), PWM_SET, 255);
    gameStage.duration(500, 200, getButtonPin(14), PWM_SET, 255);
    gameStage.duration(500, 200, getButtonPin(18), PWM_SET, 255);
    gameStage.duration(500, 200, getButtonPin(22), PWM_SET, 255);
    
    // 600-800ms: 遗迹地图15、19、23白色方形按键1按键亮
    gameStage.duration(600, 200, getButtonPin(15), PWM_SET, 255);
    gameStage.duration(600, 200, getButtonPin(19), PWM_SET, 255);
    gameStage.duration(600, 200, getButtonPin(23), PWM_SET, 255);
    
    // 700-900ms: 遗迹地图20、24白色方形按键1按键亮
    gameStage.duration(700, 200, getButtonPin(20), PWM_SET, 255);
    gameStage.duration(700, 200, getButtonPin(24), PWM_SET, 255);
    
    // 800-1000ms: 遗迹地图25白色方形按键1按键亮
    gameStage.duration(800, 200, getButtonPin(25), PWM_SET, 255);
    
    // 1000ms: 副本大卡 (正确或错误轮播) - 所有按键关闭
    gameStage.instant(1000, -1, LED_OFF);  // 使用特殊标记关闭所有按键
    
    // 1秒后跳转到下一个目标步骤
    String targetStage = getRefreshTargetStage();
    gameStage.jumpToStage(1000, targetStage);
    
    // 启动时刻表
    gameStage.startStage(5);
    
    Serial.print(F("✅ 环节 072-5 启动完成 (迷宫副本光效1，1秒后跳转"));
    Serial.print(targetStage);
    Serial.println(F(")"));
}

void GameFlowManager::defineStage072_6() {
    Serial.println(F("📍 环节 072-6：迷宫副本光效2"));
    
    // 清空之前的时刻表
    gameStage.clearStage();
    
    // 首先关闭所有按键
    for (int i = 1; i <= 25; i++) {
        int pin = getButtonPin(i);
        MillisPWM::setBrightness(pin, 0);
    }
    
    Serial.println(F("  - 开始1秒轮播光效序列"));
    
    // 根据图片时间轴实现轮播序列（与072-5不同的组合）
    // 0-200ms: 遗迹地图白色方形按键5按键亮
    gameStage.duration(0, 200, getButtonPin(5), PWM_SET, 255);
    
    // 100-300ms: 遗迹地图4、10白色方形按键按键亮
    gameStage.duration(100, 200, getButtonPin(4), PWM_SET, 255);
    gameStage.duration(100, 200, getButtonPin(10), PWM_SET, 255);
    
    // 200-400ms: 遗迹地图3、9、6白色方形按键按键亮
    gameStage.duration(200, 200, getButtonPin(3), PWM_SET, 255);
    gameStage.duration(200, 200, getButtonPin(9), PWM_SET, 255);
    gameStage.duration(200, 200, getButtonPin(6), PWM_SET, 255);
    
    // 300-500ms: 遗迹地图2、8、14、20白色方形按键按键亮
    gameStage.duration(300, 200, getButtonPin(2), PWM_SET, 255);
    gameStage.duration(300, 200, getButtonPin(8), PWM_SET, 255);
    gameStage.duration(300, 200, getButtonPin(14), PWM_SET, 255);
    gameStage.duration(300, 200, getButtonPin(20), PWM_SET, 255);
    
    // 400-600ms: 遗迹地图1、7、13、19、25白色方形按键按键亮
    gameStage.duration(400, 200, getButtonPin(1), PWM_SET, 255);
    gameStage.duration(400, 200, getButtonPin(7), PWM_SET, 255);
    gameStage.duration(400, 200, getButtonPin(13), PWM_SET, 255);
    gameStage.duration(400, 200, getButtonPin(19), PWM_SET, 255);
    gameStage.duration(400, 200, getButtonPin(25), PWM_SET, 255);
    
    // 500-700ms: 遗迹地图6、12、18、24白色方形按键按键亮
    gameStage.duration(500, 200, getButtonPin(6), PWM_SET, 255);
    gameStage.duration(500, 200, getButtonPin(12), PWM_SET, 255);
    gameStage.duration(500, 200, getButtonPin(18), PWM_SET, 255);
    gameStage.duration(500, 200, getButtonPin(24), PWM_SET, 255);
    
    // 600-800ms: 遗迹地图11、17、23白色方形按键按键亮
    gameStage.duration(600, 200, getButtonPin(11), PWM_SET, 255);
    gameStage.duration(600, 200, getButtonPin(17), PWM_SET, 255);
    gameStage.duration(600, 200, getButtonPin(23), PWM_SET, 255);
    
    // 700-900ms: 遗迹地图16、22白色方形按键按键亮
    gameStage.duration(700, 200, getButtonPin(16), PWM_SET, 255);
    gameStage.duration(700, 200, getButtonPin(22), PWM_SET, 255);
    
    // 800-1000ms: 遗迹地图21白色方形按键按键亮
    gameStage.duration(800, 200, getButtonPin(21), PWM_SET, 255);
    
    // 1000ms: 副本大卡 (正确或错误轮播) - 所有按键关闭
    gameStage.instant(1000, -1, LED_OFF);  // 使用特殊标记关闭所有按键
    
    // 1秒后跳转到下一个目标步骤
    String targetStage = getRefreshTargetStage();
    gameStage.jumpToStage(1000, targetStage);
    
    // 启动时刻表
    gameStage.startStage(6);
    
    Serial.print(F("✅ 环节 072-6 启动完成 (迷宫副本光效2，1秒后跳转"));
    Serial.print(targetStage);
    Serial.println(F(")"));
}

void GameFlowManager::defineStage072_7() {
    Serial.println(F("📍 环节 072-7：游戏失败效果1"));
    
    // 清空之前的时刻表
    gameStage.clearStage();
    
    // 执行最后按下按键的闪烁效果
    if (lastPressedButton > 0) {
        int pin = getButtonPin(lastPressedButton);
        if (pin != -1) {
            Serial.print(F("  - 最后按键"));
            Serial.print(lastPressedButton);
            Serial.println(F("闪烁效果"));
            
            // 慢闪阶段: 亮400ms，灭400ms，循环3次
            for (int i = 0; i < ERROR_SLOW_FLASH_CYCLES; i++) {
                unsigned long cycleStart = i * (ERROR_SLOW_FLASH_ON_TIME + ERROR_SLOW_FLASH_OFF_TIME);
                gameStage.duration(cycleStart, ERROR_SLOW_FLASH_ON_TIME, pin, PWM_SET, 255);      // 亮400ms
                gameStage.duration(cycleStart + ERROR_SLOW_FLASH_ON_TIME, ERROR_SLOW_FLASH_OFF_TIME, pin, PWM_SET, 0);  // 灭400ms
            }
            
            // 快闪阶段: 亮50ms，灭50ms，循环6次
            for (int i = 0; i < ERROR_FAST_FLASH_CYCLES; i++) {
                unsigned long cycleStart = ERROR_SLOW_FLASH_END + i * (ERROR_FAST_FLASH_ON_TIME + ERROR_FAST_FLASH_OFF_TIME);
                gameStage.duration(cycleStart, ERROR_FAST_FLASH_ON_TIME, pin, PWM_SET, 255);  // 亮50ms
                gameStage.duration(cycleStart + ERROR_FAST_FLASH_ON_TIME, ERROR_FAST_FLASH_OFF_TIME, pin, PWM_SET, 0);    // 灭50ms
            }
            
            // 频闪完成，确保按键熄灭
            gameStage.instant(ERROR_FAST_FLASH_END, pin, PWM_SET, 0);
        }
    }
    
    // 跳转到指定环节
    String nextRefreshStage = getNextRefreshStage();
    gameStage.jumpToStage(STAGE_072_7_DURATION, nextRefreshStage);  // 使用宏定义的总时长
    
    // 启动时刻表
    gameStage.startStage(7);
    
    Serial.print(F("✅ 环节 072-7 启动完成 (游戏失败效果1，"));
    Serial.print(STAGE_072_7_DURATION / 1000);
    Serial.print(F("秒后跳转"));
    Serial.print(nextRefreshStage);
    Serial.println(F(")"));
}

void GameFlowManager::defineStage072_8() {
    Serial.println(F("📍 环节 072-8：游戏失败效果2"));
    
    // 清空之前的时刻表
    gameStage.clearStage();
    
    // 执行最后按下按键的闪烁效果
    if (lastPressedButton > 0) {
        int pin = getButtonPin(lastPressedButton);
        if (pin != -1) {
            Serial.print(F("  - 最后按键"));
            Serial.print(lastPressedButton);
            Serial.println(F("闪烁效果"));
            
            // 慢闪阶段: 亮400ms，灭400ms，循环3次
            for (int i = 0; i < ERROR_SLOW_FLASH_CYCLES; i++) {
                unsigned long cycleStart = i * (ERROR_SLOW_FLASH_ON_TIME + ERROR_SLOW_FLASH_OFF_TIME);
                gameStage.duration(cycleStart, ERROR_SLOW_FLASH_ON_TIME, pin, PWM_SET, 255);      // 亮400ms
                gameStage.duration(cycleStart + ERROR_SLOW_FLASH_ON_TIME, ERROR_SLOW_FLASH_OFF_TIME, pin, PWM_SET, 0);  // 灭400ms
            }
            
            // 快闪阶段: 亮50ms，灭50ms，循环6次
            for (int i = 0; i < ERROR_FAST_FLASH_CYCLES; i++) {
                unsigned long cycleStart = ERROR_SLOW_FLASH_END + i * (ERROR_FAST_FLASH_ON_TIME + ERROR_FAST_FLASH_OFF_TIME);
                gameStage.duration(cycleStart, ERROR_FAST_FLASH_ON_TIME, pin, PWM_SET, 255);  // 亮50ms
                gameStage.duration(cycleStart + ERROR_FAST_FLASH_ON_TIME, ERROR_FAST_FLASH_OFF_TIME, pin, PWM_SET, 0);    // 灭50ms
            }
            
            // 频闪完成，确保按键熄灭
            gameStage.instant(ERROR_FAST_FLASH_END, pin, PWM_SET, 0);
        }
    }
    
    // 跳转到指定环节
    String nextRefreshStage = getNextRefreshStage();
    gameStage.jumpToStage(STAGE_072_8_DURATION, nextRefreshStage);  // 使用宏定义的总时长
    
    // 启动时刻表
    gameStage.startStage(8);
    
    Serial.print(F("✅ 环节 072-8 启动完成 (游戏失败效果2，"));
    Serial.print(STAGE_072_8_DURATION / 1000);
    Serial.print(F("秒后跳转"));
    Serial.print(nextRefreshStage);
    Serial.println(F(")"));
}

void GameFlowManager::defineStage072_9() {
    Serial.println(F("📍 环节 072-9：游戏失败效果3"));
    
    // 清空之前的时刻表
    gameStage.clearStage();
    
    // 执行最后按下按键的闪烁效果
    if (lastPressedButton > 0) {
        int pin = getButtonPin(lastPressedButton);
        if (pin != -1) {
            Serial.print(F("  - 最后按键"));
            Serial.print(lastPressedButton);
            Serial.println(F("闪烁效果"));
            
            // 慢闪阶段: 亮400ms，灭400ms，循环3次
            for (int i = 0; i < ERROR_SLOW_FLASH_CYCLES; i++) {
                unsigned long cycleStart = i * (ERROR_SLOW_FLASH_ON_TIME + ERROR_SLOW_FLASH_OFF_TIME);
                gameStage.duration(cycleStart, ERROR_SLOW_FLASH_ON_TIME, pin, PWM_SET, 255);      // 亮400ms
                gameStage.duration(cycleStart + ERROR_SLOW_FLASH_ON_TIME, ERROR_SLOW_FLASH_OFF_TIME, pin, PWM_SET, 0);  // 灭400ms
            }
            
            // 快闪阶段: 亮50ms，灭50ms，循环6次
            for (int i = 0; i < ERROR_FAST_FLASH_CYCLES; i++) {
                unsigned long cycleStart = ERROR_SLOW_FLASH_END + i * (ERROR_FAST_FLASH_ON_TIME + ERROR_FAST_FLASH_OFF_TIME);
                gameStage.duration(cycleStart, ERROR_FAST_FLASH_ON_TIME, pin, PWM_SET, 255);  // 亮50ms
                gameStage.duration(cycleStart + ERROR_FAST_FLASH_ON_TIME, ERROR_FAST_FLASH_OFF_TIME, pin, PWM_SET, 0);    // 灭50ms
            }
            
            // 频闪完成，确保按键熄灭
            gameStage.instant(ERROR_FAST_FLASH_END, pin, PWM_SET, 0);
        }
    }
    
    // 跳转到指定环节
    String nextRefreshStage = getNextRefreshStage();
    gameStage.jumpToStage(STAGE_072_9_DURATION, nextRefreshStage);  // 使用宏定义的总时长
    
    // 启动时刻表
    gameStage.startStage(9);
    
    Serial.print(F("✅ 环节 072-9 启动完成 (游戏失败效果3，"));
    Serial.print(STAGE_072_9_DURATION / 1000);
    Serial.print(F("秒后跳转"));
    Serial.print(nextRefreshStage);
    Serial.println(F(")"));
}

/**
 * @brief 环节 080-0：最终胜利
 * 根据效果规格实现精确的时间轴控制
 */
void GameFlowManager::defineStage080_0() {
    Serial.println(F("🏆 环节 080-0：最终胜利！"));
    
    // 清空之前的时刻表
    gameStage.clearStage();
    
    // 停止游戏状态
    gameActive = false;
    
    Serial.println(F("  - 完整最终胜利效果 (含高频闪烁)"));
    
    // ========================== 完整最终胜利效果 (含高频闪烁) ==========================
    
    // ========================== 按照用户时刻表实现080-0效果 ==========================
    
    // 阶段1: 全场闪烁3次 (0-4800ms，按键亮800ms，灭800ms，循环3次)
    Serial.println(F("  - 阶段1: 全场闪烁3次 (0-4800ms)"));
    for (int i = 0; i < STAGE_080_0_FLASH_CYCLES; i++) {
        unsigned long flashStart = STAGE_080_0_FLASH_START + i * (STAGE_080_0_FLASH_ON_TIME + STAGE_080_0_FLASH_OFF_TIME);
        gameStage.instant(flashStart, -2, LED_ON);  // 全亮
        gameStage.instant(flashStart + STAGE_080_0_FLASH_ON_TIME, -1, LED_OFF);  // 全灭
    }
    // 闪烁结束，全部熄灭
    gameStage.instant(STAGE_080_0_FLASH_END, -1, LED_OFF);
    
    // 阶段2&3: 蜡烛灯按时刻表控制 (不是同时亮灭)
    Serial.println(F("  - 阶段2&3: 蜡烛灯按时刻表控制"));
    gameStage.instant(CANDLE_LEFT_OFF_TIME, 22, PWM_SET, 0);    // 左侧蜡烛10766ms关闭
    gameStage.instant(CANDLE_RIGHT_OFF_TIME, 23, PWM_SET, 0);   // 右侧蜡烛10766ms关闭
    gameStage.instant(CANDLE_LEFT_ON_TIME, 22, PWM_SET, 255);   // 左侧蜡烛13320ms点亮
    gameStage.instant(CANDLE_RIGHT_ON_TIME, 23, PWM_SET, 255);  // 右侧蜡烛13320ms点亮
    
    // 启动时刻表
    gameStage.startStage(80);  // 使用特殊的stage ID
    
    // 阶段4: 启动蜡烛高频闪烁状态机 (15164-19566ms)
    Serial.println(F("  - 阶段4: 启动蜡烛高频闪烁状态机"));
    strobeActive = true;
    strobeState = false;  // 开始时为熄灭状态
    strobeNextTime = millis() + CANDLE_STROBE_START;  // 15164ms后开始频闪
    strobeEndTime = millis() + CANDLE_STROBE_END;     // 19566ms后结束频闪
    
    Serial.println(F("🎉 环节 080-0 启动完成 (含高频闪烁效果)"));
    Serial.println(F("  - 总时长: ~20秒"));
    Serial.println(F("  - 全场闪烁: 3次 (800ms亮/800ms灭)"));
    Serial.println(F("  - 蜡烛控制: 按时刻表精确控制"));
    Serial.println(F("  - 蜡烛频闪: 30ms亮/30ms灭高频闪烁 (15164-19566ms)"));
}

// ========================== 环节完成通知 ==========================
void GameFlowManager::notifyStageComplete(const String& currentStep, const String& nextStep, unsigned long duration) {
    // 获取当前会话ID
    String sessionId = gameStageManager.getSessionId();
    
    if (sessionId.length() == 0) {
        Serial.println(F("⚠️ 警告: 无会话ID，无法发送完成通知"));
        return;
    }
    
    // 构建STEP_COMPLETE消息参数
    String params = "current_step=" + currentStep + ",next_step=" + nextStep + ",duration=" + String(duration);
    
    // 直接构建完整的GAME消息，避免sendGAMEResponse添加额外的result=前缀
    String msg = "$[GAME]@C302{^STEP_COMPLETE^(" + params + ")}#";
    
    Serial.print(F("发送: "));
    Serial.println(msg);
    
    // 发送消息
    harbingerClient.sendMessage(msg);
    
    Serial.print(F("📤 已发送STEP_COMPLETE: "));
    Serial.print(currentStep);
    Serial.print(F(" → "));
    Serial.print(nextStep);
    Serial.print(F(" ("));
    Serial.print(duration);
    Serial.println(F("ms)"));
}

// ========================== 工具方法 ==========================

/**
 * @brief 停止动态效果，保持静态状态
 */
void GameFlowManager::stopDynamicEffects() {
    // 停止时刻表系统
    gameStage.clearStage();
    
    Serial.println(F("🛑 停止动态效果，保持静态状态"));
}

/**
 * @brief 重置游戏状态变量
 */
void GameFlowManager::resetGameState() {
    Serial.println(F("🔄 重置所有游戏状态变量"));
    
    // 重置按键状态变量
    lastPressedButton = 0;  // 重置上一个按下的按键
    
    // 重置所有输入状态数组
    for (int i = 0; i < 25; i++) {
        buttonPressed[i] = false;      // 清除所有按键按下标记
        lastButtonState[i] = HIGH;     // 重置所有按键的上次状态为HIGH
    }
    
    // 重置引脚25状态
    pin25Triggered = false;
    lastPin25State = HIGH;
    
    // 清除完成来源标记
    lastCompletionSource = "";
    
    // 重置频闪状态机
    strobeActive = false;
    strobeState = false;
    strobeNextTime = 0;
    strobeEndTime = 0;
    
    // 矩阵旋转系统：完全不重置，保持历史记录避免重复
    // currentRotation 和 lastRotation 都保持不变
    
    Serial.println(F("✅ 游戏状态变量已完全重置"));
}

String GameFlowManager::normalizeStageId(const String& stageId) {
    String id = stageId;
    id.trim();
    
    // 移除 "stage_" 前缀
    if (id.startsWith("stage_")) {
        id = id.substring(6);
    }
    
    // 将下划线替换为横线和点号
    id.replace("_0_5", "-0.5");
    id.replace("_", "-");
    
    return id;
} 

// ========================== 遗迹地图游戏辅助函数 ==========================

/**
 * @brief 根据按键编号(1-25)获取对应的引脚号
 * @param buttonNumber 按键编号 (1-25)
 * @return 对应的引脚号
 */
int GameFlowManager::getButtonPin(int buttonNumber) {
    if (buttonNumber < 1 || buttonNumber > 25) return -1;
    
    // 使用C302配置中的引脚映射
    // 按键1-13: Pin24,26,28,30,32,34,36,38,40,42,44,46,48 (偶数引脚)
    // 按键14-16: A10,A12,A14
    // 按键17-21: Pin5,14,16,18,20
    // 按键22-25: A0,A2,A4,A8
    
    if (buttonNumber <= 13) {
        return 24 + (buttonNumber - 1) * 2;  // 24,26,28,...,48
    } else if (buttonNumber <= 16) {
        return A10 + (buttonNumber - 14) * 2;  // A10,A12,A14
    } else if (buttonNumber <= 21) {
        int pins[] = {5, 14, 16, 18, 20};
        return pins[buttonNumber - 17];
    } else {
        int pins[] = {A0, A2, A4, A8};
        return pins[buttonNumber - 22];
    }
}

/**
 * @brief 设置Level 1的初始状态
 * Level 1: 除中间一排(第13排，即按键11,12,13,14,15)灭着，其他都亮
 */
void GameFlowManager::setupLevel1() {
    Serial.println(F("  - Level 1: 除中间一排外都亮着"));
    
    for (int i = 1; i <= 25; i++) {
        int pin = getButtonPin(i);
        // 中间一排(11,12,13,14,15)灭着，其他亮着
        if (i >= 11 && i <= 15) {
            MillisPWM::setBrightness(pin, 0);    // 灭着
        } else {
            MillisPWM::setBrightness(pin, 255);  // 亮着
        }
    }
}

/**
 * @brief 设置Level 2的初始状态
 * Level 2: 只有第7个按键亮着，其他都灭
 */
void GameFlowManager::setupLevel2() {
    Serial.println(F("  - Level 2: 只有第7个按键亮着"));
    
    for (int i = 1; i <= 25; i++) {
        int pin = getButtonPin(i);
        if (i == 7) {
            MillisPWM::setBrightness(pin, 255);  // 第7个亮着
        } else {
            MillisPWM::setBrightness(pin, 0);    // 其他灭着
        }
    }
}

/**
 * @brief 设置Level 3的初始状态
 * Level 3: 只有第2,9,17,18个按键亮着，其他都灭
 */
void GameFlowManager::setupLevel3() {
    Serial.println(F("  - Level 3: 第2,9,17,18个按键亮着"));
    
    for (int i = 1; i <= 25; i++) {
        int pin = getButtonPin(i);
        if (i == 2 || i == 9 || i == 17 || i == 18) {
            MillisPWM::setBrightness(pin, 255);  // 指定按键亮着
        } else {
            MillisPWM::setBrightness(pin, 0);    // 其他灭着
        }
    }
}

/**
 * @brief 设置Level 4的初始状态
 * Level 4: 只有第2个按键亮着，其他都灭
 */
void GameFlowManager::setupLevel4() {
    Serial.println(F("  - Level 4: 只有第2个按键亮着"));
    
    for (int i = 1; i <= 25; i++) {
        int pin = getButtonPin(i);
        if (i == 2) {
            MillisPWM::setBrightness(pin, 255);  // 第2个亮着
        } else {
            MillisPWM::setBrightness(pin, 0);    // 其他灭着
        }
    }
}

/**
 * @brief 检查两个按键是否相邻（5x5网格，只考虑上下左右）
 * @param button1 按键1编号 (1-25)
 * @param button2 按键2编号 (1-25)
 * @return 是否相邻
 */
bool GameFlowManager::areButtonsAdjacent(int button1, int button2) {
    if (button1 < 1 || button1 > 25 || button2 < 1 || button2 > 25) {
        return false;
    }
    
    // 转换为5x5网格坐标 (0-4, 0-4)
    int row1 = (button1 - 1) / 5;
    int col1 = (button1 - 1) % 5;
    int row2 = (button2 - 1) / 5;
    int col2 = (button2 - 1) % 5;
    
    // 检查是否相邻（上下左右，不包括斜角）
    int rowDiff = abs(row1 - row2);
    int colDiff = abs(col1 - col2);
    
    // 相邻条件：行差+列差=1（曼哈顿距离为1）
    return (rowDiff + colDiff == 1);
}

/**
 * @brief 根据按键编号(1-25)获取对应的输入引脚号
 * @param buttonNumber 按键编号 (1-25)
 * @return 对应的输入引脚号
 */
int GameFlowManager::getButtonInputPin(int buttonNumber) {
    if (buttonNumber < 1 || buttonNumber > 25) return -1;
    
    // 使用C302配置中的按键输入引脚映射
    // 按键1-13: Pin25,27,29,31,33,35,37,39,41,43,45,47,49 (奇数引脚)
    // 按键14-16: A11,A13,A15
    // 按键17-21: Pin6,15,17,19,21
    // 按键22-25: A1,A3,A5,A9
    
    if (buttonNumber <= 13) {
        return 25 + (buttonNumber - 1) * 2;  // 25,27,29,...,49
    } else if (buttonNumber <= 16) {
        return A11 + (buttonNumber - 14) * 2;  // A11,A13,A15
    } else if (buttonNumber <= 21) {
        int pins[] = {6, 15, 17, 19, 21};
        return pins[buttonNumber - 17];
    } else {
        int pins[] = {A1, A3, A5, A9};
        return pins[buttonNumber - 22];
    }
}

/**
 * @brief 处理遗迹地图游戏按键按下事件
 * @param buttonNumber 按下的按键编号 (1-25)
 */
void GameFlowManager::handleMapButtonPress(int buttonNumber) {
    if (!gameActive) {
        Serial.println(F("⚠️ 游戏未激活，忽略按键"));
        return;
    }
    
    Serial.print(F("🎮 遗迹地图游戏 - 按键"));
    Serial.print(buttonNumber);
    Serial.println(F("被按下"));
    
    // ========================== 坐标转换系统 ==========================
    // 将物理按键转换为逻辑坐标（考虑旋转）
    int logicalButton = reverseRotateButtonNumber(buttonNumber, currentRotation);
    
    Serial.print(F("🔄 坐标转换: 物理按键"));
    Serial.print(buttonNumber);
    Serial.print(F(" → 逻辑按键"));
    Serial.println(logicalButton);
    
    // 获取按键对应的输出引脚
    int outputPin = getButtonPin(buttonNumber);
    if (outputPin == -1) {
        Serial.println(F("❌ 无效的按键编号"));
        return;
    }
    
    // 检查按键是否已经亮着（游戏失败条件）
    if (isButtonLit(buttonNumber)) {
        Serial.print(F("❌ 按键"));
        Serial.print(buttonNumber);
        Serial.println(F("已经亮着！游戏失败！"));
        handleGameError(buttonNumber);
        return;
    }
    
    // 检查相邻性（如果不是第一个按键）
    // 在逻辑坐标系中检查相邻性
    if (lastPressedButton != 0) {
        int lastLogicalButton = reverseRotateButtonNumber(lastPressedButton, currentRotation);
        if (!areButtonsAdjacent(lastLogicalButton, logicalButton)) {
            Serial.print(F("❌ 按键"));
            Serial.print(buttonNumber);
            Serial.print(F("(逻辑"));
            Serial.print(logicalButton);
            Serial.print(F(")与上一个按键"));
            Serial.print(lastPressedButton);
            Serial.print(F("(逻辑"));
            Serial.print(lastLogicalButton);
            Serial.println(F(")不相邻！游戏失败！"));
            handleGameError(buttonNumber);
            return;
        }
    }
    
    // 合法移动：点亮按键
    MillisPWM::setBrightness(outputPin, 255);
    lastPressedButton = buttonNumber;  // 记录物理按键编号
    
    Serial.print(F("✅ 按键"));
    Serial.print(buttonNumber);
    Serial.print(F("已点亮 (引脚"));
    Serial.print(outputPin);
    Serial.println(F(")"));
    
    // 检查游戏是否完成（所有按键都亮了）
    if (checkGameComplete()) {
        Serial.println(F("🎉 恭喜！遗迹地图游戏完成！"));
                 handleGameComplete();
     }
}

/**
 * @brief 检查按键是否已经亮着
 * @param buttonNumber 按键编号 (1-25)
 * @return 是否已经亮着
 */
bool GameFlowManager::isButtonLit(int buttonNumber) {
    int outputPin = getButtonPin(buttonNumber);
    if (outputPin == -1) return false;
    
    // 通过PWM系统检查亮度是否大于0
    uint8_t brightness = MillisPWM::getBrightness(outputPin);
    return brightness > 0;
}

/**
 * @brief 处理游戏错误
 * @param failedButton 失败的按键编号
 */
void GameFlowManager::handleGameError(int failedButton) {
    gameActive = false;  // 停止游戏
    lastPressedButton = failedButton;  // 记录最后按下的按键
    
    Serial.print(F("❌ 遗迹地图游戏失败！按键 "));
    Serial.println(failedButton);
    
    // 设置完成来源为错误
    setCompletionSource("error");
    
    // 错误计数递增
    errorCount++;
    
    // 根据错误次数选择错误步骤
    String errorStep;
    switch (errorCount % 3) {
        case 1:
            errorStep = buildStageId("7");  // 072-7
            break;
        case 2:
            errorStep = buildStageId("8");  // 072-8
            break;
        case 0:  // errorCount % 3 == 0
            errorStep = buildStageId("9");  // 072-9
            break;
    }
    
    Serial.print(F("📤 游戏失败 → 错误步骤: "));
    Serial.print(errorStep);
    Serial.print(F(" (错误次数: "));
    Serial.print(errorCount);
    Serial.println(F(")"));
    
    // 发送游戏失败通知给服务器，先跳转到错误步骤
    unsigned long duration = getStageElapsedTime();
    notifyStageComplete(buildStageId("0.5"), errorStep, duration);
    
    Serial.println(F("✅ 游戏失败通知已发送"));
}

/**
 * @brief 检查游戏是否完成（所有按键都亮了）
 * @return 是否完成
 */
bool GameFlowManager::checkGameComplete() {
    for (int i = 1; i <= 25; i++) {
        if (!isButtonLit(i)) {
            return false;  // 还有按键没亮
        }
    }
    return true;  // 所有按键都亮了
}

/**
 * @brief 处理游戏完成
 */
void GameFlowManager::handleGameComplete() {
    gameActive = false;  // 停止游戏
    
    Serial.println(F("🎊 遗迹地图游戏胜利！"));
    
    // 设置完成来源为成功
    setCompletionSource("success");
    
    // 增加成功计数
    successCount++;
    Serial.print(F("🏆 成功次数: "));
    Serial.print(successCount);
    Serial.println(F("/3"));
    
    // 检查是否达到最终胜利条件（3次成功）
    if (successCount >= 3) {
        Serial.println(F("🎉 达到3次成功！先跳转到072-3庆祝！"));
        
        // 发送游戏完成通知给服务器，先跳转到072-3庆祝
        unsigned long duration = getStageElapsedTime();
        notifyStageComplete(buildStageId("0.5"), buildStageId("3"), duration);
        
        Serial.println(F("✅ 072-3庆祝跳转通知已发送"));
        return;
    }
    
    // 未达到3次成功，继续正常流程
    // 获取成功后的下一个步骤 (072-1/2/3)
    String successStep = getNextSuccessStage();
    
    Serial.print(F("📤 游戏完成 → 成功步骤: "));
    Serial.println(successStep);
    
    // 发送游戏完成通知给服务器，跳转到成功步骤
    unsigned long duration = getStageElapsedTime();
    notifyStageComplete(buildStageId("0.5"), successStep, duration);
    
    Serial.println(F("✅ 游戏完成通知已发送"));
}

// ========================== 刷新步骤循环管理 ==========================

/**
 * @brief 获取下一个刷新步骤(-5或-6)
 * @return 下一个刷新步骤的完整ID
 */
String GameFlowManager::getNextRefreshStage() {
    String nextStage;
    
    if (lastRefreshWas5) {
        // 上次是-5，这次应该是-6
        nextStage = buildStageId("6");
    } else {
        // 上次是-6或初始状态，这次应该是-5
        nextStage = buildStageId("5");
    }
    
    Serial.print(F("🔄 下一个刷新步骤: "));
    Serial.print(nextStage);
    Serial.print(F(" (上次是"));
    Serial.print(lastRefreshWas5 ? "-5" : "-6");
    Serial.println(F(")"));
    
    return nextStage;
}

/**
 * @brief 记录执行的刷新步骤
 * @param stageId 执行的环节ID
 */
void GameFlowManager::recordRefreshStage(const String& stageId) {
    String normalizedId = normalizeStageId(stageId);
    
    if (normalizedId.endsWith("-5")) {
        lastRefreshWas5 = true;
        Serial.println(F("📝 记录刷新步骤: -5"));
    } else if (normalizedId.endsWith("-6")) {
        lastRefreshWas5 = false;
        Serial.println(F("📝 记录刷新步骤: -6"));
    }
}

/**
 * @brief 重置刷新循环（从-5开始）
 */
void GameFlowManager::resetRefreshCycle() {
    lastRefreshWas5 = false;  // 下次从-5开始
    Serial.println(F("🔄 重置刷新循环，下次从-5开始"));
}

// ========================== Level管理系统 ==========================

/**
 * @brief 获取当前Level
 */
int GameFlowManager::getCurrentLevel() const {
    return currentLevel;
}

/**
 * @brief 设置当前Level
 */
void GameFlowManager::setCurrentLevel(int level) {
    if (level >= 1 && level <= 4) {
        currentLevel = level;
        Serial.print(F("🎯 设置当前Level: "));
        Serial.println(level);
    } else {
        Serial.print(F("❌ 无效的Level: "));
        Serial.println(level);
    }
}

/**
 * @brief 进入下一个Level (1→2→4→3→4→3...)
 */
void GameFlowManager::advanceToNextLevel() {
    switch (currentLevel) {
        case 1:
            currentLevel = 2;
            Serial.println(F("🎯 Level 1 → Level 2"));
            break;
        case 2:
            currentLevel = 4;
            Serial.println(F("🎯 Level 2 → Level 4"));
            break;
        case 4:
            currentLevel = 3;
            Serial.println(F("🎯 Level 4 → Level 3"));
            break;
        case 3:
            currentLevel = 4;
            Serial.println(F("🎯 Level 3 → Level 4 (开始4-3循环)"));
            break;
        default:
            currentLevel = 1;
            Serial.println(F("🎯 异常情况，重置到Level 1"));
            break;
    }
}

/**
 * @brief 获取成功后的下一个步骤
 */
String GameFlowManager::getNextSuccessStage() {
    switch (currentLevel) {
        case 1:
            return buildStageId("1");  // 072-1
        case 2:
            return buildStageId("2");  // 072-2
        case 3:
            return buildStageId("3");  // 072-3
        case 4:
            return buildStageId("3");  // 072-3 (Level 4也使用072-3庆祝)
        default:
            return buildStageId("1");  // 默认返回072-1
    }
}

/**
 * @brief 获取刷新后的目标步骤
 */
String GameFlowManager::getRefreshTargetStage() {
    if (lastCompletionSource == "error") {
        // 错误后保持当前Level
        keepCurrentLevel();
        return buildStageId("0.5");  // 回到当前Level的072-0.5
    } else if (lastCompletionSource == "success") {
        // 成功后进入下一个Level
        advanceToNextLevel();
        return buildStageId("0.5");  // 进入下一个Level的072-0.5
    } else {
        // 默认情况，回到当前Level的072-0.5
        return buildStageId("0.5");
    }
}

/**
 * @brief 设置完成来源
 */
void GameFlowManager::setCompletionSource(const String& source) {
    lastCompletionSource = source;
    Serial.print(F("📝 设置完成来源: "));
    Serial.println(source);
}

/**
 * @brief 保持当前Level (错误时使用)
 * 特殊规则：Level 1和2错误时重复同样Level，Level 3和4错误时相互切换
 */
void GameFlowManager::keepCurrentLevel() {
    switch (currentLevel) {
        case 1:
            // Level 1错误 → 保持Level 1
            Serial.println(F("🔄 Level 1错误 → 保持Level 1"));
            break;
        case 2:
            // Level 2错误 → 保持Level 2
            Serial.println(F("🔄 Level 2错误 → 保持Level 2"));
            break;
        case 3:
            // Level 3错误 → 切换到Level 4
            currentLevel = 4;
            Serial.println(F("🔄 Level 3错误 → 切换到Level 4"));
            break;
        case 4:
            // Level 4错误 → 切换到Level 3
            currentLevel = 3;
            Serial.println(F("🔄 Level 4错误 → 切换到Level 3"));
            break;
        default:
            Serial.print(F("🔄 异常Level("));
            Serial.print(currentLevel);
            Serial.println(F(")错误 → 重置到Level 1"));
            currentLevel = 1;
            break;
    }
}

// ========================== 矩阵旋转系统 ==========================

/**
 * @brief 生成不重复的随机旋转方向
 * @return 旋转方向 (0=原始, 1=90°, 2=180°, 3=270°)
 */
int GameFlowManager::generateRandomRotation() {
    int newRotation;
    
    // 旋转方向名称数组（只声明一次）
    const char* rotationNames[] = {"原始", "90°", "180°", "270°"};
    
    Serial.print(F("🎲 旋转选择: 上次旋转="));
    if (lastRotation == -1) {
        Serial.print(F("无(首次)"));
    } else {
        Serial.print(rotationNames[lastRotation]);
    }
    
    // 从其他3个方向中随机选择（避免与上次旋转相同）
    if (lastRotation == -1) {
        // 如果是首次选择，可以选择任意方向
        newRotation = random(0, 4);  // 0, 1, 2, 3
        Serial.print(F(", 首次可选任意方向"));
    } else {
        // 从其他3个方向中选择（避免与lastRotation相同）
        int availableRotations[3];
        int count = 0;
        for (int i = 0; i < 4; i++) {
            if (i != lastRotation) {
                availableRotations[count++] = i;
            }
        }
        newRotation = availableRotations[random(0, 3)];
        Serial.print(F(", 从其他3个方向中选择"));
    }
    
    // 更新历史记录
    lastRotation = currentRotation;  // 保存当前旋转为历史
    currentRotation = newRotation;   // 设置新的当前旋转
    
    // 打印旋转信息
    Serial.print(F(" → 选中: "));
    Serial.println(rotationNames[newRotation]);
    
    return newRotation;
}

/**
 * @brief 根据旋转方向转换按键编号
 * @param originalButton 原始按键编号 (1-25)
 * @param rotation 旋转方向 (0=原始, 1=90°, 2=180°, 3=270°)
 * @return 旋转后的按键编号 (1-25)
 */
int GameFlowManager::rotateButtonNumber(int originalButton, int rotation) {
    if (originalButton < 1 || originalButton > 25) {
        return originalButton;  // 无效按键编号，直接返回
    }
    
    // 转换为0-24的索引
    int index = originalButton - 1;
    
    // 转换为5x5矩阵坐标 (row, col)
    int row = index / 5;
    int col = index % 5;
    
    int newRow, newCol;
    
    // 根据旋转方向计算新坐标
    switch (rotation) {
        case 0:  // 原始 (0°)
            newRow = row;
            newCol = col;
            break;
        case 1:  // 90° 顺时针旋转
            newRow = col;
            newCol = 4 - row;
            break;
        case 2:  // 180° 旋转
            newRow = 4 - row;
            newCol = 4 - col;
            break;
        case 3:  // 270° 顺时针旋转 (或90°逆时针)
            newRow = 4 - col;
            newCol = row;
            break;
        default:
            newRow = row;
            newCol = col;
            break;
    }
    
    // 转换回按键编号 (1-25)
    return newRow * 5 + newCol + 1;
}

/**
 * @brief 反向旋转：从旋转后坐标获取原始坐标
 * @param rotatedButton 旋转后的按键编号 (1-25)
 * @param rotation 旋转方向 (0=原始, 1=90°, 2=180°, 3=270°)
 * @return 原始按键编号 (1-25)
 */
int GameFlowManager::reverseRotateButtonNumber(int rotatedButton, int rotation) {
    if (rotatedButton < 1 || rotatedButton > 25) {
        return rotatedButton;  // 无效按键编号，直接返回
    }
    
    // 反向旋转：使用相反的旋转方向
    int reverseRotation;
    switch (rotation) {
        case 0:  // 原始 → 原始
            reverseRotation = 0;
            break;
        case 1:  // 90° → 270°
            reverseRotation = 3;
            break;
        case 2:  // 180° → 180°
            reverseRotation = 2;
            break;
        case 3:  // 270° → 90°
            reverseRotation = 1;
            break;
        default:
            reverseRotation = 0;
            break;
    }
    
    return rotateButtonNumber(rotatedButton, reverseRotation);
}

/**
 * @brief 对指定Level应用旋转
 * @param level Level编号 (1-4)
 * @param rotation 旋转方向 (0=原始, 1=90°, 2=180°, 3=270°)
 */
void GameFlowManager::applyRotationToLevel(int level, int rotation) {
    Serial.print(F("🎯 对Level "));
    Serial.print(level);
    Serial.print(F(" 应用"));
    const char* rotationNames[] = {"原始", "90°", "180°", "270°"};
    Serial.print(rotationNames[rotation]);
    Serial.println(F("旋转"));
    
    // 先关闭所有LED
    for (int i = 1; i <= 25; i++) {
        int pin = getButtonPin(i);
        MillisPWM::setBrightness(pin, 0);
    }
    
    // 根据Level获取原始亮灯模式，然后应用旋转
    switch (level) {
        case 1: {
            // Level 1原始: 除中间一排(11,12,13,14,15)外都亮
            for (int originalButton = 1; originalButton <= 25; originalButton++) {
                // 检查原始按键是否应该亮着
                bool shouldLight = !(originalButton >= 11 && originalButton <= 15);
                
                if (shouldLight) {
                    // 计算旋转后的按键位置
                    int rotatedButton = rotateButtonNumber(originalButton, rotation);
                    int pin = getButtonPin(rotatedButton);
                    MillisPWM::setBrightness(pin, 255);
                }
            }
            break;
        }
        case 2: {
            // Level 2原始: 只有第7个按键亮着
            int originalButton = 7;
            int rotatedButton = rotateButtonNumber(originalButton, rotation);
            int pin = getButtonPin(rotatedButton);
            MillisPWM::setBrightness(pin, 255);
            break;
        }
        case 3: {
            // Level 3原始: 第2,9,17,18个按键亮着
            int originalButtons[] = {2, 9, 17, 18};
            for (int i = 0; i < 4; i++) {
                int rotatedButton = rotateButtonNumber(originalButtons[i], rotation);
                int pin = getButtonPin(rotatedButton);
                MillisPWM::setBrightness(pin, 255);
            }
            break;
        }
        case 4: {
            // Level 4原始: 只有第2个按键亮着
            int originalButton = 2;
            int rotatedButton = rotateButtonNumber(originalButton, rotation);
            int pin = getButtonPin(rotatedButton);
            MillisPWM::setBrightness(pin, 255);
            break;
        }
        default:
            Serial.println(F("❌ 无效的Level"));
            break;
    }
    
    Serial.println(F("✅ 旋转应用完成"));
} 