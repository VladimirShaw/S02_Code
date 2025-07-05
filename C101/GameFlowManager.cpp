/**
 * =============================================================================
 * GameFlowManager - C101音频控制器游戏流程管理器 - 实现文件
 * 版本: 2.0 - C101专用版本
 * 创建日期: 2025-01-03
 * =============================================================================
 */

#include "GameFlowManager.h"
#include "UniversalHarbingerClient.h"
#include "BY_VoiceController_Unified.h"
#include "C101_SimpleConfig.h"  // 添加配置文件引用
#include "MillisPWM.h"          // 添加PWM呼吸灯控制
#include <string.h>  // for memset

// 外部全局实例
extern UniversalHarbingerClient harbingerClient;
extern BY_VoiceController_Unified voice;

// 全局实例
GameFlowManager gameFlowManager;

// ========================== 统一引脚状态管理器实现 ==========================

// 全局引脚管理器实例
UnifiedPinManager pinManager;

// 注册需要管理的引脚
void UnifiedPinManager::registerPin(int pin, bool initialState) {
    if (managedPinCount >= MAX_MANAGED_PINS) {
        Serial.println(F("❌ 引脚管理器已满，无法注册更多引脚"));
        return;
    }
    
    // 检查是否已经注册过
    if (findPinIndex(pin) >= 0) {
        Serial.print(F("⚠️ 引脚"));
        Serial.print(pin);
        Serial.println(F("已经注册过"));
        return;
    }
    
    // 注册新引脚
    managedPins[managedPinCount].pin = pin;
    managedPins[managedPinCount].desiredState = initialState;
    managedPins[managedPinCount].currentState = initialState;
    managedPins[managedPinCount].changeTime = millis();
    managedPins[managedPinCount].duration = 0;
    managedPins[managedPinCount].needsUpdate = true;
    
    pinMode(pin, OUTPUT);
    digitalWrite(pin, initialState);
    
    Serial.print(F("✅ 注册引脚"));
    Serial.print(pin);
    Serial.print(F("，初始状态："));
    Serial.println(initialState ? F("HIGH") : F("LOW"));
    
    managedPinCount++;
}

// 设置引脚状态（立即生效）
void UnifiedPinManager::setPinState(int pin, bool state) {
    int index = findPinIndex(pin);
    if (index < 0) {
        // 静默处理未注册的引脚，避免输出错误信息
        return;
    }
    
    managedPins[index].desiredState = state;
    managedPins[index].changeTime = millis();
    managedPins[index].duration = 0;  // 永久状态
    managedPins[index].needsUpdate = true;
}

// 设置引脚临时状态（指定时间后自动恢复）
void UnifiedPinManager::setPinTemporaryState(int pin, bool tempState, unsigned long duration, bool restoreState) {
    int index = findPinIndex(pin);
    if (index < 0) {
        // 静默处理未注册的引脚，避免输出错误信息
        return;
    }
    
    managedPins[index].desiredState = tempState;
    managedPins[index].changeTime = millis();
    managedPins[index].duration = duration;
    managedPins[index].needsUpdate = true;
    
    // 临时保存恢复状态（简化实现，直接在duration到期时设为restoreState）
    // 这里可以扩展为更复杂的状态管理
}

// 检查引脚是否被PWM控制（避免冲突）
bool UnifiedPinManager::isPinPWMControlled(int pin) {
    // 检查MillisPWM系统是否正在控制这个引脚
    // 这里需要与MillisPWM系统集成，暂时返回false
    return false;  // 简化实现
}

// 统一更新所有引脚状态
void UnifiedPinManager::updateAllPins() {
    for (int i = 0; i < managedPinCount; i++) {
        updateSinglePin(i);
    }
}

// 获取引脚当前状态
bool UnifiedPinManager::getPinState(int pin) {
    int index = findPinIndex(pin);
    if (index < 0) {
        return digitalRead(pin);  // 如果未注册，直接读取硬件状态
    }
    return managedPins[index].currentState;
}

// 调试：打印所有引脚状态
void UnifiedPinManager::printPinStates() {
    Serial.println(F("=== 引脚状态管理器 ==="));
    for (int i = 0; i < managedPinCount; i++) {
        Serial.print(F("引脚"));
        Serial.print(managedPins[i].pin);
        Serial.print(F(": 期望="));
        Serial.print(managedPins[i].desiredState ? F("HIGH") : F("LOW"));
        Serial.print(F(", 当前="));
        Serial.print(managedPins[i].currentState ? F("HIGH") : F("LOW"));
        Serial.print(F(", 需要更新="));
        Serial.println(managedPins[i].needsUpdate ? F("是") : F("否"));
    }
}

// 查找引脚索引
int UnifiedPinManager::findPinIndex(int pin) {
    for (int i = 0; i < managedPinCount; i++) {
        if (managedPins[i].pin == pin) {
            return i;
        }
    }
    return -1;  // 未找到
}

// 实际更新单个引脚
void UnifiedPinManager::updateSinglePin(int index) {
    if (index < 0 || index >= managedPinCount) return;
    
    VoiceIOState& pinState = managedPins[index];
    
    // 检查是否被PWM控制
    if (isPinPWMControlled(pinState.pin)) {
        return;  // 跳过PWM控制的引脚
    }
    
    // 检查临时状态是否到期
    if (pinState.duration > 0 && millis() - pinState.changeTime >= pinState.duration) {
        // 临时状态到期，恢复到HIGH（默认恢复状态）
        pinState.desiredState = HIGH;
        pinState.duration = 0;
        pinState.needsUpdate = true;
    }
    
    // 更新硬件状态
    if (pinState.needsUpdate && pinState.desiredState != pinState.currentState) {
        digitalWrite(pinState.pin, pinState.desiredState);
        pinState.currentState = pinState.desiredState;
        pinState.needsUpdate = false;
        
        // 移除引脚更新信息输出，减少串口输出
    }
}

// ========================== 构造和初始化 ==========================
GameFlowManager::GameFlowManager() {
    // 初始化并行环节数组
    activeStageCount = 0;
    globalStopped = false;
    
    // 初始化兼容性变量
    currentStageId = "";
    stageStartTime = 0;
    stageRunning = false;
    jumpRequested = false;
    
    // 初始化紧急开门变量
    emergencyUnlockStartTime = 0;
    emergencyUnlockActive = false;
    lastCardReaderState = HIGH;  // 读卡器默认为高电平
    
    // 初始化所有环节槽位
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        stages[i] = StageState();
    }
}

bool GameFlowManager::begin() {
    Serial.println(F("🎮 GameFlowManager初始化开始"));
    
    // 初始化所有环节状态
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        stages[i].running = false;
        stages[i].stageId = "";
        stages[i].startTime = 0;
        stages[i].jumpRequested = false;
        memset(&stages[i].state, 0, sizeof(stages[i].state));
    }
    
    activeStageCount = 0;
    globalStopped = false;
    
    // 初始化兼容旧接口的变量
    currentStageId = "";
    stageStartTime = 0;
    stageRunning = false;
    jumpRequested = false;
    
    // 初始化紧急开门功能
    emergencyUnlockStartTime = 0;
    emergencyUnlockActive = false;
    
    // 初始化引脚管理器
    Serial.println(F("🔧 初始化统一引脚管理器..."));
    
    // 注册需要管理的引脚
    // 语音IO引脚
    for (int i = 0; i < C101_AUDIO_MODULE_COUNT; i++) {
        pinManager.registerPin(C101_AUDIO_IO1_PINS[i], HIGH);
        pinManager.registerPin(C101_AUDIO_IO2_PINS[i], HIGH);
    }
    
    // 画灯引脚
    for (int i = 0; i < C101_PAINTING_LIGHT_COUNT; i++) {
        pinManager.registerPin(C101_PAINTING_LIGHT_PINS[i], LOW);
    }
    
    // 按键灯引脚
    for (int i = 0; i < C101_TAUNT_BUTTON_COUNT; i++) {
        pinManager.registerPin(C101_TAUNT_BUTTON_LIGHT_PINS[i], LOW);
    }
    
    // 植物灯引脚
    for (int i = 0; i < C101_PLANT_LIGHT_COUNT; i++) {
        pinManager.registerPin(C101_PLANT_LIGHT_PINS[i], LOW);
    }
    
    // 其他控制引脚
    pinManager.registerPin(C101_DOOR_LOCK_PIN, HIGH);
    pinManager.registerPin(C101_DOOR_LIGHT_PIN, LOW);
    pinManager.registerPin(C101_AMBIENT_LIGHT_PIN, LOW);
    pinManager.registerPin(C101_HINT_LED_PINS[0], LOW);
    pinManager.registerPin(C101_HINT_LED_PINS[1], LOW);
    pinManager.registerPin(C101_BUTTERFLY_CARD_RELAY_PIN, LOW);
    pinManager.registerPin(C101_BUTTERFLY_LIGHT_PIN, HIGH);
    pinManager.registerPin(C101_AD_FAN_PIN, LOW);
    
    Serial.println(F("✅ 统一引脚管理器初始化完成"));
    
    // 打印PWM通道状态
    MillisPWM::printChannelStatus();
    
    Serial.println(F("✅ GameFlowManager初始化完成"));
    return true;
}

// ========================== 私有辅助方法 ==========================
int GameFlowManager::findStageIndex(const String& stageId) {
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        if (stages[i].running && stages[i].stageId == stageId) {
            return i;
        }
    }
    return -1;
}

int GameFlowManager::findEmptySlot() {
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        if (!stages[i].running) {
            return i;
        }
    }
    return -1;
}

void GameFlowManager::updateCompatibilityVars() {
    // 更新兼容性变量，使其指向第一个活跃环节
    stageRunning = (activeStageCount > 0);
    
    if (activeStageCount > 0) {
        // 找到第一个运行中的环节
        for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
            if (stages[i].running) {
                currentStageId = stages[i].stageId;
                stageStartTime = stages[i].startTime;
                jumpRequested = stages[i].jumpRequested;
                break;
            }
        }
    } else {
        currentStageId = "";
        stageStartTime = 0;
        jumpRequested = false;
    }
}

