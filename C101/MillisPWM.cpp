/**
 * =============================================================================
 * MillisPWM - 基于millis()的超低CPU占用PWM库 - 实现文件
 * 版本: 1.0
 * 创建日期: 2024-12-28
 * =============================================================================
 */

#include "MillisPWM.h"

// 时间源函数指针定义
unsigned long (*MillisTimeSource::getTime)() = nullptr;

// 静态成员变量定义
PWMChannel MillisPWM::channels[MPWM_MAX_CHANNELS];
uint8_t MillisPWM::breathingTable[MPWM_BREATHING_TABLE_SIZE];
bool MillisPWM::initialized = false;
int MillisPWM::channelCount = 0;

// 性能统计
static unsigned long updateCount = 0;

// ========================== PWMChannel 实现 ==========================

PWMChannel::PWMChannel() : pin(-1), dutyCycle(0), pwmPeriod(MPWM_DEFAULT_PERIOD), 
                           isActive(false), currentState(false), breathingEnabled(false),
                           breathingCyclePeriod(2000), breathingStartTime(0),
                           breathingUpdateInterval(50), fadeEnabled(false),
                           fadeStartValue(0), fadeTargetValue(0), fadeDuration(1000),
                           fadeStartTime(0), fadeLastUpdate(0), unstableEnabled(false),
                           baseVoltage(180), currentVoltage(180), targetVoltage(180),
                           flickerIntensity(0), inDropout(false), instabilityLevel(3) {
    updateTiming();
    lastVoltageChange = 0;
    lastFlicker = 0;
    lastRandomShift = 0;
    dropoutStart = 0;
}

void PWMChannel::updateTiming() {
    onTime = (pwmPeriod * dutyCycle) / 255;
}

bool PWMChannel::start(int p, uint8_t duty, unsigned long periodMs) {
    pin = (int8_t)p;  // 转换为int8_t
    dutyCycle = duty;
    pwmPeriod = (uint16_t)min(periodMs, 65535UL);  // 限制在uint16_t范围内
    updateTiming();
    pinMode(pin, OUTPUT);
    isActive = true;
    lastToggle = MillisTimeSource::getCurrentTime();
    currentState = false;
    digitalWrite(pin, LOW);
    return true;
}

void PWMChannel::stop() {
    if (isActive && pin >= 0) {
        digitalWrite(pin, LOW);
        isActive = false;
        breathingEnabled = false;
    }
}

void PWMChannel::setDutyCycle(uint8_t duty) {
    if (dutyCycle != duty) {
        dutyCycle = duty;
        updateTiming();
    }
}

void PWMChannel::setPeriod(unsigned long periodMs) {
    pwmPeriod = (uint16_t)min(periodMs, 65535UL);  // 限制在uint16_t范围内
    updateTiming();
}

void PWMChannel::startBreathing(unsigned long cyclePeriodMs, unsigned long startDelayMs) {
    breathingEnabled = true;
    breathingCyclePeriod = (uint16_t)min(cyclePeriodMs, 65535UL);  // 限制在uint16_t范围内
    breathingStartTime = MillisTimeSource::getCurrentTime() + startDelayMs;
    breathingLastUpdate = 0;
}

void PWMChannel::stopBreathing() {
    breathingEnabled = false;
}

// ========================== Fade渐变功能实现 ==========================
void PWMChannel::fadeIn(uint8_t targetValue, unsigned long durationMs) {
    if (!isActive) return;
    
    fadeEnabled = true;
    fadeStartValue = 0;  // 从0开始
    fadeTargetValue = targetValue;
    fadeDuration = (uint16_t)min(durationMs, 65535UL);
    fadeStartTime = MillisTimeSource::getCurrentTime();
    fadeLastUpdate = fadeStartTime;
    
    // 立即设置为起始值
    setDutyCycle(fadeStartValue);
}

void PWMChannel::fadeOut(unsigned long durationMs) {
    if (!isActive) return;
    
    fadeEnabled = true;
    fadeStartValue = dutyCycle;  // 从当前亮度开始
    fadeTargetValue = 0;         // 渐变到0
    fadeDuration = (uint16_t)min(durationMs, 65535UL);
    fadeStartTime = MillisTimeSource::getCurrentTime();
    fadeLastUpdate = fadeStartTime;
}

