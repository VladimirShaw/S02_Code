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

void GameFlowManager::begin() {
    Serial.println(F("C101 GameFlowManager初始化完成（支持并行环节）"));
    Serial.print(F("最大并行环节数: "));
    Serial.println(MAX_PARALLEL_STAGES);
    
    // 初始化所有通道音量为默认值
    initializeAllVolumes();
    
    // 初始化紧急开门功能
    initEmergencyDoorControl();
    
    // 初始化门锁和灯光状态
    Serial.println(F("🎮 初始化门锁和灯光状态"));
    resetDoorAndLightState();
    Serial.println(F("✅ 门锁和灯光状态初始化完成"));
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
        digitalWrite(C101_DOOR_LOCK_PIN, STAGE_000_0_DOOR_LOCK_STATE);
        digitalWrite(C101_DOOR_LIGHT_PIN, STAGE_000_0_DOOR_LIGHT_STATE);
        
        // 氛围射灯系统
        digitalWrite(C101_AMBIENT_LIGHT_PIN, STAGE_000_0_AMBIENT_LIGHT_STATE);
        
        // 嘲讽按键灯光系统
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[0], STAGE_000_0_TAUNT_BUTTON1_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[1], STAGE_000_0_TAUNT_BUTTON2_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[2], STAGE_000_0_TAUNT_BUTTON3_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[3], STAGE_000_0_TAUNT_BUTTON4_STATE);
        
        // 画灯谜题系统
        digitalWrite(C101_PAINTING_LIGHT_PINS[0], STAGE_000_0_PAINTING_LIGHT1_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[1], STAGE_000_0_PAINTING_LIGHT2_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[2], STAGE_000_0_PAINTING_LIGHT3_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[3], STAGE_000_0_PAINTING_LIGHT4_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[4], STAGE_000_0_PAINTING_LIGHT5_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[5], STAGE_000_0_PAINTING_LIGHT6_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[6], STAGE_000_0_PAINTING_LIGHT7_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[7], STAGE_000_0_PAINTING_LIGHT8_STATE);
        
        // 提示灯带系统
        digitalWrite(C101_HINT_LED_PINS[0], STAGE_000_0_HINT_LED1_STATE);
        digitalWrite(C101_HINT_LED_PINS[1], STAGE_000_0_HINT_LED2_STATE);
        
        // 蝴蝶灯谜题系统
        digitalWrite(C101_BUTTERFLY_CARD_RELAY_PIN, STAGE_000_0_BUTTERFLY_CARD_STATE);
        digitalWrite(C101_BUTTERFLY_LIGHT_PIN, STAGE_000_0_BUTTERFLY_LIGHT_STATE);
        digitalWrite(C101_AD_FAN_PIN, STAGE_000_0_AD_FAN_STATE);
        
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
        digitalWrite(C101_DOOR_LOCK_PIN, STAGE_001_1_DOOR_LOCK_STATE);
        digitalWrite(C101_DOOR_LIGHT_PIN, STAGE_001_1_DOOR_LIGHT_STATE);
        
        // 氛围射灯系统
        digitalWrite(C101_AMBIENT_LIGHT_PIN, STAGE_001_1_AMBIENT_LIGHT_STATE);
        
        // 嘲讽按键灯光系统
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[0], STAGE_001_1_TAUNT_BUTTON1_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[1], STAGE_001_1_TAUNT_BUTTON2_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[2], STAGE_001_1_TAUNT_BUTTON3_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[3], STAGE_001_1_TAUNT_BUTTON4_STATE);
        
        // 画灯谜题系统
        digitalWrite(C101_PAINTING_LIGHT_PINS[0], STAGE_001_1_PAINTING_LIGHT1_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[1], STAGE_001_1_PAINTING_LIGHT2_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[2], STAGE_001_1_PAINTING_LIGHT3_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[3], STAGE_001_1_PAINTING_LIGHT4_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[4], STAGE_001_1_PAINTING_LIGHT5_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[5], STAGE_001_1_PAINTING_LIGHT6_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[6], STAGE_001_1_PAINTING_LIGHT7_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[7], STAGE_001_1_PAINTING_LIGHT8_STATE);
        
        // 提示灯带系统
        digitalWrite(C101_HINT_LED_PINS[0], STAGE_001_1_HINT_LED1_STATE);
        digitalWrite(C101_HINT_LED_PINS[1], STAGE_001_1_HINT_LED2_STATE);
        
        // 蝴蝶灯谜题系统
        digitalWrite(C101_BUTTERFLY_CARD_RELAY_PIN, STAGE_001_1_BUTTERFLY_CARD_STATE);
        digitalWrite(C101_BUTTERFLY_LIGHT_PIN, STAGE_001_1_BUTTERFLY_LIGHT_STATE);
        digitalWrite(C101_AD_FAN_PIN, STAGE_001_1_AD_FAN_STATE);
        
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
        digitalWrite(C101_DOOR_LOCK_PIN, STAGE_001_2_DOOR_LOCK_STATE);
        digitalWrite(C101_DOOR_LIGHT_PIN, STAGE_001_2_DOOR_LIGHT_STATE);
        Serial.print(F("🔒 电磁锁"));
        Serial.print(STAGE_001_2_DOOR_LOCK_STATE ? "上锁" : "解锁");
        Serial.println(F(" (Pin26)"));
        
        // 氛围射灯系统
        digitalWrite(C101_AMBIENT_LIGHT_PIN, STAGE_001_2_AMBIENT_LIGHT_STATE);
        
        // 嘲讽按键灯光系统
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[0], STAGE_001_2_TAUNT_BUTTON1_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[1], STAGE_001_2_TAUNT_BUTTON2_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[2], STAGE_001_2_TAUNT_BUTTON3_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[3], STAGE_001_2_TAUNT_BUTTON4_STATE);
        
        // 画灯谜题系统
        digitalWrite(C101_PAINTING_LIGHT_PINS[0], STAGE_001_2_PAINTING_LIGHT1_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[1], STAGE_001_2_PAINTING_LIGHT2_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[2], STAGE_001_2_PAINTING_LIGHT3_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[3], STAGE_001_2_PAINTING_LIGHT4_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[4], STAGE_001_2_PAINTING_LIGHT5_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[5], STAGE_001_2_PAINTING_LIGHT6_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[6], STAGE_001_2_PAINTING_LIGHT7_STATE);
        digitalWrite(C101_PAINTING_LIGHT_PINS[7], STAGE_001_2_PAINTING_LIGHT8_STATE);
        
        // 提示灯带系统
        digitalWrite(C101_HINT_LED_PINS[0], STAGE_001_2_HINT_LED1_STATE);
        digitalWrite(C101_HINT_LED_PINS[1], STAGE_001_2_HINT_LED2_STATE);
        
        // 蝴蝶灯谜题系统
        digitalWrite(C101_BUTTERFLY_CARD_RELAY_PIN, STAGE_001_2_BUTTERFLY_CARD_STATE);
        digitalWrite(C101_BUTTERFLY_LIGHT_PIN, STAGE_001_2_BUTTERFLY_LIGHT_STATE);
        digitalWrite(C101_AD_FAN_PIN, STAGE_001_2_AD_FAN_STATE);
        
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
        digitalWrite(C101_DOOR_LOCK_PIN, STAGE_002_0_DOOR_LOCK_STATE);
        digitalWrite(C101_DOOR_LIGHT_PIN, STAGE_002_0_DOOR_LIGHT_STATE);
        
        // 氛围射灯系统
        digitalWrite(C101_AMBIENT_LIGHT_PIN, STAGE_002_0_AMBIENT_LIGHT_STATE);
        
        // 嘲讽按键灯光系统
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[0], STAGE_002_0_TAUNT_BUTTON1_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[1], STAGE_002_0_TAUNT_BUTTON2_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[2], STAGE_002_0_TAUNT_BUTTON3_STATE);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[3], STAGE_002_0_TAUNT_BUTTON4_STATE);
        
        // 画灯谜题系统 - 初始化为关闭状态，由呼吸和闪烁效果动态控制
        digitalWrite(C101_PAINTING_LIGHT_PINS[0], STAGE_002_0_PAINTING_LIGHT1_STATE);  // 画1：不参与效果
        digitalWrite(C101_PAINTING_LIGHT_PINS[1], STAGE_002_0_PAINTING_LIGHT2_STATE);  // 画2：呼吸+闪烁
        digitalWrite(C101_PAINTING_LIGHT_PINS[2], STAGE_002_0_PAINTING_LIGHT3_STATE);  // 画3：不参与效果
        digitalWrite(C101_PAINTING_LIGHT_PINS[3], STAGE_002_0_PAINTING_LIGHT4_STATE);  // 画4：呼吸+闪烁
        digitalWrite(C101_PAINTING_LIGHT_PINS[4], STAGE_002_0_PAINTING_LIGHT5_STATE);  // 画5：不参与效果
        digitalWrite(C101_PAINTING_LIGHT_PINS[5], STAGE_002_0_PAINTING_LIGHT6_STATE);  // 画6：闪烁
        digitalWrite(C101_PAINTING_LIGHT_PINS[6], STAGE_002_0_PAINTING_LIGHT7_STATE);  // 画7：不参与效果
        digitalWrite(C101_PAINTING_LIGHT_PINS[7], STAGE_002_0_PAINTING_LIGHT8_STATE);  // 画8：呼吸+闪烁
        
        // 提示灯带系统
        digitalWrite(C101_HINT_LED_PINS[0], STAGE_002_0_HINT_LED1_STATE);
        digitalWrite(C101_HINT_LED_PINS[1], STAGE_002_0_HINT_LED2_STATE);
        
        // 蝴蝶灯谜题系统
        digitalWrite(C101_BUTTERFLY_CARD_RELAY_PIN, STAGE_002_0_BUTTERFLY_CARD_STATE);
        digitalWrite(C101_BUTTERFLY_LIGHT_PIN, STAGE_002_0_BUTTERFLY_LIGHT_STATE);
        digitalWrite(C101_AD_FAN_PIN, STAGE_002_0_AD_FAN_STATE);
        
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
        digitalWrite(C101_PLANT_LIGHT_PINS[i], LOW);
    }
    Serial.println(F("💡 所有植物灯效果已停止"));
    
    // 停止所有画灯效果（002_0环节相关）
    for (int i = 0; i < C101_PAINTING_LIGHT_COUNT; i++) {
        MillisPWM::stopBreathing(C101_PAINTING_LIGHT_PINS[i]);
        digitalWrite(C101_PAINTING_LIGHT_PINS[i], LOW);
    }
    Serial.println(F("🎨 所有画灯效果已停止"));
    
    // 重置所有环节状态
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        if (stages[i].running) {
            Serial.print(F("⏹️ 停止环节[槽位"));
            Serial.print(i);
            Serial.print(F("]: "));
            Serial.println(stages[i].stageId);
        }
        stages[i].running = false;
        stages[i].stageId = "";
    }
    activeStageCount = 0;
    updateCompatibilityVars();
    
    Serial.println(F("✅ 所有C101环节已停止"));
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
            normalizedId == "002_0");
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
    
    Serial.println(F("=============================="));
}