// ========================== 环节控制 ==========================
bool GameFlowManager::startStage(const String& stageId) {
    // 标准化环节ID
    String normalizedId = normalizeStageId(stageId);
    
    // 检查环节是否已经在运行
    if (findStageIndex(normalizedId) >= 0) {
        Serial.print(F("⚠️ 环节已在运行: "));
        Serial.println(normalizedId);
        return false;
    }
    
    // 查找空闲槽位
    int slot = findEmptySlot();
    if (slot < 0) {
        Serial.print(F("❌ 无可用槽位，已达最大并行数: "));
        Serial.println(MAX_PARALLEL_STAGES);
        return false;
    }
    
    Serial.print(F("=== 启动C101音频环节[槽位"));
    Serial.print(slot);
    Serial.print(F("]: "));
    Serial.print(stageId);
    if (normalizedId != stageId) {
        Serial.print(F(" (标准化为: "));
        Serial.print(normalizedId);
        Serial.print(F(")"));
    }
    Serial.println(F(" ==="));
    
    // 重置全局停止标志
    globalStopped = false;
    
    // 初始化环节状态
    stages[slot].stageId = normalizedId;
    stages[slot].startTime = millis();
    stages[slot].running = true;
    stages[slot].jumpRequested = false;
    memset(&stages[slot].state, 0, sizeof(stages[slot].state));
    
    // 根据环节ID执行对应逻辑
    if (normalizedId == "000_0") {
        Serial.println(F("🌟 ===== C101序章初始化效果启动 ====="));
        Serial.println(F("💡 环节000_0：植物灯顺序呼吸效果（C101专用，无音频）"));
        
        Serial.println(F("💡 植物灯顺序呼吸效果：每个灯持续1500ms，循环切换"));
        Serial.println(F("   灯1(0ms) -> 灯3(1500ms) -> 灯2(3000ms) -> 灯4(4500ms) -> 循环"));
        Serial.println(F("🚨 紧急开门功能激活"));
        
        // ========================== 应用000_0环节引脚状态配置 ==========================
        Serial.println(F("🔧 应用000_0环节引脚状态配置..."));
        
        // 入口门系统
        pinManager.setPinState(C101_DOOR_LOCK_PIN, STAGE_000_0_DOOR_LOCK_STATE);
        pinManager.setPinState(C101_DOOR_LIGHT_PIN, STAGE_000_0_DOOR_LIGHT_STATE);
        
        // 氛围射灯系统
        pinManager.setPinState(C101_AMBIENT_LIGHT_PIN, STAGE_000_0_AMBIENT_LIGHT_STATE);
        
        // 嘲讽按键灯光系统
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[0], STAGE_000_0_TAUNT_BUTTON1_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[1], STAGE_000_0_TAUNT_BUTTON2_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[2], STAGE_000_0_TAUNT_BUTTON3_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[3], STAGE_000_0_TAUNT_BUTTON4_STATE);
        
        // 画灯谜题系统
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[0], STAGE_000_0_PAINTING_LIGHT1_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[1], STAGE_000_0_PAINTING_LIGHT2_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[2], STAGE_000_0_PAINTING_LIGHT3_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[3], STAGE_000_0_PAINTING_LIGHT4_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[4], STAGE_000_0_PAINTING_LIGHT5_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[5], STAGE_000_0_PAINTING_LIGHT6_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[6], STAGE_000_0_PAINTING_LIGHT7_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[7], STAGE_000_0_PAINTING_LIGHT8_STATE);
        
        // 提示灯带系统
        pinManager.setPinState(C101_HINT_LED_PINS[0], STAGE_000_0_HINT_LED1_STATE);
        pinManager.setPinState(C101_HINT_LED_PINS[1], STAGE_000_0_HINT_LED2_STATE);
        
        // 蝴蝶灯谜题系统
        pinManager.setPinState(C101_BUTTERFLY_CARD_RELAY_PIN, STAGE_000_0_BUTTERFLY_CARD_STATE);
        pinManager.setPinState(C101_BUTTERFLY_LIGHT_PIN, STAGE_000_0_BUTTERFLY_LIGHT_STATE);
        pinManager.setPinState(C101_AD_FAN_PIN, STAGE_000_0_AD_FAN_STATE);
        
        Serial.println(F("✅ 000_0环节引脚状态配置完成"));
        
        // 初始化环节特定状态（C101无音频，只有灯光控制）
        stages[slot].state.stage000.channelStarted = false;  // 保留字段但不使用
        stages[slot].state.stage000.lastCheckTime = 0;       // 保留字段但不使用
        stages[slot].state.stage000.currentLightIndex = -1;  // 修复：初始化为-1，确保第一次切换正确
        stages[slot].state.stage000.lightCycleStartTime = 0;
        stages[slot].state.stage000.lightEffectStarted = false;
        
        Serial.println(F("⏳ 等待植物灯效果启动..."));
        activeStageCount++;
        updateCompatibilityVars();
        return true;
    } else if (normalizedId == "001_1") {
        Serial.println(F("🎮 ===== 游戏开始环节启动 ====="));
        Serial.println(F("🔍 环节001_1：干簧管检测环节（C101专用，无音频）"));
        Serial.print(F("🔍 等待Pin"));
        Serial.print(STAGE_001_1_REED_PIN);
        Serial.println(F("干簧管触发"));
        Serial.println(F("🌱 植物灯继续000_0的呼吸效果"));
        
        // ========================== 应用001_1环节引脚状态配置 ==========================
        Serial.println(F("🔧 应用001_1环节引脚状态配置..."));
        
        // 入口门系统
        pinManager.setPinState(C101_DOOR_LOCK_PIN, STAGE_001_1_DOOR_LOCK_STATE);
        pinManager.setPinState(C101_DOOR_LIGHT_PIN, STAGE_001_1_DOOR_LIGHT_STATE);
        
        // 氛围射灯系统
        pinManager.setPinState(C101_AMBIENT_LIGHT_PIN, STAGE_001_1_AMBIENT_LIGHT_STATE);
        
        // 嘲讽按键灯光系统
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[0], STAGE_001_1_TAUNT_BUTTON1_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[1], STAGE_001_1_TAUNT_BUTTON2_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[2], STAGE_001_1_TAUNT_BUTTON3_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[3], STAGE_001_1_TAUNT_BUTTON4_STATE);
        
        // 画灯谜题系统
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[0], STAGE_001_1_PAINTING_LIGHT1_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[1], STAGE_001_1_PAINTING_LIGHT2_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[2], STAGE_001_1_PAINTING_LIGHT3_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[3], STAGE_001_1_PAINTING_LIGHT4_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[4], STAGE_001_1_PAINTING_LIGHT5_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[5], STAGE_001_1_PAINTING_LIGHT6_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[6], STAGE_001_1_PAINTING_LIGHT7_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[7], STAGE_001_1_PAINTING_LIGHT8_STATE);
        
        // 提示灯带系统
        pinManager.setPinState(C101_HINT_LED_PINS[0], STAGE_001_1_HINT_LED1_STATE);
        pinManager.setPinState(C101_HINT_LED_PINS[1], STAGE_001_1_HINT_LED2_STATE);
        
        // 蝴蝶灯谜题系统
        pinManager.setPinState(C101_BUTTERFLY_CARD_RELAY_PIN, STAGE_001_1_BUTTERFLY_CARD_STATE);
        pinManager.setPinState(C101_BUTTERFLY_LIGHT_PIN, STAGE_001_1_BUTTERFLY_LIGHT_STATE);
        pinManager.setPinState(C101_AD_FAN_PIN, STAGE_001_1_AD_FAN_STATE);
        
        Serial.println(F("✅ 001_1环节引脚状态配置完成"));
        
        // 🌱 重要：确保植物灯继续000_0的呼吸效果
        // 检查000_0环节是否还在运行，如果是，则继承其植物灯状态
        int stage000Index = findStageIndex("000_0");
        if (stage000Index >= 0 && stages[stage000Index].running) {
            Serial.println(F("🌱 检测到000_0环节仍在运行，继承植物灯状态"));
            // 继承000_0环节的当前植物灯索引
            stages[slot].state.stage001_1.lastLightIndex = stages[stage000Index].state.stage000.currentLightIndex;
            Serial.print(F("🌱 继承植物灯索引: "));
            Serial.println(stages[slot].state.stage001_1.lastLightIndex);
            
            // 停止000_0环节，由001_1接管植物灯控制
            Serial.println(F("🌱 停止000_0环节，由001_1接管植物灯控制"));
            stages[stage000Index].running = false;
            activeStageCount--;
        } else {
            Serial.println(F("🌱 000_0环节已停止，启动植物灯呼吸效果"));
            // 如果000_0已停止，则重新启动植物灯呼吸效果
            MillisPWM::startBreathing(C101_PLANT_LIGHT_PINS[0], 3.0);  // 从灯1开始
            stages[slot].state.stage001_1.lastLightIndex = 0;
        }
        
        // 初始化干簧管检测引脚
        pinMode(STAGE_001_1_REED_PIN, INPUT_PULLUP);
        Serial.print(F("🔍 初始化干簧管检测引脚"));
        Serial.print(STAGE_001_1_REED_PIN);
        Serial.println(F("为INPUT_PULLUP模式"));
        
        // 初始化环节特定状态（C101无音频，只有干簧管检测）
        stages[slot].state.stage001_1.channelStarted = false;  // 保留字段但不使用
        stages[slot].state.stage001_1.lastCheckTime = 0;       // 保留字段但不使用
        stages[slot].state.stage001_1.lastReedCheckTime = 0;
        stages[slot].state.stage001_1.lastReedState = digitalRead(STAGE_001_1_REED_PIN);
        stages[slot].state.stage001_1.reedTriggered = false;
        stages[slot].state.stage001_1.lastLightIndex = -1;     // 初始化为-1表示未设置
        // 初始化防抖状态
        stages[slot].state.stage001_1.lowStateStartTime = 0;   // 未开始LOW状态
        stages[slot].state.stage001_1.debounceComplete = false; // 防抖未完成
        
        Serial.print(F("🔍 干簧管初始状态: "));
        Serial.println(stages[slot].state.stage001_1.lastReedState ? "HIGH" : "LOW");
        
        Serial.println(F("⏳ 等待干簧管触发..."));
        activeStageCount++;
        updateCompatibilityVars();
        return true;
    } else if (normalizedId == "001_2") {
        Serial.println(F("🌱 ===== 植物灯渐灭环节启动 ====="));
        Serial.print(F("🌱 环节001_2：植物灯渐灭效果("));
        Serial.print(STAGE_001_2_FADE_DURATION);
        Serial.println(F("ms内完成)"));
        
        // 🌱 重要：确保001_1环节完全停止，避免继续执行植物灯切换逻辑
        int stage001_1Index = findStageIndex("001_1");
        if (stage001_1Index >= 0 && stages[stage001_1Index].running) {
            Serial.println(F("🌱 检测到001_1环节仍在运行，立即停止"));
            stages[stage001_1Index].running = false;
            stages[stage001_1Index].stageId = "";
            activeStageCount--;
            Serial.println(F("🌱 001_1环节已停止，植物灯切换逻辑将终止"));
        }
        
        // ========================== 应用001_2环节引脚状态配置 ==========================
        Serial.println(F("🔧 应用001_2环节引脚状态配置..."));
        
        // 入口门系统
        pinManager.setPinState(C101_DOOR_LOCK_PIN, STAGE_001_2_DOOR_LOCK_STATE);
        pinManager.setPinState(C101_DOOR_LIGHT_PIN, STAGE_001_2_DOOR_LIGHT_STATE);
        Serial.print(F("🔒 电磁锁"));
        Serial.print(STAGE_001_2_DOOR_LOCK_STATE ? "上锁" : "解锁");
        Serial.println(F(" (Pin26)"));
        
        // 氛围射灯系统
        pinManager.setPinState(C101_AMBIENT_LIGHT_PIN, STAGE_001_2_AMBIENT_LIGHT_STATE);
        
        // 嘲讽按键灯光系统
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[0], STAGE_001_2_TAUNT_BUTTON1_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[1], STAGE_001_2_TAUNT_BUTTON2_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[2], STAGE_001_2_TAUNT_BUTTON3_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[3], STAGE_001_2_TAUNT_BUTTON4_STATE);
        
        // 画灯谜题系统
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[0], STAGE_001_2_PAINTING_LIGHT1_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[1], STAGE_001_2_PAINTING_LIGHT2_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[2], STAGE_001_2_PAINTING_LIGHT3_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[3], STAGE_001_2_PAINTING_LIGHT4_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[4], STAGE_001_2_PAINTING_LIGHT5_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[5], STAGE_001_2_PAINTING_LIGHT6_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[6], STAGE_001_2_PAINTING_LIGHT7_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[7], STAGE_001_2_PAINTING_LIGHT8_STATE);
        
        // 提示灯带系统
        pinManager.setPinState(C101_HINT_LED_PINS[0], STAGE_001_2_HINT_LED1_STATE);
        pinManager.setPinState(C101_HINT_LED_PINS[1], STAGE_001_2_HINT_LED2_STATE);
        
        // 蝴蝶灯谜题系统
        pinManager.setPinState(C101_BUTTERFLY_CARD_RELAY_PIN, STAGE_001_2_BUTTERFLY_CARD_STATE);
        pinManager.setPinState(C101_BUTTERFLY_LIGHT_PIN, STAGE_001_2_BUTTERFLY_LIGHT_STATE);
        pinManager.setPinState(C101_AD_FAN_PIN, STAGE_001_2_AD_FAN_STATE);
        
        Serial.println(F("✅ 001_2环节引脚状态配置完成"));
        
        // 初始化环节特定状态
        stages[slot].state.stage001_2.fadeStarted = false;
        stages[slot].state.stage001_2.lastFadeUpdate = 0;
        stages[slot].state.stage001_2.currentFadeStep = 0;
        stages[slot].state.stage001_2.fadeComplete = false;
        
        Serial.println(F("⏳ 准备开始植物灯渐灭效果..."));
        activeStageCount++;
        updateCompatibilityVars();
        return true;
    } else if (normalizedId == "002_0") {
        Serial.println(F("🎨 ===== 画灯谜题复杂效果环节启动 ====="));
        Serial.println(F("🎵 环节002_0：002号音频播放一次 + 203号音频循环播放"));
        Serial.println(F("🌟 画灯呼吸效果 + 闪烁效果并行执行"));
        Serial.println(F("💡 C101专注于灯光控制，音频由C102负责"));
        
        // ========================== 应用002_0环节引脚状态配置 ==========================
        Serial.println(F("🔧 应用002_0环节引脚状态配置..."));
        
        // 入口门系统
        pinManager.setPinState(C101_DOOR_LOCK_PIN, STAGE_002_0_DOOR_LOCK_STATE);
        pinManager.setPinState(C101_DOOR_LIGHT_PIN, STAGE_002_0_DOOR_LIGHT_STATE);
        
        // 氛围射灯系统
        pinManager.setPinState(C101_AMBIENT_LIGHT_PIN, STAGE_002_0_AMBIENT_LIGHT_STATE);
        
        // 嘲讽按键灯光系统
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[0], STAGE_002_0_TAUNT_BUTTON1_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[1], STAGE_002_0_TAUNT_BUTTON2_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[2], STAGE_002_0_TAUNT_BUTTON3_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[3], STAGE_002_0_TAUNT_BUTTON4_STATE);
        
        // 画灯谜题系统 - 初始化为关闭状态，由呼吸和闪烁效果动态控制
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[0], STAGE_002_0_PAINTING_LIGHT1_STATE);  // 画1：不参与效果
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[1], STAGE_002_0_PAINTING_LIGHT2_STATE);  // 画2：呼吸+闪烁
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[2], STAGE_002_0_PAINTING_LIGHT3_STATE);  // 画3：不参与效果
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[3], STAGE_002_0_PAINTING_LIGHT4_STATE);  // 画4：呼吸+闪烁
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[4], STAGE_002_0_PAINTING_LIGHT5_STATE);  // 画5：不参与效果
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[5], STAGE_002_0_PAINTING_LIGHT6_STATE);  // 画6：闪烁
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[6], STAGE_002_0_PAINTING_LIGHT7_STATE);  // 画7：不参与效果
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[7], STAGE_002_0_PAINTING_LIGHT8_STATE);  // 画8：呼吸+闪烁
        
        // 提示灯带系统
        pinManager.setPinState(C101_HINT_LED_PINS[0], STAGE_002_0_HINT_LED1_STATE);
        pinManager.setPinState(C101_HINT_LED_PINS[1], STAGE_002_0_HINT_LED2_STATE);
        
        // 蝴蝶灯谜题系统
        pinManager.setPinState(C101_BUTTERFLY_CARD_RELAY_PIN, STAGE_002_0_BUTTERFLY_CARD_STATE);
        pinManager.setPinState(C101_BUTTERFLY_LIGHT_PIN, STAGE_002_0_BUTTERFLY_LIGHT_STATE);
        pinManager.setPinState(C101_AD_FAN_PIN, STAGE_002_0_AD_FAN_STATE);
        
        Serial.println(F("✅ 002_0环节引脚状态配置完成"));
        
        // ========================== 初始化画灯效果状态 ==========================
        Serial.println(F("🎨 初始化画灯效果状态..."));
        
        // 音频相关状态（保留但不使用）
        stages[slot].state.stage002.channel1Started = false;
        stages[slot].state.stage002.channel2Started = false;
        stages[slot].state.stage002.multiJumpTriggered = false;
        
        // 呼吸效果状态初始化
        stages[slot].state.stage002.breathEffectStartTime = 0;
        stages[slot].state.stage002.currentBreathStep = -1;      // -1表示未开始
        stages[slot].state.stage002.breathEffectActive = false;
        
        // 闪烁效果状态初始化
        stages[slot].state.stage002.flashEffectStartTime = 0;
        stages[slot].state.stage002.currentFlashGroup = -1;      // -1表示未开始
        stages[slot].state.stage002.currentFlashCycle = 0;
        stages[slot].state.stage002.flashEffectActive = false;
        stages[slot].state.stage002.flashState = false;
        stages[slot].state.stage002.lastFlashToggle = 0;
        
        Serial.println(F("🌟 画灯呼吸效果时间表："));
        Serial.print(F("   8118ms: 画4长呼吸亮 -> 12009ms: 画4长呼吸灭"));
        Serial.print(F(" -> 17205ms: 画8长呼吸亮 -> 18705ms: 画8长呼吸灭"));
        Serial.println(F(" -> 24741ms: 画2长呼吸亮 -> 27495ms: 画2长呼吸灭"));
        
        Serial.println(F("⚡ 画灯闪烁效果时间表："));
        Serial.print(F("   22860ms: 画4长+画8长闪烁 -> 77204ms: 画2长+画6长闪烁"));
        Serial.print(F(" -> 125538ms: 画4长+画8长闪烁 -> 173219ms: 画2长+画6长闪烁"));
        Serial.println(F(" (50ms亮/50ms灭，循环4次)"));
        
        Serial.println(F("⏳ 等待画灯效果启动..."));
        Serial.println(F("⏳ 等待30秒触发多环节跳转..."));
        activeStageCount++;
        updateCompatibilityVars();
        return true;
    } else if (normalizedId == "006_0") {
        Serial.println(F("🎮 ===== 嘲讽按键游戏环节启动 ====="));
        Serial.println(F("🎵 环节006_0：音频提示+按键匹配游戏"));
        Serial.print(F("🎯 需要连续"));
        Serial.print(STAGE_006_0_REQUIRED_CORRECT);
        Serial.println(F("次正确才能通关"));
        
        // ========================== 应用006_0环节引脚状态配置 ==========================
        Serial.println(F("🔧 应用006_0环节引脚状态配置..."));
        
        // 入口门系统
        pinManager.setPinState(C101_DOOR_LOCK_PIN, STAGE_006_0_DOOR_LOCK_STATE);
        pinManager.setPinState(C101_DOOR_LIGHT_PIN, STAGE_006_0_DOOR_LIGHT_STATE);
        
        // 氛围射灯系统
        pinManager.setPinState(C101_AMBIENT_LIGHT_PIN, STAGE_006_0_AMBIENT_LIGHT_STATE);
        
        // 嘲讽按键灯光系统 - 初始化为关闭，由呼吸效果控制
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[0], STAGE_006_0_TAUNT_BUTTON1_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[1], STAGE_006_0_TAUNT_BUTTON2_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[2], STAGE_006_0_TAUNT_BUTTON3_STATE);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[3], STAGE_006_0_TAUNT_BUTTON4_STATE);
        
        // 画灯谜题系统
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[0], STAGE_006_0_PAINTING_LIGHT1_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[1], STAGE_006_0_PAINTING_LIGHT2_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[2], STAGE_006_0_PAINTING_LIGHT3_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[3], STAGE_006_0_PAINTING_LIGHT4_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[4], STAGE_006_0_PAINTING_LIGHT5_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[5], STAGE_006_0_PAINTING_LIGHT6_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[6], STAGE_006_0_PAINTING_LIGHT7_STATE);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[7], STAGE_006_0_PAINTING_LIGHT8_STATE);
        
        // 提示灯带系统
        pinManager.setPinState(C101_HINT_LED_PINS[0], STAGE_006_0_HINT_LED1_STATE);
        pinManager.setPinState(C101_HINT_LED_PINS[1], STAGE_006_0_HINT_LED2_STATE);
        
        // 蝴蝶灯谜题系统
        pinManager.setPinState(C101_BUTTERFLY_CARD_RELAY_PIN, STAGE_006_0_BUTTERFLY_CARD_STATE);
        pinManager.setPinState(C101_BUTTERFLY_LIGHT_PIN, STAGE_006_0_BUTTERFLY_LIGHT_STATE);
        pinManager.setPinState(C101_AD_FAN_PIN, STAGE_006_0_AD_FAN_STATE);
        
        Serial.println(F("✅ 006_0环节引脚状态配置完成"));
        
        // ========================== 初始化嘲讽按键游戏状态 ==========================
        Serial.println(F("�� 初始化嘲讽按键游戏状态..."));
        
        // 初始化内部状态机
        stages[slot].state.stage006.subState = (decltype(stages[slot].state.stage006.subState))0; // SUB_INIT
        
        // 游戏核心状态
        stages[slot].state.stage006.totalCount = 0;           // 总计数器从0开始
        stages[slot].state.stage006.correctCount = 0;         // 正确计数器
        stages[slot].state.stage006.currentCorrectButton = 0; // 当前正确按键
        stages[slot].state.stage006.pressedButton = 0;        // 按下的按键
        stages[slot].state.stage006.buttonPressed = false;    // 按键状态
        
        // 语音控制状态
        stages[slot].state.stage006.voiceTriggered = false;
        stages[slot].state.stage006.voiceTriggerTime = 0;
        stages[slot].state.stage006.voicePlayedOnce = false;
        stages[slot].state.stage006.lastVoiceTime = 0;
        
        // 按键防抖状态
        stages[slot].state.stage006.buttonDebouncing = false;
        stages[slot].state.stage006.debouncingButton = -1;
        stages[slot].state.stage006.debounceStartTime = 0;
        
        // 时序控制状态
        stages[slot].state.stage006.errorStartTime = 0;
        stages[slot].state.stage006.correctStartTime = 0;
        
        // 初始化植物灯状态记录
        for (int i = 0; i < 4; i++) {
            stages[slot].state.stage006.plantLightStates[i] = false;
        }
        
        // 初始化植物灯时序呼吸状态
        stages[slot].state.stage006.plantBreathActive = false;
        stages[slot].state.stage006.plantBreathStartTime = 0;
        stages[slot].state.stage006.plantBreathIndex = 0;
        
        // 初始化嘲讽按键输入引脚
        for (int i = 0; i < C101_TAUNT_BUTTON_COUNT; i++) {
            pinMode(C101_TAUNT_BUTTON_COM_PINS[i], INPUT_PULLUP);
        }
        Serial.println(F("🔘 嘲讽按键输入引脚初始化完成"));
        
        // 初始化按键防抖状态
        stages[slot].state.stage006.buttonDebouncing = false;
        stages[slot].state.stage006.debouncingButton = -1;
        stages[slot].state.stage006.debounceStartTime = 0;
        for (int i = 0; i < 4; i++) {
            stages[slot].state.stage006.lastButtonStates[i] = HIGH;  // 初始状态为HIGH（未按下）
        }
        Serial.println(F("🔘 按键防抖状态初始化完成"));
        
        // 初始化语音IO输出引脚
        pinMode(STAGE_006_0_VOICE_IO_1, OUTPUT);
        pinMode(STAGE_006_0_VOICE_IO_2, OUTPUT);
        pinMode(STAGE_006_0_VOICE_IO_3, OUTPUT);
        pinMode(STAGE_006_0_VOICE_IO_4, OUTPUT);
        pinManager.setPinState(STAGE_006_0_VOICE_IO_1, HIGH);
        pinManager.setPinState(STAGE_006_0_VOICE_IO_2, HIGH);
        pinManager.setPinState(STAGE_006_0_VOICE_IO_3, HIGH);
        pinManager.setPinState(STAGE_006_0_VOICE_IO_4, HIGH);
        Serial.println(F(" 语音IO输出引脚初始化完成"));
        
        Serial.println(F("🌟 嘲讽按键呼吸效果："));
        Serial.println(F("   10秒循环：0-1500ms亮，1500-3000ms灭，5000-6500ms亮，6500-8000ms灭"));
        
        Serial.println(F("🎵 语音轮播系统："));
        Serial.println(F("   m%4映射：0→IO1, 1→IO3, 2→IO2, 3→IO4"));
        
        Serial.println(F("⏳ 等待游戏开始..."));
        activeStageCount++;
        updateCompatibilityVars();
        return true;
    } else {
        Serial.print(F("❌ 未定义的C101环节: "));
        Serial.println(normalizedId);
        stages[slot].running = false;
        return false;
    }
}