void PWMChannel::fadeTo(uint8_t targetValue, unsigned long durationMs) {
    if (!isActive) return;
    
    fadeEnabled = true;
    fadeStartValue = dutyCycle;  // 从当前亮度开始
    fadeTargetValue = targetValue;
    fadeDuration = (uint16_t)min(durationMs, 65535UL);
    fadeStartTime = MillisTimeSource::getCurrentTime();
    fadeLastUpdate = fadeStartTime;
}

void PWMChannel::stopFade() {
    fadeEnabled = false;
}

void PWMChannel::updateFade() {
    if (!fadeEnabled || !isActive) return;
    
    unsigned long now = MillisTimeSource::getCurrentTime();
    unsigned long elapsed = now - fadeStartTime;
    
    // 检查fade是否完成
    if (elapsed >= fadeDuration) {
        setDutyCycle(fadeTargetValue);
        fadeEnabled = false;  // fade完成
        return;
    }
    
    // 计算当前应该的亮度值 (线性插值)
    uint8_t currentValue;
    if (fadeStartValue < fadeTargetValue) {
        // 渐亮
        uint16_t range = fadeTargetValue - fadeStartValue;
        uint16_t progress = (elapsed * range) / fadeDuration;
        currentValue = fadeStartValue + progress;
    } else {
        // 渐暗
        uint16_t range = fadeStartValue - fadeTargetValue;
        uint16_t progress = (elapsed * range) / fadeDuration;
        currentValue = fadeStartValue - progress;
    }
    
    setDutyCycle(currentValue);
}

void PWMChannel::startUnstable(uint8_t baseVolt, uint8_t level) {
    unstableEnabled = true;
    baseVoltage = baseVolt;
    currentVoltage = baseVolt;
    targetVoltage = baseVolt;
    instabilityLevel = constrain(level, 1, 5);
    flickerIntensity = 0;
    inDropout = false;
    lastVoltageChange = MillisTimeSource::getCurrentTime();
    lastFlicker = MillisTimeSource::getCurrentTime();
    lastRandomShift = MillisTimeSource::getCurrentTime();
    dropoutStart = 0;
}

void PWMChannel::stopUnstable() {
    unstableEnabled = false;
}

void PWMChannel::setInstabilityLevel(uint8_t level) {
    instabilityLevel = constrain(level, 1, 5);
}

void PWMChannel::updateBreathing() {
    if (!breathingEnabled || !isActive) return;
    
    unsigned long now = MillisTimeSource::getCurrentTime();
    
    // 检查是否到了启动时间
    if (now < breathingStartTime) return;
    
    if (now - breathingLastUpdate >= breathingUpdateInterval) {
        breathingLastUpdate = now;
        
        // 计算呼吸周期位置
        unsigned long cycleTime = (now - breathingStartTime) % breathingCyclePeriod;
        uint8_t tableIndex = (cycleTime * MPWM_BREATHING_TABLE_SIZE) / breathingCyclePeriod;
        tableIndex = constrain(tableIndex, 0, MPWM_BREATHING_TABLE_SIZE - 1);
        
        // 从查找表获取亮度
        uint8_t brightness = MillisPWM::getBreathingValue(tableIndex);
        setDutyCycle(brightness);
    }
}