// ========================== 更新和调试功能 ==========================
void GameFlowManager::update() {
    // ========================== 紧急开门控制 (最高优先级) ==========================
    // 无视任何步骤，只要Pin24触发，就让Pin26解锁10秒
    updateEmergencyDoorControl();
    
    if (activeStageCount == 0) {
        return;
    }
    
    // 检查全局停止标志
    if (globalStopped) {
        return;
    }
    
    // 更新所有运行中的环节
    for (int i = 0; i < MAX_PARALLEL_STAGES; i++) {
        if (stages[i].running) {
            const String& stageId = stages[i].stageId;
            
            // 根据环节ID调用对应的更新方法
            if (stageId == "000_0") {
                updateStep000(i);
            } else if (stageId == "001_1") {
                updateStep001_1(i);
            } else if (stageId == "001_2") {
                updateStep001_2(i);
            } else if (stageId == "002_0") {
                updateStep002(i);
            }
        }
    }
    
    // 更新兼容性变量
    updateCompatibilityVars();
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
    Serial.print(F("📤 请求从"));
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
        digitalWrite(C101_DOOR_LOCK_PIN, LOW);   // Pin26解锁（断电）
        emergencyUnlockStartTime = millis();
        emergencyUnlockActive = true;
        
        Serial.println(F("🔓 电磁锁已解锁，10秒后自动上锁"));
    }
    lastCardReaderState = currentCardReaderState;
    
    // 检查紧急解锁超时
    if (emergencyUnlockActive && (millis() - emergencyUnlockStartTime >= EMERGENCY_UNLOCK_DURATION)) {
        digitalWrite(C101_DOOR_LOCK_PIN, HIGH);  // Pin26上锁（通电）
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
        digitalWrite(C101_DOOR_LOCK_PIN, HIGH);   // Pin26电磁锁上锁（通电）
        Serial.println(F("🔒 电磁锁已上锁"));
    } else {
        Serial.println(F("⚠️ 紧急解锁激活中，跳过门锁重置"));
    }
    
    digitalWrite(C101_DOOR_LIGHT_PIN, LOW);       // Pin25指引射灯关闭
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
            digitalWrite(C101_PLANT_LIGHT_PINS[i], LOW);    // 设置为低电平
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
                digitalWrite(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_4_INDEX], LOW);
                digitalWrite(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_8_INDEX], LOW);
                Serial.print(F("⚡ [循环"));
                Serial.print(currentCycle + 1);
                Serial.println(F("] 开始画4长+画8长闪烁"));
            } else if (currentFlashGroup == 1 || currentFlashGroup == 3) {
                // 画2长+画6长闪烁组：停止PWM并清理灯光
                MillisPWM::stop(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_2_INDEX]);
                MillisPWM::stop(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_6_INDEX]);
                digitalWrite(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_2_INDEX], LOW);
                digitalWrite(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_6_INDEX], LOW);
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
                    digitalWrite(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_4_INDEX], stage.state.stage002.flashState ? HIGH : LOW);
                    digitalWrite(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_8_INDEX], stage.state.stage002.flashState ? HIGH : LOW);
                } else if (currentFlashGroup == 1 || currentFlashGroup == 3) {
                    // 画2长+画6长闪烁
                    digitalWrite(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_2_INDEX], stage.state.stage002.flashState ? HIGH : LOW);
                    digitalWrite(C101_PAINTING_LIGHT_PINS[STAGE_002_0_PAINTING_LIGHT_6_INDEX], stage.state.stage002.flashState ? HIGH : LOW);
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
            Serial.print(F("⏰ [C101-槽位"));
            Serial.print(index);
            Serial.print(F("] 环节002_0完成，跳转到"));
            Serial.println(STAGE_002_0_NEXT_STAGE);
            notifyStageComplete("002_0", STAGE_002_0_NEXT_STAGE, elapsed);
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