bool GameFlowManager::startMultipleStages(const String& stageIds) {
    Serial.print(F("=== 启动多个并行环节: "));
    Serial.print(stageIds);
    Serial.println(F(" ==="));
    
    int successCount = 0;
    int startPos = 0;
    
    // 解析逗号分隔的环节ID列表
    while (startPos < stageIds.length()) {
        int commaPos = stageIds.indexOf(',', startPos);
        String stageId;
        
        if (commaPos < 0) {
            // 最后一个环节
            stageId = stageIds.substring(startPos);
        } else {
            // 中间的环节
            stageId = stageIds.substring(startPos, commaPos);
        }
        
        // 去除前后空格
        stageId.trim();
        
        if (stageId.length() > 0) {
            if (startStage(stageId)) {
                successCount++;
            }
        }
        
        if (commaPos < 0) {
            break;
        }
        startPos = commaPos + 1;
    }
    
    Serial.print(F("✅ 成功启动"));
    Serial.print(successCount);
    Serial.print(F("个环节，当前活跃环节数: "));
    Serial.println(activeStageCount);
    
    return successCount > 0;
}

void GameFlowManager::stopStage(const String& stageId) {
    String normalizedId = normalizeStageId(stageId);
    int index = findStageIndex(normalizedId);
    
    if (index >= 0) {
        Serial.print(F("⏹️ 停止环节[槽位"));
        Serial.print(index);
        Serial.print(F("]: "));
        Serial.println(normalizedId);
        
        stages[index].running = false;
        stages[index].stageId = "";
        activeStageCount--;
        updateCompatibilityVars();
    }
}