void PWMChannel::updateUnstable() {
    if (!unstableEnabled || !isActive) return;
    
    unsigned long now = MillisTimeSource::getCurrentTime();
    
    // 根据不稳定程度调整频率和强度
    uint8_t dropoutChance = instabilityLevel * 2;      // 2% 4% 6% 8% 10%
    uint8_t flickerChance = instabilityLevel * 3;      // 3% 6% 9% 12% 15%
    uint8_t waveChance = instabilityLevel * 4;         // 4% 8% 12% 16% 20%
    uint8_t blackoutChance = instabilityLevel;         // 0.1% 0.2% 0.3% 0.4% 0.5%
    
    // 电压掉落事件处理
    if (inDropout) {
        if (now - dropoutStart > random(300, 1200)) {
            inDropout = false;
            targetVoltage = baseVoltage + random(-10, 20);
        }
    } else if (random(1000) < dropoutChance) {
        inDropout = true;
        dropoutStart = now;
        targetVoltage = random(20, 90);
    }
    
    // 快速闪烁
    if (now - lastFlicker > random(10, 30)) {
        lastFlicker = now;
        if (random(100) < flickerChance) {
            flickerIntensity = random(30, 80);
        } else {
            flickerIntensity = 0;
        }
    }
    
    // 基础电压变化
    if (now - lastRandomShift > random(1000, 3000)) {
        lastRandomShift = now;
        if (random(100) < waveChance) {
            targetVoltage = baseVoltage + random(-60, 60);
        } else {
            targetVoltage = baseVoltage + random(-15, 15);
        }
        targetVoltage = constrain(targetVoltage, 20, 240);
    }
    
    // 瞬间断电
    if (random(10000) < blackoutChance) {
        currentVoltage = 0;
        setDutyCycle(0);
        // 🚀 优化：使用非阻塞延迟而不是delay()
        static unsigned long blackoutStart = 0;
        if (blackoutStart == 0) {
            blackoutStart = now;
        } else if (now - blackoutStart > random(10, 100)) {
            blackoutStart = 0;  // 重置，允许继续处理
        }
        return;
    }
    
    // 电压变化
    if (now - lastVoltageChange > random(50, 200)) {
        lastVoltageChange = now;
        
        // 逐步接近目标电压
        if (currentVoltage < targetVoltage) {
            currentVoltage += random(1, 5);
            if (currentVoltage > targetVoltage) currentVoltage = targetVoltage;
        } else if (currentVoltage > targetVoltage) {
            currentVoltage -= random(1, 5);
            if (currentVoltage < targetVoltage) currentVoltage = targetVoltage;
        }
        
        // 添加闪烁效果
        uint8_t finalVoltage = currentVoltage + flickerIntensity;
        
        // 高频噪声
        if (random(100) < 30) {
            finalVoltage += random(-2, 3);
        }
        
        finalVoltage = constrain(finalVoltage, 0, 255);
        setDutyCycle(finalVoltage);
    }
}

void PWMChannel::update() {
    if (!isActive || pin < 0) return;
    
    // 先更新fade渐变
    updateFade();
    
    // 更新呼吸灯 (如果fade未启用)
    if (!fadeEnabled) {
        updateBreathing();
    }
    
    // 更新电压不稳定 (如果fade和breathing都未启用)
    if (!fadeEnabled && !breathingEnabled) {
        updateUnstable();
    }
    
    // PWM控制逻辑
    if (dutyCycle == 0) {
        if (currentState) {
            currentState = false;
            digitalWrite(pin, LOW);
        }
        return;
    }
    
    if (dutyCycle == 255) {
        if (!currentState) {
            currentState = true;
            digitalWrite(pin, HIGH);
        }
        return;
    }
    
    unsigned long now = MillisTimeSource::getCurrentTime();
    unsigned long elapsed = now - lastToggle;
    
    if (elapsed >= pwmPeriod) {
        lastToggle = now;
        if (onTime > 0) {
            currentState = true;
            digitalWrite(pin, HIGH);
        } else {
            currentState = false;
            digitalWrite(pin, LOW);
        }
    } else if (elapsed >= onTime && currentState) {
        currentState = false;
        digitalWrite(pin, LOW);
    }
}

// 状态查询
bool PWMChannel::getIsActive() const { return isActive; }
uint8_t PWMChannel::getDutyCycle() const { return dutyCycle; }
unsigned long PWMChannel::getPeriod() const { return pwmPeriod; }
bool PWMChannel::isBreathing() const { return breathingEnabled; }
bool PWMChannel::isUnstable() const { return unstableEnabled; }
bool PWMChannel::isFading() const { return fadeEnabled; }

// ========================== MillisPWM 实现 ==========================

void MillisPWM::begin() {
    if (!initialized) {
        generateBreathingTable();
        channelCount = 0;
        initialized = true;
    }
}

void MillisPWM::generateBreathingTable() {
    for (int i = 0; i < MPWM_BREATHING_TABLE_SIZE; i++) {
        float progress = (float)i / (MPWM_BREATHING_TABLE_SIZE - 1);
        
        // 简化的平滑呼吸曲线
        float brightness;
        if (progress <= 0.5) {
            brightness = progress * 2.0;
        } else {
            brightness = 2.0 - progress * 2.0;
        }
        
        // 轻微平滑
        brightness = brightness * brightness * (3.0 - 2.0 * brightness);
        breathingTable[i] = (uint8_t)(brightness * 255);
    }
}