void GameFlowManager::stopCurrentStage() {
    // 兼容旧接口：停止第一个活跃环节
    if (activeStageCount > 0) {
        for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
            if (stages[i].running) {
                Serial.print(F("⏹️ 结束当前环节[槽位"));
                Serial.print(i);
                Serial.print(F("]: "));
                Serial.println(stages[i].stageId);
                
                stages[i].running = false;
                stages[i].stageId = "";
                activeStageCount--;
                updateCompatibilityVars();
                break;
            }
        }
    }
}

void GameFlowManager::stopAllStages() {
    Serial.println(F("🛑 停止所有C101环节"));
    
    // 设置全局停止标志
    globalStopped = true;
    
    // 停止所有植物灯呼吸效果（000_0环节相关）
    for (int i = 0; i < C101_PLANT_LIGHT_COUNT; i++) {
        MillisPWM::stopBreathing(C101_PLANT_LIGHT_PINS[i]);
        MillisPWM::stop(C101_PLANT_LIGHT_PINS[i]);
        pinManager.setPinState(C101_PLANT_LIGHT_PINS[i], LOW);
    }
    Serial.println(F("💡 所有植物灯效果已停止"));
    
    // 停止所有画灯效果（002_0环节相关）
    for (int i = 0; i < C101_PAINTING_LIGHT_COUNT; i++) {
        MillisPWM::stopBreathing(C101_PAINTING_LIGHT_PINS[i]);
        MillisPWM::stop(C101_PAINTING_LIGHT_PINS[i]);
        pinManager.setPinState(C101_PAINTING_LIGHT_PINS[i], LOW);
    }
    Serial.println(F("🎨 所有画灯效果已停止"));
    
    // 停止所有按键灯呼吸效果（006_0环节相关）
    for (int i = 0; i < 4; i++) {
        MillisPWM::stopBreathing(C101_TAUNT_BUTTON_LIGHT_PINS[i]);
        MillisPWM::stop(C101_TAUNT_BUTTON_LIGHT_PINS[i]);
        pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[i], LOW);
    }
    Serial.println(F("💡 所有按键灯效果已停止"));
    
    // 停止所有音频播放（006_0环节相关）
    for (int i = 0; i < C101_AUDIO_MODULE_COUNT; i++) {
        pinManager.setPinState(C101_AUDIO_IO1_PINS[i], HIGH);
        pinManager.setPinState(C101_AUDIO_IO2_PINS[i], HIGH);
    }
    Serial.println(F("🎵 所有音频播放已停止"));
    
    // 停止所有环节
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        if (stages[i].running) {
            Serial.print(F("⏹️ 停止环节[槽位"));
            Serial.print(i);
            Serial.print(F("]: "));
            Serial.println(stages[i].stageId);
            
            stages[i].running = false;
            stages[i].stageId = "";
            stages[i].jumpRequested = false;
        }
    }
    
    activeStageCount = 0;
    updateCompatibilityVars();
    
    // 🔧 新增：压缩PWM通道，释放资源
    Serial.println(F("🔧 清理PWM通道资源..."));
    MillisPWM::stopAll();         // 停止所有PWM
    MillisPWM::compactChannels(); // 压缩通道数组
    MillisPWM::printChannelStatus(); // 打印最终状态
    
    Serial.println(F("✅ 所有环节已停止，资源已释放"));
}

// ========================== 状态查询 ==========================
const String& GameFlowManager::getCurrentStageId() const {
    return currentStageId;
}

bool GameFlowManager::isStageRunning() const {
    return stageRunning;
}

bool GameFlowManager::isStageRunning(const String& stageId) {
    String normalizedId = normalizeStageId(stageId);
    int index = findStageIndex(normalizedId);
    return (index >= 0 && stages[index].running);
}

unsigned long GameFlowManager::getStageElapsedTime() const {
    if (stageRunning) {
        return millis() - stageStartTime;
    }
    return 0;
}

unsigned long GameFlowManager::getStageElapsedTime(const String& stageId) {
    String normalizedId = normalizeStageId(stageId);
    int index = findStageIndex(normalizedId);
    if (index >= 0 && stages[index].running) {
        return millis() - stages[index].startTime;
    }
    return 0;
}

int GameFlowManager::getActiveStageCount() const {
    return activeStageCount;
}

void GameFlowManager::getActiveStages(String stages[], int maxCount) {
    int count = 0;
    for (int i = 0; i < MAX_PARALLEL_STAGES && count < maxCount; i++) {
        if (this->stages[i].running) {
            stages[count++] = this->stages[i].stageId;
        }
    }
}

// ========================== 环节列表 ==========================
bool GameFlowManager::isValidStageId(const String& stageId) {
    String normalizedId = normalizeStageId(stageId);
    return (normalizedId == "000_0" || 
            normalizedId == "001_1" ||
            normalizedId == "001_2" || 
            normalizedId == "002_0" ||
            normalizedId == "006_0");
}

void GameFlowManager::printAvailableStages() {
    Serial.println(F("=== C101可用音频环节列表 ==="));
    
    Serial.println(F("000_0 - C101初始化环节：植物灯顺序呼吸效果(无音频)"));
    
    Serial.println(F("001_1 - C101干簧管检测环节(无音频，等待干簧管触发)"));
    
    Serial.print(F("001_2 - 植物灯渐灭效果("));
    Serial.print(STAGE_001_2_FADE_DURATION);
    Serial.println(F("ms内完成)"));
    
    Serial.print(F("002_0 - 画灯谜题复杂效果：呼吸效果+闪烁效果并行，30秒触发多环节跳转("));
    Serial.print(STAGE_002_0_DURATION/1000);
    Serial.println(F("秒后完成)"));
    
    Serial.print(F("006_0 - 嘲讽按键游戏：音频提示+按键匹配，需要连续"));
    Serial.print(STAGE_006_0_REQUIRED_CORRECT);
    Serial.println(F("次正确才能通关"));
    
    Serial.println(F("=============================="));
}

// ========================== 更新和调试功能 ==========================
void GameFlowManager::update() {
    // 更新统一引脚管理器
    pinManager.updateAllPins();
    
    // 检查紧急开门功能
    checkEmergencyDoorControl();
    
    // 更新所有活跃环节
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        if (stages[i].running) {
            updateStage(i);
        }
    }
}

void GameFlowManager::printStatus() {
    Serial.println(F("=== C101 GameFlowManager状态 ==="));
    Serial.print(F("活跃环节数: "));
    Serial.print(activeStageCount);
    Serial.print(F("/"));
    Serial.println(MAX_PARALLEL_STAGES);
    Serial.print(F("全局停止: "));
    Serial.println(globalStopped ? "是" : "否");
    
    if (activeStageCount > 0) {
        Serial.println(F("--- 运行中的环节 ---"));
        for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
            if (stages[i].running) {
                Serial.print(F("[槽位"));
                Serial.print(i);
                Serial.print(F("] "));
                Serial.print(stages[i].stageId);
                Serial.print(F(" - 运行时间: "));
                Serial.print(millis() - stages[i].startTime);
                Serial.print(F("ms"));
                if (stages[i].jumpRequested) {
                    Serial.print(F(" [已请求跳转]"));
                }
                Serial.println();
            }
        }
    } else {
        Serial.println(F("当前无运行环节"));
    }
    
    Serial.println(F("================================"));
}

// ========================== 环节跳转请求 ==========================
void GameFlowManager::requestStageJump(const String& nextStage) {
    // 兼容旧接口：使用第一个活跃环节
    if (activeStageCount > 0) {
        for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
            if (stages[i].running && !stages[i].jumpRequested) {
                requestMultiStageJump(stages[i].stageId, nextStage);
                break;
            }
        }
    }
}

void GameFlowManager::requestMultiStageJump(const String& currentStep, const String& nextSteps) {
    Serial.print(F("�� 请求从"));
    Serial.print(currentStep);
    Serial.print(F("跳转到环节: "));
    Serial.println(nextSteps);
    
    // 发送STEP_COMPLETE消息，支持多个next_step
    String message = "$[GAME]@C101{^STEP_COMPLETE^(current_step=\"" + currentStep + 
                    "\",next_step=\"" + nextSteps + 
                    "\",duration=" + String(getStageElapsedTime(currentStep)) + 
                    ",error_count=0)}#";
    
    harbingerClient.sendMessage(message);
    Serial.print(F("📡 发送消息: "));
    Serial.println(message);
    
    // 标记该环节已请求跳转
    int index = findStageIndex(currentStep);
    if (index >= 0) {
        stages[index].jumpRequested = true;
    }
}

// ========================== 紧急开门功能实现 ==========================
void GameFlowManager::initEmergencyDoorControl() {
    Serial.println(F("🚨 初始化紧急开门功能"));
    Serial.print(F("   读卡器检测引脚: Pin"));
    Serial.println(C101_DOOR_CARD_COM_PIN);
    Serial.print(F("   电磁锁控制引脚: Pin"));
    Serial.println(C101_DOOR_LOCK_PIN);
    Serial.print(F("   解锁持续时间: "));
    Serial.print(EMERGENCY_UNLOCK_DURATION / 1000);
    Serial.println(F("秒"));
    
    // 确保引脚已正确初始化（在initC101Hardware中已完成）
    lastCardReaderState = digitalRead(C101_DOOR_CARD_COM_PIN);
    Serial.println(F("✅ 紧急开门功能就绪"));
}

void GameFlowManager::updateEmergencyDoorControl() {
    // 检测Pin24门禁读卡器状态 - 高频检测
    bool currentCardReaderState = digitalRead(C101_DOOR_CARD_COM_PIN);
    
    // 方式1：边沿检测（从HIGH到LOW的下降沿触发）
    bool edgeTriggered = (lastCardReaderState == HIGH && currentCardReaderState == LOW);
    
    // 方式2：直接LOW状态检测（更敏感）
    bool directTriggered = (currentCardReaderState == LOW && !emergencyUnlockActive);
    
    // 任一方式触发都立即解锁
    if (edgeTriggered || directTriggered) {
        Serial.println(F("🚨 紧急开门触发！门禁读卡器检测到信号"));
        
        // 立即解锁电磁锁
        pinManager.setPinState(C101_DOOR_LOCK_PIN, LOW);   // Pin26解锁（断电）
        emergencyUnlockStartTime = millis();
        emergencyUnlockActive = true;
        
        Serial.println(F("🔓 电磁锁已解锁，10秒后自动上锁"));
    }
    lastCardReaderState = currentCardReaderState;
    
    // 检查紧急解锁超时
    if (emergencyUnlockActive && (millis() - emergencyUnlockStartTime >= EMERGENCY_UNLOCK_DURATION)) {
        pinManager.setPinState(C101_DOOR_LOCK_PIN, HIGH);  // Pin26上锁（通电）
        emergencyUnlockActive = false;
        Serial.println(F("🔒 电磁锁自动上锁"));
    }
}

bool GameFlowManager::isEmergencyUnlockActive() const {
    return emergencyUnlockActive;
}

// ========================== 门锁和灯光控制实现 ==========================
void GameFlowManager::resetDoorAndLightState() {
    // 只有在非紧急解锁状态下才重置门锁状态
    if (!emergencyUnlockActive) {
        pinManager.setPinState(C101_DOOR_LOCK_PIN, HIGH);   // Pin26电磁锁上锁（通电）
        Serial.println(F("🔒 电磁锁已上锁"));
    } else {
        Serial.println(F("⚠️ 紧急解锁激活中，跳过门锁重置"));
    }
    
    pinManager.setPinState(C101_DOOR_LIGHT_PIN, LOW);       // Pin25指引射灯关闭
    Serial.println(F("💡 指引射灯已关闭"));
}

// ========================== 私有方法实现 ==========================

// 环节完成通知
void GameFlowManager::notifyStageComplete(const String& currentStep, const String& nextStep, unsigned long duration) {
    // 查找环节索引
    int index = findStageIndex(currentStep);
    if (index >= 0 && stages[index].jumpRequested) {
        return;  // 避免重复通知
    }
    
    String message = "$[GAME]@C101{^STEP_COMPLETE^(current_step=\"" + currentStep + 
                    "\",next_step=\"" + nextStep + 
                    "\",duration=" + String(duration) + 
                    ",error_count=0)}#";
    
    harbingerClient.sendMessage(message);
    Serial.print(F("📡 环节完成通知: "));
    Serial.println(message);
    
    // 标记该环节已请求跳转
    if (index >= 0) {
        stages[index].jumpRequested = true;
    }
}

void GameFlowManager::notifyStageComplete(const String& currentStep, unsigned long duration) {
    // 查找环节索引
    int index = findStageIndex(currentStep);
    if (index >= 0 && stages[index].jumpRequested) {
        return;  // 避免重复通知
    }
    
    String message = "$[GAME]@C101{^STEP_COMPLETE^(current_step=\"" + currentStep + 
                    "\",duration=" + String(duration) + 
                    ",error_count=0)}#";
    
    harbingerClient.sendMessage(message);
    Serial.print(F("📡 环节完成通知: "));
    Serial.println(message);
    
    // 标记该环节已请求跳转
    if (index >= 0) {
        stages[index].jumpRequested = true;
    }
}

// C101音频环节更新方法
void GameFlowManager::updateStep000(int index) {
    if (index < 0 || index >= MAX_PARALLEL_STAGES || !stages[index].running) {
        return;
    }
    
    unsigned long elapsed = millis() - stages[index].startTime;
    StageState& stage = stages[index];
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // C101的000_0环节：只有植物灯顺序呼吸效果，无音频播放
    
    // 启动植物灯顺序呼吸效果
    if (!stage.state.stage000.lightEffectStarted && elapsed >= STAGE_000_0_START) {
        stage.state.stage000.lightEffectStarted = true;
        stage.state.stage000.lightCycleStartTime = elapsed;
        
        // 🔧 立即启动植物灯1的呼吸效果
        MillisPWM::startBreathing(C101_PLANT_LIGHT_PINS[0], 3.0);  // 3秒呼吸周期
        Serial.println(F("💡 植物灯顺序呼吸效果启动"));
    }
    
    // 植物灯顺序呼吸效果控制
    if (stage.state.stage000.lightEffectStarted) {
        unsigned long cycleElapsed = elapsed - stage.state.stage000.lightCycleStartTime;
        unsigned long currentCycleTime = cycleElapsed % STAGE_000_0_LIGHT_CYCLE;
        
        // 根据时间表确定当前应该亮起的灯
        // 0ms: 灯1, 1500ms: 灯3, 3000ms: 灯2, 4500ms: 灯4
        int targetLightIndex;
        if (currentCycleTime < 1500) {
            targetLightIndex = 0;  // 灯1 (Pin2)
        } else if (currentCycleTime < 3000) {
            targetLightIndex = 2;  // 灯3 (Pin6) 
        } else if (currentCycleTime < 4500) {
            targetLightIndex = 1;  // 灯2 (Pin3)
        } else {
            targetLightIndex = 3;  // 灯4 (Pin5)
        }
        
        // 如果需要切换灯光
        if (stage.state.stage000.currentLightIndex != targetLightIndex) {
            // 关闭当前灯
            if (stage.state.stage000.currentLightIndex >= 0) {
                MillisPWM::stopBreathing(C101_PLANT_LIGHT_PINS[stage.state.stage000.currentLightIndex]);
            }
            
            // 开启新灯
            MillisPWM::startBreathing(C101_PLANT_LIGHT_PINS[targetLightIndex], 3.0);  // 3秒呼吸周期
            Serial.print(F("🌱 植物灯"));
            Serial.print(targetLightIndex + 1);
            Serial.println(F("呼吸"));
            
            stage.state.stage000.currentLightIndex = targetLightIndex;
        }
    }
    
    // C101的000_0环节作为初始化环节，不自动跳转，等待服务器指令
    // 只有植物灯顺序呼吸效果，无音频播放
}

void GameFlowManager::updateStep001_1(int index) {
    if (index < 0 || index >= MAX_PARALLEL_STAGES || !stages[index].running) {
        return;
    }
    
    unsigned long elapsed = millis() - stages[index].startTime;
    StageState& stage = stages[index];
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 🌱 继续执行植物灯顺序呼吸效果（继承000_0的逻辑）
    // 使用环节状态中的时间基准来保持连续性
    if (stage.state.stage001_1.lastCheckTime == 0) {
        // 第一次运行时，设置基准时间
        stage.state.stage001_1.lastCheckTime = millis() - elapsed;
    }
    unsigned long totalElapsed = millis() - stage.state.stage001_1.lastCheckTime;
    
    // 植物灯顺序呼吸效果控制（与000_0相同的逻辑）
    unsigned long currentCycleTime = totalElapsed % STAGE_000_0_LIGHT_CYCLE;
    
    // 根据时间表确定当前应该亮起的灯
    // 0ms: 灯1, 1500ms: 灯3, 3000ms: 灯2, 4500ms: 灯4
    int targetLightIndex;
    if (currentCycleTime < 1500) {
        targetLightIndex = 0;  // 灯1 (Pin2)
    } else if (currentCycleTime < 3000) {
        targetLightIndex = 2;  // 灯3 (Pin6) 
    } else if (currentCycleTime < 4500) {
        targetLightIndex = 1;  // 灯2 (Pin3)
    } else {
        targetLightIndex = 3;  // 灯4 (Pin5)
    }
    
    // 如果需要切换灯光
    if (stage.state.stage001_1.lastLightIndex != targetLightIndex) {
        // 关闭当前灯
        if (stage.state.stage001_1.lastLightIndex >= 0) {
            MillisPWM::stopBreathing(C101_PLANT_LIGHT_PINS[stage.state.stage001_1.lastLightIndex]);
        }
        
        // 开启新灯
        MillisPWM::startBreathing(C101_PLANT_LIGHT_PINS[targetLightIndex], 3.0);  // 3秒呼吸周期
        Serial.print(F("🌱 [001_1] 植物灯"));
        Serial.print(targetLightIndex + 1);
        Serial.println(F("呼吸"));
        
        stage.state.stage001_1.lastLightIndex = targetLightIndex;
    }
    
    // 干簧管检测逻辑：高频检测 + 防抖机制
    if (!stage.state.stage001_1.reedTriggered) {
        unsigned long now = millis();
        
        // 高频检测（10ms间隔）
        if (now - stage.state.stage001_1.lastReedCheckTime >= STAGE_001_1_REED_CHECK_INTERVAL) {
            bool currentReedState = digitalRead(STAGE_001_1_REED_PIN);
            
            if (currentReedState == LOW) {
                // 检测到LOW状态
                if (stage.state.stage001_1.lowStateStartTime == 0) {
                    // 第一次检测到LOW状态，记录开始时间
                    stage.state.stage001_1.lowStateStartTime = now;
                } else {
                    // 持续LOW状态，检查是否达到防抖时间
                    unsigned long lowDuration = now - stage.state.stage001_1.lowStateStartTime;
                    if (lowDuration >= STAGE_001_1_REED_DEBOUNCE_TIME && !stage.state.stage001_1.debounceComplete) {
                        // 防抖完成，触发跳转
                        Serial.print(F("🔍 Pin"));
                        Serial.print(STAGE_001_1_REED_PIN);
                        Serial.print(F("防抖完成("));
                        Serial.print(lowDuration);
                        Serial.println(F("ms)，跳转到001_2"));
                        
                        stage.state.stage001_1.reedTriggered = true;
                        stage.state.stage001_1.debounceComplete = true;
                        notifyStageComplete("001_1", STAGE_001_1_NEXT_STAGE, elapsed);
                    }
                }
            } else {
                // 检测到HIGH状态，重置防抖状态
                if (stage.state.stage001_1.lowStateStartTime != 0) {
                    stage.state.stage001_1.lowStateStartTime = 0;
                    stage.state.stage001_1.debounceComplete = false;
                }
            }
            
            // 更新检测时间
            stage.state.stage001_1.lastReedCheckTime = now;
        }
    }
}