int MillisPWM::findChannelByPin(int pin) {
    for (int i = 0; i < channelCount; i++) {
        if (channels[i].pin == pin && channels[i].getIsActive()) {
            return i;
        }
    }
    return -1;
}

bool MillisPWM::start(int pin, uint8_t dutyCycle) {
    return start(pin, dutyCycle, MPWM_DEFAULT_PERIOD);
}

bool MillisPWM::start(int pin, uint8_t dutyCycle, unsigned long periodMs) {
    if (!initialized) begin();
    
    // 查找现有通道
    int channelIndex = findChannelByPin(pin);
    
    if (channelIndex >= 0) {
        // 更新现有通道
        channels[channelIndex].setDutyCycle(dutyCycle);
        channels[channelIndex].setPeriod(periodMs);
        return true;
    }
    
    // 创建新通道
    if (channelCount < MPWM_MAX_CHANNELS) {
        channels[channelCount].start(pin, dutyCycle, periodMs);
        channelCount++;
        return true;
    }
    
    return false; // 通道已满
}

void MillisPWM::stop(int pin) {
    int channelIndex = findChannelByPin(pin);
    if (channelIndex >= 0) {
        channels[channelIndex].stop();
    }
}

void MillisPWM::stopAll() {
    for (int i = 0; i < channelCount; i++) {
        channels[i].stop();
    }
    // 🔧 关键修复：重置通道计数，清空所有通道槽位
    channelCount = 0;
}

void MillisPWM::setBrightness(int pin, uint8_t brightness) {
    // 如果PWM通道不存在，先创建
    if (findChannelByPin(pin) < 0) {
        if (!start(pin, brightness)) return;  // 创建失败则返回
    }
    
    int channelIndex = findChannelByPin(pin);
    if (channelIndex >= 0) {
        channels[channelIndex].stopBreathing(); // 停止呼吸模式
        channels[channelIndex].setDutyCycle(brightness);
    }
}

void MillisPWM::setBrightnessPercent(int pin, float percentage) {
    uint8_t brightness = (uint8_t)constrain(percentage * 2.55, 0, 255);
    setBrightness(pin, brightness);
}

bool MillisPWM::startBreathing(int pin, float cyclePeriodSeconds) {
    return startBreathing(pin, cyclePeriodSeconds, 0.0);
}

bool MillisPWM::startBreathing(int pin, float cyclePeriodSeconds, float startDelaySeconds) {
    // 如果PWM通道不存在，先创建
    if (findChannelByPin(pin) < 0) {
        if (!start(pin, 128)) return false;
    }
    
    int channelIndex = findChannelByPin(pin);
    if (channelIndex >= 0) {
        unsigned long cyclePeriodMs = (unsigned long)(cyclePeriodSeconds * 1000);
        unsigned long startDelayMs = (unsigned long)(startDelaySeconds * 1000);
        channels[channelIndex].startBreathing(cyclePeriodMs, startDelayMs);
        return true;
    }
    
    return false;
}

void MillisPWM::stopBreathing(int pin) {
    int channelIndex = findChannelByPin(pin);
    if (channelIndex >= 0) {
        channels[channelIndex].stopBreathing();
    }
}

// ========================== Fade渐变控制静态方法 ==========================
bool MillisPWM::fadeIn(int pin, uint8_t targetValue, unsigned long durationMs) {
    // 如果PWM通道不存在，先创建
    if (findChannelByPin(pin) < 0) {
        if (!start(pin, 0)) return false;  // 以0亮度开始
    }
    
    int channelIndex = findChannelByPin(pin);
    if (channelIndex >= 0) {
        channels[channelIndex].stopBreathing(); // 停止呼吸模式
        channels[channelIndex].stopFade();      // 停止之前的fade
        channels[channelIndex].fadeIn(targetValue, durationMs);
        return true;
    }
    
    return false;
}