void GameFlowManager::updateStep001_2(int index) {
    if (index < 0 || index >= MAX_PARALLEL_STAGES || !stages[index].running) {
        return;
    }
    
    unsigned long elapsed = millis() - stages[index].startTime;
    StageState& stage = stages[index];
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 立即开始植物灯渐灭效果（从0ms开始，不需要延迟）
    if (!stage.state.stage001_2.fadeStarted) {
        stage.state.stage001_2.fadeStarted = true;
        Serial.println(F("🌱 立即开始植物灯渐灭效果"));
        
        // 停止所有植物灯的呼吸效果，开始fade渐灭
        for (int i = 0; i < C101_PLANT_LIGHT_COUNT; i++) {
            MillisPWM::stopBreathing(C101_PLANT_LIGHT_PINS[i]);
            // 使用MillisPWM的fadeOut功能，1500ms内从当前亮度渐灭到0
            MillisPWM::fadeOut(C101_PLANT_LIGHT_PINS[i], STAGE_001_2_FADE_DURATION);
        }
    }
    
    // 检查渐灭是否完成
    if (stage.state.stage001_2.fadeStarted && !stage.state.stage001_2.fadeComplete && elapsed >= STAGE_001_2_FADE_DURATION) {
        stage.state.stage001_2.fadeComplete = true;
        
        // 确保所有植物灯都完全关闭
        for (int i = 0; i < C101_PLANT_LIGHT_COUNT; i++) {
            MillisPWM::stop(C101_PLANT_LIGHT_PINS[i]);      // 停止PWM
            pinManager.setPinState(C101_PLANT_LIGHT_PINS[i], LOW);    // 设置为低电平
        }
        
        Serial.println(F("✅ 植物灯渐灭完成"));
    }
}

void GameFlowManager::updateStep002(int index) {
    if (index < 0 || index >= MAX_PARALLEL_STAGES || !stages[index].running) {
        return;
    }
    
    unsigned long elapsed = millis() - stages[index].startTime;
    StageState& stage = stages[index];
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // ========================== 画灯呼吸效果控制 ==========================
    // 🔧 自然呼吸效果：每个画灯在指定时间段内完成一次完整的呼吸周期
    if (elapsed >= STAGE_002_0_BREATH_START_1 && elapsed < STAGE_002_0_BREATH_END_2) {
        // 画4长呼吸时间段：8118ms-13509ms (总时长5391ms)
        if (stage.state.stage002.currentBreathStep != 0) {
            stage.state.stage002.currentBreathStep = 0;
            // 计算呼吸周期：整个时间段就是一个完整的呼吸周期
            float breathCycleDuration = (STAGE_002_0_BREATH_END_2 - STAGE_002_0_BREATH_START_1) / 1000.0; // 转换为秒
            MillisPWM::startBreathing(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_4_INDEX], breathCycleDuration);
            Serial.print(F("🎨 画4长射灯开始呼吸（周期："));
            Serial.print(breathCycleDuration);
            Serial.println(F("秒）"));
        }
    } else if (elapsed >= STAGE_002_0_BREATH_END_2 && stage.state.stage002.currentBreathStep == 0) {
        // 画4长呼吸结束
        stage.state.stage002.currentBreathStep = 1;
        MillisPWM::stopBreathing(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_4_INDEX]);
        MillisPWM::setBrightness(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_4_INDEX], 0);
        Serial.println(F("🎨 画4长射灯呼吸结束"));
    }
    
    if (elapsed >= STAGE_002_0_BREATH_START_3 && elapsed < STAGE_002_0_BREATH_END_4) {
        // 画8长呼吸时间段：17205ms-19822ms (总时长2617ms)
        if (stage.state.stage002.currentBreathStep != 2) {
            stage.state.stage002.currentBreathStep = 2;
            // 计算呼吸周期：整个时间段就是一个完整的呼吸周期
            float breathCycleDuration = (STAGE_002_0_BREATH_END_4 - STAGE_002_0_BREATH_START_3) / 1000.0; // 转换为秒
            MillisPWM::startBreathing(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_8_INDEX], breathCycleDuration);
            Serial.print(F("🎨 画8长射灯开始呼吸（周期："));
            Serial.print(breathCycleDuration);
            Serial.println(F("秒）"));
        }
    } else if (elapsed >= STAGE_002_0_BREATH_END_4 && stage.state.stage002.currentBreathStep == 2) {
        // 画8长呼吸结束
        stage.state.stage002.currentBreathStep = 3;
        MillisPWM::stopBreathing(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_8_INDEX]);
        MillisPWM::setBrightness(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_8_INDEX], 0);
        Serial.println(F("🎨 画8长射灯呼吸结束"));
    }
    
    if (elapsed >= STAGE_002_0_BREATH_START_5 && elapsed < STAGE_002_0_BREATH_END_6) {
        // 画2长呼吸时间段：24741ms-28995ms (总时长4254ms)
        if (stage.state.stage002.currentBreathStep != 4) {
            stage.state.stage002.currentBreathStep = 4;
            // 计算呼吸周期：整个时间段就是一个完整的呼吸周期
            float breathCycleDuration = (STAGE_002_0_BREATH_END_6 - STAGE_002_0_BREATH_START_5) / 1000.0; // 转换为秒
            MillisPWM::startBreathing(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_2_INDEX], breathCycleDuration);
            Serial.print(F("🎨 画2长射灯开始呼吸（周期："));
            Serial.print(breathCycleDuration);
            Serial.println(F("秒）"));
        }
    } else if (elapsed >= STAGE_002_0_BREATH_END_6 && stage.state.stage002.currentBreathStep == 4) {
        // 画2长呼吸结束
        stage.state.stage002.currentBreathStep = 5;
        MillisPWM::stopBreathing(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_2_INDEX]);
        MillisPWM::setBrightness(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_2_INDEX], 0);
        Serial.println(F("🎨 画2长射灯呼吸结束"));
    }
    
    // ========================== 画灯闪烁效果控制（从22860ms开始，每180秒循环）==========================
    if (elapsed >= STAGE_002_0_FLASH_START_1) {
        // 计算当前在哪个180秒循环周期内
        unsigned long flashElapsed = elapsed - STAGE_002_0_FLASH_START_1;
        unsigned long currentCycle = flashElapsed / STAGE_002_0_FLASH_CYCLE_DURATION;  // 当前循环轮数（0,1,2...）
        unsigned long cycleTime = flashElapsed % STAGE_002_0_FLASH_CYCLE_DURATION;    // 当前循环内的时间
        
        // 计算当前循环内的绝对时间点
        unsigned long absoluteTime = STAGE_002_0_FLASH_START_1 + cycleTime;
        
        // 判断当前闪烁组
        int currentFlashGroup = -1;
        
        if (absoluteTime >= STAGE_002_0_FLASH_START_1 && absoluteTime < STAGE_002_0_FLASH_END_1) {
            currentFlashGroup = 0;  // 画4长+画8长闪烁
        } else if (absoluteTime >= STAGE_002_0_FLASH_START_2 && absoluteTime < STAGE_002_0_FLASH_END_2) {
            currentFlashGroup = 1;  // 画2长+画6长闪烁
        } else if (absoluteTime >= STAGE_002_0_FLASH_START_3 && absoluteTime < STAGE_002_0_FLASH_END_3) {
            currentFlashGroup = 2;  // 画4长+画8长闪烁
        } else if (absoluteTime >= STAGE_002_0_FLASH_START_4 && absoluteTime < STAGE_002_0_FLASH_END_4) {
            currentFlashGroup = 3;  // 画2长+画6长闪烁
        }
        
        // 🔧 关键修复：如果当前闪烁组正在执行且未完成，保持该组状态
        if (stage.state.stage002.currentFlashGroup >= 0 && 
            stage.state.stage002.currentFlashCycle < STAGE_002_0_FLASH_CYCLES) {
            // 正在闪烁中，不管时间窗口，保持当前闪烁组
            currentFlashGroup = stage.state.stage002.currentFlashGroup;
        }
        
        // 如果闪烁组发生变化，重置闪烁状态
        if (currentFlashGroup != stage.state.stage002.currentFlashGroup) {
            stage.state.stage002.currentFlashGroup = currentFlashGroup;
            stage.state.stage002.currentFlashCycle = 0;
            stage.state.stage002.flashState = false;
            stage.state.stage002.lastFlashToggle = millis();
            
            // 🔧 修复：停止PWM，确保digitalWrite能正常工作
            if (currentFlashGroup == 0 || currentFlashGroup == 2) {
                // 画4长+画8长闪烁组：停止PWM并清理灯光
                MillisPWM::stop(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_4_INDEX]);
                MillisPWM::stop(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_8_INDEX]);
                pinManager.setPinState(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_4_INDEX], LOW);
                pinManager.setPinState(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_8_INDEX], LOW);
                Serial.print(F("⚡ [循环"));
                Serial.print(currentCycle + 1);
                Serial.println(F("] 开始画4长+画8长闪烁"));
            } else if (currentFlashGroup == 1 || currentFlashGroup == 3) {
                // 画2长+画6长闪烁组：停止PWM并清理灯光
                MillisPWM::stop(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_2_INDEX]);
                MillisPWM::stop(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_6_INDEX]);
                pinManager.setPinState(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_2_INDEX], LOW);
                pinManager.setPinState(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_6_INDEX], LOW);
                Serial.print(F("⚡ [循环"));
                Serial.print(currentCycle + 1);
                Serial.println(F("] 开始画2长+画6长闪烁"));
            }
        }
        
        // 执行闪烁逻辑
        if (currentFlashGroup >= 0 && stage.state.stage002.currentFlashCycle < STAGE_002_0_FLASH_CYCLES) {
            unsigned long now = millis();
            unsigned long flashInterval = stage.state.stage002.flashState ? STAGE_002_0_FLASH_ON_TIME : STAGE_002_0_FLASH_OFF_TIME;
            
            if (now - stage.state.stage002.lastFlashToggle >= flashInterval) {
                stage.state.stage002.flashState = !stage.state.stage002.flashState;
                stage.state.stage002.lastFlashToggle = now;
                
                // 根据闪烁组和状态控制灯光
                if (currentFlashGroup == 0 || currentFlashGroup == 2) {
                    // 画4长+画8长闪烁
                    pinManager.setPinState(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_4_INDEX], stage.state.stage002.flashState ? HIGH : LOW);
                    pinManager.setPinState(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_8_INDEX], stage.state.stage002.flashState ? HIGH : LOW);
                } else if (currentFlashGroup == 1 || currentFlashGroup == 3) {
                    // 画2长+画6长闪烁
                    pinManager.setPinState(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_2_INDEX], stage.state.stage002.flashState ? HIGH : LOW);
                    pinManager.setPinState(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_6_INDEX], stage.state.stage002.flashState ? HIGH : LOW);
                }
                
                // 如果完成了一个亮灭周期，增加循环计数
                if (!stage.state.stage002.flashState) {
                    stage.state.stage002.currentFlashCycle++;
                }
            }
        }
    }
    
    // ========================== 多环节跳转控制 ==========================
    // 30秒时触发多环节跳转
    if (!stage.state.stage002.multiJumpTriggered && elapsed >= STAGE_002_0_MULTI_JUMP_TIME) {
        stage.state.stage002.multiJumpTriggered = true;
        Serial.print(F("🚀 [C101-槽位"));
        Serial.print(index);
        Serial.print(F("] 30秒时触发多环节跳转: "));
        Serial.println(STAGE_002_0_MULTI_JUMP_STAGES);
        requestMultiStageJump("002_0", STAGE_002_0_MULTI_JUMP_STAGES);
    }
    
    // ========================== 环节完成控制 ==========================
    // 60秒后跳转到下一环节或报告完成
    if (!stage.jumpRequested && elapsed >= STAGE_002_0_DURATION) {
        if (strlen(STAGE_002_0_NEXT_STAGE) > 0) {
            // 检查下一环节是否已经在运行
            if (!isStageRunning(STAGE_002_0_NEXT_STAGE)) {
                Serial.print(F("⏰ [C101-槽位"));
                Serial.print(index);
                Serial.print(F("] 环节002_0完成，跳转到"));
                Serial.println(STAGE_002_0_NEXT_STAGE);
                notifyStageComplete("002_0", STAGE_002_0_NEXT_STAGE, elapsed);
            } else {
                Serial.print(F("⚠️ [C101-槽位"));
                Serial.print(index);
                Serial.print(F("] 环节002_0定时跳转取消，目标环节"));
                Serial.print(STAGE_002_0_NEXT_STAGE);
                Serial.println(F("已在运行"));
                stage.jumpRequested = true;  // 标记为已处理，避免重复检查
            }
        } else {
            Serial.print(F("⏰ [C101-槽位"));
            Serial.print(index);
            Serial.println(F("] 环节002_0完成"));
            notifyStageComplete("002_0", elapsed);
        }
    }
}

// 工具方法
String GameFlowManager::normalizeStageId(const String& stageId) {
    String normalized = stageId;
    
    // 移除引号
    normalized.replace("\"", "");
    
    // C101音频格式保持下划线格式：000_0, 001_2, 002_0
    // 不需要转换，直接返回
    
    Serial.print(F("🔧 环节ID标准化: "));
    Serial.print(stageId);
    Serial.print(F(" -> "));
    Serial.println(normalized);
    
    return normalized;
}

// ========================== 音量管理方法 ==========================
void GameFlowManager::initializeAllVolumes() {
    Serial.println(F("🔊 初始化所有通道音量..."));
    for (int channel = 1; channel <= TOTAL_CHANNELS; channel++) {
        voice.setVolume(channel, DEFAULT_VOLUME);
        Serial.print(F("🔊 通道"));
        Serial.print(channel);
        Serial.print(F("音量设置为"));
        Serial.println(DEFAULT_VOLUME);
        delay(50); // 避免命令发送过快
    }
    Serial.println(F("✅ 所有通道音量初始化完成"));
}

void GameFlowManager::resetChannelVolume(int channel) {
    if (channel >= 1 && channel <= TOTAL_CHANNELS) {
        voice.setVolume(channel, DEFAULT_VOLUME);
        Serial.print(F("🔊 重置通道"));
        Serial.print(channel);
        Serial.print(F("音量为"));
        Serial.println(DEFAULT_VOLUME);
    }
}

void GameFlowManager::resetAllVolumes() {
    Serial.println(F("🔊 重置所有通道音量..."));
    for (int channel = 1; channel <= TOTAL_CHANNELS; channel++) {
        voice.setVolume(channel, DEFAULT_VOLUME);
        Serial.print(F("🔊 通道"));
        Serial.print(channel);
        Serial.print(F("音量重置为"));
        Serial.println(DEFAULT_VOLUME);
        delay(50); // 避免命令发送过快
    }
    Serial.println(F("✅ 所有通道音量重置完成"));
}