bool MillisPWM::fadeOut(int pin, unsigned long durationMs) {
    // 如果PWM通道不存在，先创建并设置为当前引脚状态
    if (findChannelByPin(pin) < 0) {
        // 读取引脚当前状态，如果是HIGH则设为255，否则设为0
        pinMode(pin, INPUT);
        uint8_t currentState = digitalRead(pin) ? 255 : 0;
        pinMode(pin, OUTPUT);
        if (!start(pin, currentState)) return false;
    }
    
    int channelIndex = findChannelByPin(pin);
    if (channelIndex >= 0) {
        channels[channelIndex].stopBreathing(); // 停止呼吸模式
        channels[channelIndex].stopFade();      // 停止之前的fade
        channels[channelIndex].fadeOut(durationMs);
        return true;
    }
    
    return false;
}

bool MillisPWM::fadeTo(int pin, uint8_t targetValue, unsigned long durationMs) {
    // 如果PWM通道不存在，先创建并设置为当前引脚状态
    if (findChannelByPin(pin) < 0) {
        // 读取引脚当前状态，如果是HIGH则设为255，否则设为0
        pinMode(pin, INPUT);
        uint8_t currentState = digitalRead(pin) ? 255 : 0;
        pinMode(pin, OUTPUT);
        if (!start(pin, currentState)) return false;
    }
    
    int channelIndex = findChannelByPin(pin);
    if (channelIndex >= 0) {
        channels[channelIndex].stopBreathing(); // 停止呼吸模式
        channels[channelIndex].stopFade();      // 停止之前的fade
        channels[channelIndex].fadeTo(targetValue, durationMs);
        return true;
    }
    
    return false;
}

void MillisPWM::stopFade(int pin) {
    int channelIndex = findChannelByPin(pin);
    if (channelIndex >= 0) {
        channels[channelIndex].stopFade();
    }
}

bool MillisPWM::startUnstable(int pin, uint8_t baseVoltage, uint8_t instabilityLevel) {
    // 如果PWM通道不存在，先创建
    if (findChannelByPin(pin) < 0) {
        if (!start(pin, baseVoltage)) return false;
    }
    
    int channelIndex = findChannelByPin(pin);
    if (channelIndex >= 0) {
        channels[channelIndex].stopBreathing(); // 停止呼吸模式
        channels[channelIndex].startUnstable(baseVoltage, instabilityLevel);
        return true;
    }
    
    return false;
}

void MillisPWM::stopUnstable(int pin) {
    int channelIndex = findChannelByPin(pin);
    if (channelIndex >= 0) {
        channels[channelIndex].stopUnstable();
    }
}

void MillisPWM::setInstabilityLevel(int pin, uint8_t level) {
    int channelIndex = findChannelByPin(pin);
    if (channelIndex >= 0) {
        channels[channelIndex].setInstabilityLevel(level);
    }
}

void MillisPWM::startAllBreathing(int* pins, int count, float minCycle, float maxCycle) {
    for (int i = 0; i < count; i++) {
        float cyclePeriod = minCycle + (maxCycle - minCycle) * i / (count - 1);
        float startDelay = (float)i * 2.0 / count; // 2秒内交错启动
        startBreathing(pins[i], cyclePeriod, startDelay);
    }
}

void MillisPWM::startRangeBreathing(int startPin, int endPin, float minCycle, float maxCycle) {
    int count = endPin - startPin + 1;
    for (int i = 0; i < count; i++) {
        int pin = startPin + i;
        float cyclePeriod = minCycle + (maxCycle - minCycle) * i / (count - 1);
        float startDelay = (float)i * 2.0 / count;
        startBreathing(pin, cyclePeriod, startDelay);
    }
}

bool MillisPWM::isActive(int pin) {
    int channelIndex = findChannelByPin(pin);
    return (channelIndex >= 0) ? channels[channelIndex].getIsActive() : false;
}

uint8_t MillisPWM::getBrightness(int pin) {
    int channelIndex = findChannelByPin(pin);
    return (channelIndex >= 0) ? channels[channelIndex].getDutyCycle() : 0;
}

bool MillisPWM::isBreathing(int pin) {
    int channelIndex = findChannelByPin(pin);
    return (channelIndex >= 0) ? channels[channelIndex].isBreathing() : false;
}

bool MillisPWM::isUnstable(int pin) {
    int channelIndex = findChannelByPin(pin);
    return (channelIndex >= 0) ? channels[channelIndex].isUnstable() : false;
}

bool MillisPWM::isFading(int pin) {
    int channelIndex = findChannelByPin(pin);
    return (channelIndex >= 0) ? channels[channelIndex].isFading() : false;
}

int MillisPWM::getActiveCount() {
    int count = 0;
    for (int i = 0; i < channelCount; i++) {
        if (channels[i].getIsActive()) count++;
    }
    return count;
}

int MillisPWM::getBreathingCount() {
    int count = 0;
    for (int i = 0; i < channelCount; i++) {
        if (channels[i].isBreathing()) count++;
    }
    return count;
}

int MillisPWM::getUnstableCount() {
    int count = 0;
    for (int i = 0; i < channelCount; i++) {
        if (channels[i].isUnstable()) count++;
    }
    return count;
}

int MillisPWM::getFadingCount() {
    int count = 0;
    for (int i = 0; i < channelCount; i++) {
        if (channels[i].isFading()) count++;
    }
    return count;
}

void MillisPWM::update() {
    for (int i = 0; i < channelCount; i++) {
        if (channels[i].getIsActive()) {
            channels[i].update();
            updateCount++;
        }
    }
}

unsigned long MillisPWM::getUpdateCount() {
    return updateCount;
}

void MillisPWM::resetUpdateCount() {
    updateCount = 0;
}

uint8_t MillisPWM::getBreathingValue(uint8_t index) {
    if (index >= MPWM_BREATHING_TABLE_SIZE) index = MPWM_BREATHING_TABLE_SIZE - 1;
    return breathingTable[index];
}

// ================= 高级预设功能实现 =================

void MillisPWM::initializeMultiChannel(int startPin, int endPin, int basePeriodMs) {
    if (!initialized) begin();
    
    int channelCount = endPin - startPin + 1;
    for (int i = 0; i < channelCount; i++) {
        int pin = startPin + i;
        unsigned long pwmPeriod = basePeriodMs + (i % 11);
        start(pin, 128, pwmPeriod);
    }
}

void MillisPWM::startStaggeredBreathing(int startPin, int endPin, int minCycleMs, int maxCycleMs) {
    int channelCount = endPin - startPin + 1;
    for (int i = 0; i < channelCount; i++) {
        int pin = startPin + i;
        float breathingCycle = (minCycleMs + 
                              (i * (maxCycleMs - minCycleMs)) / (channelCount - 1)) / 1000.0;
        float startDelay = (float)(i * 2000) / (channelCount * 1000.0);
        startBreathing(pin, breathingCycle, startDelay);
    }
}

void MillisPWM::printDetailedStatus() {
    // 此函数已移除打印功能，如需状态信息请调用相应的getter方法
}

void MillisPWM::printSimpleStatus() {
    // 此函数已移除打印功能，如需状态信息请调用相应的getter方法
    resetUpdateCount();
}

bool MillisPWM::processCommand(String command, int startPin, int endPin) {
    command.trim();
    
    if (command == "start_all") {
        startStaggeredBreathing(startPin, endPin);
        return true;
    } else if (command == "stop_all") {
        stopAll();
        return true;
    } else if (command == "status") {
        printSimpleStatus();
        return true;
    } else if (command == "detail") {
        printDetailedStatus();
        return true;
    } else if (command.startsWith("pin ")) {
        int space1 = command.indexOf(' ');
        int space2 = command.indexOf(' ', space1 + 1);
        if (space1 > 0 && space2 > 0) {
            int pin = command.substring(space1 + 1, space2).toInt();
            int brightness = command.substring(space2 + 1).toInt();
            if (pin >= startPin && pin <= endPin && brightness >= 0 && brightness <= 255) {
                setBrightness(pin, (uint8_t)brightness);
                return true;
            }
        }
    } else if (command.startsWith("breathing ")) {
        int pin = command.substring(10).toInt();
        if (pin >= startPin && pin <= endPin) {
            startBreathing(pin, 2.0);
            return true;
        }
    } else if (command.startsWith("bright ")) {
        int space = command.indexOf(' ', 7);
        if (space > 0) {
            int pin = command.substring(7, space).toInt();
            int brightness = command.substring(space + 1).toInt();
            if (pin >= startPin && pin <= endPin && brightness >= 0 && brightness <= 255) {
                setBrightness(pin, (uint8_t)brightness);
                return true;
            }
        }
    }
    
    return false; // 未识别的命令
} 