// ========================== 006_0环节：嘲讽按键游戏 ==========================
void GameFlowManager::updateStep006(int index) {
    if (index < 0 || index >= MAX_PARALLEL_STAGES || !stages[index].running) {
        return;
    }
    
    unsigned long elapsed = millis() - stages[index].startTime;
    StageState& stage = stages[index];
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // ========================== 基于步骤的if-else流程 ==========================
    
    if (stage.state.stage006.subState == 0) {
        // ========================== STEP_1_INIT: 初始化 ==========================
        
        Serial.println(F("🎮 开始006环节初始化"));
        
        // 启动4个按键的呼吸效果（3秒周期）
        for (int i = 0; i < 4; i++) {
            MillisPWM::startBreathing(C101_TAUNT_BUTTON_LIGHT_PINS[i], 3.0);
        }
        
        // 初始化游戏状态
        stage.state.stage006.totalCount = 1;  // 第一轮 m=1
        stage.state.stage006.correctCount = 0;
        stage.state.stage006.buttonPressed = false;
        stage.state.stage006.buttonDebouncing = false;
        
        // 初始化按键状态记录
        for (int i = 0; i < 4; i++) {
            stage.state.stage006.lastButtonStates[i] = digitalRead(C101_TAUNT_BUTTON_COM_PINS[i]);
        }
        
        // 初始化植物灯状态记录
        for (int i = 0; i < 4; i++) {
            stage.state.stage006.plantLightStates[i] = false;
        }
        
        // 计算第一轮的正确按键和语音IO
        int voiceIndex = (stage.state.stage006.totalCount - 1) % 4;  // m=1时，voiceIndex=0
        stage.state.stage006.currentCorrectButton = (voiceIndex == 0) ? 1 : 
                                                   (voiceIndex == 1) ? 3 : 
                                                   (voiceIndex == 2) ? 2 : 4;
        
        // 触发对应的语音IO
        int voicePin = (voiceIndex == 0) ? STAGE_006_0_VOICE_IO_1 :
                       (voiceIndex == 1) ? STAGE_006_0_VOICE_IO_3 :
                       (voiceIndex == 2) ? STAGE_006_0_VOICE_IO_2 : 
                                           STAGE_006_0_VOICE_IO_4;
        
        Serial.print(F("🎵 播放语音IO"));
        Serial.print(voiceIndex + 1);
        Serial.print(F("，正确按键="));
        Serial.println(stage.state.stage006.currentCorrectButton);
        
        // 正常的语音IO触发逻辑 - 使用临时状态自动恢复
        pinManager.setPinTemporaryState(voicePin, LOW, STAGE_006_0_VOICE_TRIGGER_LOW_TIME, HIGH);
        stage.state.stage006.voiceTriggered = true;
        stage.state.stage006.voiceTriggerTime = millis();
        stage.state.stage006.voicePlayedOnce = false;
        stage.state.stage006.lastVoiceTime = millis();
        
        // 立即转入等待输入状态
        stage.state.stage006.subState = 1; // SUB_WAITING_INPUT
        return;
    } else if (stage.state.stage006.subState == 1) {
        // ========================== STEP_2_WAIT_INPUT: 等待玩家输入 ==========================
        
        // 语音IO恢复逻辑现在由pinManager自动处理，无需手动控制
        // 检查语音IO是否已经自动恢复
        if (stage.state.stage006.voiceTriggered && 
            millis() - stage.state.stage006.voiceTriggerTime >= STAGE_006_0_VOICE_TRIGGER_LOW_TIME) {
            stage.state.stage006.voiceTriggered = false;
            stage.state.stage006.voicePlayedOnce = true;
        }
        
        // 循环播放控制（仅在循环模式下）
        if (STAGE_006_0_VOICE_PLAY_MODE == 1 && // 循环模式
            stage.state.stage006.voicePlayedOnce && // 已播放过一次
            !stage.state.stage006.voiceTriggered && // 当前没有正在播放
            millis() - stage.state.stage006.lastVoiceTime >= STAGE_006_0_VOICE_LOOP_INTERVAL) {
            
            // 重新触发语音播放
            int voiceIndex = (stage.state.stage006.totalCount - 1) % 4;
            int voicePin = (voiceIndex == 0) ? STAGE_006_0_VOICE_IO_1 :
                           (voiceIndex == 1) ? STAGE_006_0_VOICE_IO_3 :
                           (voiceIndex == 2) ? STAGE_006_0_VOICE_IO_2 : 
                                               STAGE_006_0_VOICE_IO_4;
            
            pinManager.setPinTemporaryState(voicePin, LOW, STAGE_006_0_VOICE_TRIGGER_LOW_TIME, HIGH);
            stage.state.stage006.voiceTriggered = true;
            stage.state.stage006.voiceTriggerTime = millis();
            stage.state.stage006.lastVoiceTime = millis();
            stage.state.stage006.voicePlayedOnce = false;
        }
        
        // 按键检测
        if (!stage.state.stage006.buttonPressed) {
            if (!stage.state.stage006.buttonDebouncing) {
                // 不在防抖中，检测按键状态变化
                for (int i = 0; i < 4; i++) {
                    int currentState = digitalRead(C101_TAUNT_BUTTON_COM_PINS[i]);
                    
                    // 检测到按键从HIGH变为LOW（按下）
                    if (stage.state.stage006.lastButtonStates[i] == HIGH && currentState == LOW) {
                        // 启动防抖
                        stage.state.stage006.buttonDebouncing = true;
                        stage.state.stage006.debouncingButton = i;
                        stage.state.stage006.debounceStartTime = millis();
                        break;
                    }
                    
                    // 更新状态记录
                    stage.state.stage006.lastButtonStates[i] = currentState;
                }
            } else {
                // 正在防抖中，检查防抖是否完成
                int buttonIndex = stage.state.stage006.debouncingButton;
                int currentState = digitalRead(C101_TAUNT_BUTTON_COM_PINS[buttonIndex]);
                unsigned long debounceElapsed = millis() - stage.state.stage006.debounceStartTime;
                
                if (currentState == LOW && debounceElapsed >= STAGE_006_0_BUTTON_DEBOUNCE_TIME) {
                    // 防抖完成，确认按键按下
                    Serial.print(F("✅ 按键"));
                    Serial.print(buttonIndex + 1);
                    Serial.println(F("按下"));
                    
                    stage.state.stage006.buttonPressed = true;
                    stage.state.stage006.pressedButton = buttonIndex + 1;
                    stage.state.stage006.buttonDebouncing = false;
                    
                    // 设置按键灯状态：只有按下的按键亮，其他熄灭
                    for (int i = 0; i < 4; i++) {
                        MillisPWM::stopBreathing(C101_TAUNT_BUTTON_LIGHT_PINS[i]);
                        if (i == buttonIndex) {
                            // 按下的按键设为HIGH亮
                            pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[i], HIGH);
                        } else {
                            // 其他按键先停止PWM，再设为LOW
                            MillisPWM::stop(C101_TAUNT_BUTTON_LIGHT_PINS[i]);
                            pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[i], LOW);
                        }
                    }
                    
                    // 判断按键是否正确
                    if (stage.state.stage006.pressedButton == stage.state.stage006.currentCorrectButton) {
                        Serial.println(F("✅ 按键正确！"));
                        stage.state.stage006.correctCount++;
                        
                        // 正确按键 - 点亮对应的植物灯（按键1对应植物灯1）
                        int plantIndex = stage.state.stage006.pressedButton - 1; // 按键1对应植物灯0
                        if (plantIndex >= 0 && plantIndex < 4) {
                            // 先停止PWM，再设置数字状态
                            MillisPWM::stopBreathing(C101_PLANT_LIGHT_PINS[plantIndex]);
                            pinManager.setPinState(C101_PLANT_LIGHT_PINS[plantIndex], HIGH);
                            stage.state.stage006.plantLightStates[plantIndex] = true;
                            
                            Serial.print(F("🌱 植物灯"));
                            Serial.print(plantIndex + 1);
                            Serial.println(F("点亮"));
                        }
                        
                        // 启动植物灯时序呼吸效果（375ms间隔）
                        stage.state.stage006.plantBreathStartTime = millis();
                        stage.state.stage006.plantBreathIndex = 0;
                        stage.state.stage006.plantBreathActive = true;
                        
                        Serial.println(F("🌱 开始植物灯时序呼吸效果"));
                    } else {
                        Serial.println(F("❌ 按键错误！"));
                        
                        // 错误按键 - 停止所有植物灯呼吸并熄灭
                        for (int i = 0; i < 4; i++) {
                            MillisPWM::stopBreathing(C101_PLANT_LIGHT_PINS[i]);
                            MillisPWM::stop(C101_PLANT_LIGHT_PINS[i]);
                            pinManager.setPinState(C101_PLANT_LIGHT_PINS[i], LOW);
                            stage.state.stage006.plantLightStates[i] = false;
                        }
                        
                        stage.state.stage006.correctCount = 0;  // 重置正确计数
                        stage.state.stage006.plantBreathActive = false; // 停止植物灯呼吸时序
                    }
                    
                    // 发送游戏状态通知
                    stage.state.stage006.totalCount++; // m+1
                    int jumpIndex = (stage.state.stage006.totalCount - 1) % 4;
                    String jumpResult = (jumpIndex == 0) ? STAGE_006_0_JUMP_MOD_0 :
                                       (jumpIndex == 1) ? STAGE_006_0_JUMP_MOD_1 :
                                       (jumpIndex == 2) ? STAGE_006_0_JUMP_MOD_2 : 
                                                          STAGE_006_0_JUMP_MOD_3;
                    
                    if (stage.state.stage006.pressedButton == stage.state.stage006.currentCorrectButton) {
                        // 正确按键的消息发送
                        String message = "$[GAME]@C101{^STEP_STATUS^(current_step=\"006_0\",";
                        message += "button_feedback=" + jumpResult + ")}#";
                        
                        Serial.print(F("📤 发送正确命令: "));
                        Serial.println(message);
                        harbingerClient.sendMessage(message);
                        
                        stage.state.stage006.subState = 2; // SUB_CORRECT
                        stage.state.stage006.correctStartTime = millis();
                    } else {
                        // 错误按键的消息发送
                        int errorGroup = ((stage.state.stage006.totalCount - 2) / 2) % 3;
                        String errorJump = (errorGroup == 0) ? STAGE_006_0_ERROR_JUMP_1 :
                                          (errorGroup == 1) ? STAGE_006_0_ERROR_JUMP_2 : 
                                                              STAGE_006_0_ERROR_JUMP_3;
                        
                        String message = "$[GAME]@C101{^STEP_STATUS^(current_step=\"006_0\",";
                        message += "button_feedback=" + jumpResult + ",";
                        message += "error_music=" + errorJump + ")}#";
                        
                        Serial.print(F("📤 发送错误命令: "));
                        Serial.println(message);
                        harbingerClient.sendMessage(message);
                        
                        stage.state.stage006.subState = 3; // SUB_ERROR
                        stage.state.stage006.errorStartTime = millis();
                    }
                } else if (currentState == HIGH) {
                    // 按键在防抖期间被释放，取消防抖
                    stage.state.stage006.buttonDebouncing = false;
                    stage.state.stage006.lastButtonStates[buttonIndex] = HIGH;
                }
            }
        }
        
    } else if (stage.state.stage006.subState == 2) {
        // ========================== STEP_3_PROCESS_CORRECT: 处理正确按键 ==========================
        
        unsigned long correctElapsed = millis() - stage.state.stage006.correctStartTime;
        
        // 检查是否达到成功条件
        if (stage.state.stage006.correctCount >= STAGE_006_0_REQUIRED_CORRECT) {
            Serial.println(F("🎉 游戏成功！达到所需正确数"));
            notifyStageComplete("006_0", STAGE_006_0_SUCCESS_JUMP, elapsed);
            stage.state.stage006.subState = 5; // SUB_SUCCESS
        } else if (correctElapsed >= 1000) {  // 1秒后继续下一轮
            Serial.println(F("🔄 正确处理完成，转入下一轮准备"));
            stage.state.stage006.subState = 4; // SUB_NEXT_ROUND
            stage.state.stage006.errorStartTime = millis();
        }
        
    } else if (stage.state.stage006.subState == 3) {
        // ========================== STEP_4_PROCESS_ERROR: 处理错误按键 ==========================
        
        unsigned long errorElapsed = millis() - stage.state.stage006.errorStartTime;
        
        // 被按下的错误按键也要熄灭（在1125ms时）
        if (errorElapsed >= 1125 && stage.state.stage006.pressedButton > 0) {
            int buttonIndex = stage.state.stage006.pressedButton - 1;
            Serial.print(F("💡 熄灭错误按键"));
            Serial.print(buttonIndex + 1);
            Serial.println(F("灯光"));
            MillisPWM::stopBreathing(C101_TAUNT_BUTTON_LIGHT_PINS[buttonIndex]);
            MillisPWM::stop(C101_TAUNT_BUTTON_LIGHT_PINS[buttonIndex]);
            pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[buttonIndex], LOW);
            stage.state.stage006.pressedButton = 0;  // 标记已处理
        }
        
        // 进入等待状态（增加延迟确保用户看到按键灯熄灭）
        if (errorElapsed >= 2000) {  // 从1125ms改为2000ms，增加延迟
            Serial.println(F("🔄 错误处理完成，转入下一轮准备"));
            stage.state.stage006.subState = 4; // SUB_NEXT_ROUND
            stage.state.stage006.errorStartTime = millis();
        }
        
    } else if (stage.state.stage006.subState == 4) {
        // ========================== STEP_5_NEXT_ROUND: 准备下一轮 ==========================
        
        unsigned long waitElapsed = millis() - stage.state.stage006.errorStartTime;
        
        if (waitElapsed >= STAGE_006_0_ERROR_WAIT_TIME) {
            // 重置按键状态
            stage.state.stage006.buttonPressed = false;
            stage.state.stage006.pressedButton = 0;
            stage.state.stage006.buttonDebouncing = false;
            
            // 重置语音状态 - 确保音频IO恢复HIGH状态
            stage.state.stage006.voiceTriggered = false;
            stage.state.stage006.voiceTriggerTime = 0;
            stage.state.stage006.voicePlayedOnce = false;
            stage.state.stage006.lastVoiceTime = 0;
            
            // 强制重置所有语音IO为HIGH状态
            pinManager.setPinState(STAGE_006_0_VOICE_IO_1, HIGH);
            pinManager.setPinState(STAGE_006_0_VOICE_IO_2, HIGH);
            pinManager.setPinState(STAGE_006_0_VOICE_IO_3, HIGH);
            pinManager.setPinState(STAGE_006_0_VOICE_IO_4, HIGH);
            Serial.println(F("🔄 所有语音IO重置为HIGH状态"));
            
            // 重新启动所有按键呼吸效果
            for (int i = 0; i < 4; i++) {
                MillisPWM::stopBreathing(C101_TAUNT_BUTTON_LIGHT_PINS[i]);
                MillisPWM::stop(C101_TAUNT_BUTTON_LIGHT_PINS[i]);
                pinManager.setPinState(C101_TAUNT_BUTTON_LIGHT_PINS[i], LOW);
                MillisPWM::startBreathing(C101_TAUNT_BUTTON_LIGHT_PINS[i], 3.0);
            }
            Serial.println(F("🔄 所有按键呼吸效果重新启动"));
            
            // 计算下一轮的正确按键和语音IO
            int voiceIndex = (stage.state.stage006.totalCount - 1) % 4;
            stage.state.stage006.currentCorrectButton = (voiceIndex == 0) ? 1 : 
                                                       (voiceIndex == 1) ? 3 : 
                                                       (voiceIndex == 2) ? 2 : 4;
            
            int voicePin = (voiceIndex == 0) ? STAGE_006_0_VOICE_IO_1 :
                           (voiceIndex == 1) ? STAGE_006_0_VOICE_IO_3 :
                           (voiceIndex == 2) ? STAGE_006_0_VOICE_IO_2 : 
                                               STAGE_006_0_VOICE_IO_4;
            
            Serial.print(F("🎵 播放语音IO"));
            Serial.print(voiceIndex + 1);
            Serial.print(F("，正确按键="));
            Serial.println(stage.state.stage006.currentCorrectButton);
            
            // 触发下一轮语音
            pinManager.setPinTemporaryState(voicePin, LOW, STAGE_006_0_VOICE_TRIGGER_LOW_TIME, HIGH);
            stage.state.stage006.voiceTriggered = true;
            stage.state.stage006.voiceTriggerTime = millis();
            stage.state.stage006.lastVoiceTime = millis();
            
            // 转入等待输入状态
            stage.state.stage006.subState = 1; // SUB_WAITING_INPUT
            Serial.println(F("�� 准备完成，返回等待输入状态"));
        }
        
    } else if (stage.state.stage006.subState == 5) {
        // ========================== STEP_6_SUCCESS: 游戏成功 ==========================
        
        // 游戏成功，不需要更新逻辑，等待跳转
        return;
    }

    // ========================== 植物灯时序呼吸效果处理 ==========================
    if (stage.state.stage006.plantBreathActive) {
        unsigned long breathElapsed = millis() - stage.state.stage006.plantBreathStartTime;
        unsigned long currentCheckTime = stage.state.stage006.plantBreathIndex * 375; // 每375ms检查一次
        
        if (breathElapsed >= currentCheckTime) {
            int currentPlantIndex = stage.state.stage006.plantBreathIndex;
            
            if (currentPlantIndex < 4) {
                // 检查当前植物灯是否应该呼吸
                if (stage.state.stage006.plantLightStates[currentPlantIndex]) {
                    // 该植物灯已点亮，启动呼吸效果
                    MillisPWM::stopBreathing(C101_PLANT_LIGHT_PINS[currentPlantIndex]);
                    MillisPWM::startBreathing(C101_PLANT_LIGHT_PINS[currentPlantIndex], 3.0);
                    Serial.print(F("🌱 植物灯"));
                    Serial.print(currentPlantIndex + 1);
                    Serial.println(F("开始呼吸"));
                } else {
                    // 该植物灯未点亮，确保停止呼吸并设为LOW
                    MillisPWM::stopBreathing(C101_PLANT_LIGHT_PINS[currentPlantIndex]);
                    MillisPWM::stop(C101_PLANT_LIGHT_PINS[currentPlantIndex]);
                    pinManager.setPinState(C101_PLANT_LIGHT_PINS[currentPlantIndex], LOW);
                }
                
                stage.state.stage006.plantBreathIndex++;
            } else {
                // 所有植物灯都检查完毕，停止时序呼吸
                stage.state.stage006.plantBreathActive = false;
                Serial.println(F("🌱 植物灯时序呼吸效果完成"));
            }
        }
    }

    // ========================== 按键防抖处理 ==========================
}

// 更新单个环节
void GameFlowManager::updateStage(int index) {
    if (index < 0 || index >= MAX_PARALLEL_STAGES || !stages[index].running) {
        return;
    }
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    const String& stageId = stages[index].stageId;
    
    // 根据环节ID调用对应的更新方法
    if (stageId == "000_0") {
        updateStep000(index);
    } else if (stageId == "001_1") {
        updateStep001_1(index);
    } else if (stageId == "001_2") {
        updateStep001_2(index);
    } else if (stageId == "002_0") {
        updateStep002(index);
    } else if (stageId == "006_0") {
        updateStep006(index);
    }
    
    // 更新兼容性变量
    updateCompatibilityVars();
}

// 检查紧急开门功能
void GameFlowManager::checkEmergencyDoorControl() {
    // ========================== 紧急开门控制 (最高优先级) ==========================
    // 无视任何步骤，只要Pin24触发，就让Pin26解锁10秒
    updateEmergencyDoorControl();
}