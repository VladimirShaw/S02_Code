#include "DigitalIOController.h"
#include "MillisPWM.h"  // 包含现有的MillisTimeSource定义

// 静态成员变量定义
DigitalOutputChannel DigitalIOController::outputChannels[DigitalIOController::MAX_OUTPUT_CHANNELS];
DigitalInputChannel DigitalIOController::inputChannels[DigitalIOController::MAX_INPUT_CHANNELS];
uint8_t DigitalIOController::outputChannelCount = 0;
uint8_t DigitalIOController::inputChannelCount = 0;
bool DigitalIOController::initialized = false;

// ========================== DigitalOutputChannel 实现 ==========================

DigitalOutputChannel::DigitalOutputChannel() : pin(-1), state(OUTPUT_IDLE), currentLevel(LOW),
                                                startTime(0), duration(0), delayTime(0), isActive(false), toggleElapsed(0) {}

bool DigitalOutputChannel::start(int p, bool level) {
    pin = (int8_t)p;
    currentLevel = level;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, level);
    isActive = true;
    state = OUTPUT_ACTIVE;
    startTime = MillisTimeSource::getCurrentTime();
    duration = 0; // 持续输出
    return true;
}

void DigitalOutputChannel::stop() {
    if (isActive && pin >= 0) {
        digitalWrite(pin, LOW);
        isActive = false;
        state = OUTPUT_IDLE;
    }
}

bool DigitalOutputChannel::scheduleOutput(int p, bool level, unsigned long delayMs, unsigned long durationMs) {
    pin = (int8_t)p;
    currentLevel = level;
    delayTime = delayMs;
    duration = durationMs;
    pinMode(pin, OUTPUT);
    
    if (delayMs == 0) {
        // 立即执行
        digitalWrite(pin, level);
        state = OUTPUT_ACTIVE;
        startTime = MillisTimeSource::getCurrentTime();
    } else {
        // 等待执行
        state = OUTPUT_WAITING;
        startTime = MillisTimeSource::getCurrentTime();
    }
    
    isActive = true;
    return true;
}

bool DigitalOutputChannel::pulseOutput(int p, unsigned long pulseWidthMs) {
    return scheduleOutput(p, HIGH, 0, pulseWidthMs);
}

bool DigitalOutputChannel::toggleOutput(int p, unsigned long intervalMs, int pulseCount) {
    pin = (int8_t)p;
    pinMode(pin, OUTPUT);
    
    // 开始闪烁模式
    currentLevel = LOW;  // 从LOW开始
    digitalWrite(pin, currentLevel);
    isActive = true;
    state = OUTPUT_ACTIVE;  // 直接进入活跃状态
    startTime = MillisTimeSource::getCurrentTime();
    delayTime = intervalMs;  // 使用delayTime作为切换间隔
    duration = (pulseCount > 0) ? (pulseCount * intervalMs * 2) : 0;  // 0表示无限闪烁
    toggleElapsed = 0;  // 重置闪烁计数器
    
    return true;
}

void DigitalOutputChannel::update() {
    if (!isActive || pin < 0) return;
    
    unsigned long now = MillisTimeSource::getCurrentTime();
    
    switch (state) {
        case OUTPUT_WAITING:
            if (now - startTime >= delayTime) {
                digitalWrite(pin, currentLevel);
                state = OUTPUT_ACTIVE;
                startTime = now; // 重置开始时间
            }
            break;
            
        case OUTPUT_ACTIVE:
            // 检查是否需要切换状态（闪烁模式）
            if (delayTime > 0 && (now - startTime >= delayTime)) {
                // 切换状态
                currentLevel = !currentLevel;
                digitalWrite(pin, currentLevel);
                startTime = now; // 重置时间基准
                
                // 检查是否达到总持续时间（有限次闪烁）
                if (duration > 0) {
                    toggleElapsed += delayTime;
                    if (toggleElapsed >= duration) {
                        digitalWrite(pin, LOW);
                        isActive = false;
                        state = OUTPUT_IDLE;
                        toggleElapsed = 0;  // 重置计数器
                    }
                }
            }
            // 非闪烁模式的原有逻辑
            else if (delayTime == 0 && duration > 0 && (now - startTime >= duration)) {
                digitalWrite(pin, LOW);
                isActive = false;
                state = OUTPUT_IDLE;
            }
            break;
            
        case OUTPUT_IDLE:
        default:
            break;
    }
}

bool DigitalOutputChannel::getIsActive() const { return isActive; }
bool DigitalOutputChannel::getCurrentLevel() const { return currentLevel; }
OutputState DigitalOutputChannel::getState() const { return state; }

unsigned long DigitalOutputChannel::getRemainingTime() const {
    if (!isActive || duration == 0) return 0;
    
    unsigned long now = MillisTimeSource::getCurrentTime();
    unsigned long elapsed = now - startTime;
    
    if (state == OUTPUT_WAITING) {
        return delayTime + duration - elapsed;
    } else if (state == OUTPUT_ACTIVE) {
        return (elapsed < duration) ? (duration - elapsed) : 0;
    }
    
    return 0;
}

// ========================== DigitalInputChannel 实现 ==========================

DigitalInputChannel::DigitalInputChannel() : pin(-1), mode(INPUT_SINGLE), lastValue(LOW), currentValue(LOW),
                                              lastSampleTime(0), sampleInterval(100), hasChanged(false),
                                              isActive(false), bufferIndex(0), sampleCount(0) {
    memset(sampleBuffer, 0, sizeof(sampleBuffer));
}

bool DigitalInputChannel::start(int p, InputMode m, unsigned long intervalMs) {
    pin = (int8_t)p;
    mode = m;
    sampleInterval = intervalMs;
    pinMode(pin, INPUT);
    
    // 读取初始值
    currentValue = digitalRead(pin);
    lastValue = currentValue;
    hasChanged = false;
    isActive = true;
    lastSampleTime = MillisTimeSource::getCurrentTime();
    
    return true;
}

void DigitalInputChannel::stop() {
    isActive = false;
}

bool DigitalInputChannel::readValue() {
    if (!isActive || pin < 0) return false;
    
    lastValue = currentValue;
    currentValue = digitalRead(pin);
    
    if (currentValue != lastValue) {
        hasChanged = true;
        
        // 更新采样缓冲区
        if (mode == INPUT_CONTINUOUS) {
            sampleBuffer[bufferIndex] = currentValue ? 1 : 0;
            bufferIndex = (bufferIndex + 1) % 8;
            if (sampleCount < 8) sampleCount++;
        }
    }
    
    return currentValue;
}

bool DigitalInputChannel::hasValueChanged() {
    return hasChanged;
}

void DigitalInputChannel::resetChangeFlag() {
    hasChanged = false;
}

void DigitalInputChannel::setSampleInterval(unsigned long intervalMs) {
    sampleInterval = intervalMs;
}

uint8_t DigitalInputChannel::getSampleHistory(bool* buffer, uint8_t maxCount) {
    uint8_t count = min(sampleCount, maxCount);
    
    for (uint8_t i = 0; i < count; i++) {
        uint8_t index = (bufferIndex - count + i + 8) % 8;
        buffer[i] = sampleBuffer[index] ? true : false;
    }
    
    return count;
}

void DigitalInputChannel::update() {
    if (!isActive || pin < 0) return;
    
    unsigned long now = MillisTimeSource::getCurrentTime();
    
    if (mode == INPUT_CONTINUOUS || mode == INPUT_CHANGE) {
        if (now - lastSampleTime >= sampleInterval) {
            lastSampleTime = now;
            readValue();
        }
    }
}

bool DigitalInputChannel::getIsActive() const { return isActive; }
bool DigitalInputChannel::getCurrentValue() const { return currentValue; }
bool DigitalInputChannel::getLastValue() const { return lastValue; }
InputMode DigitalInputChannel::getMode() const { return mode; }

// ========================== DigitalIOController 实现 ==========================

void DigitalIOController::begin() {
    if (!initialized) {
        outputChannelCount = 0;
        inputChannelCount = 0;
        initialized = true;
    }
}

int DigitalIOController::findOutputChannelByPin(int pin) {
    // 首先查找活跃的同引脚通道
    for (int i = 0; i < outputChannelCount; i++) {
        if (outputChannels[i].pin == pin && outputChannels[i].getIsActive()) {
            return i;
        }
    }
    return -1;
}

int DigitalIOController::findAvailableOutputChannel() {
    // 查找第一个不活跃的通道用于复用
    for (int i = 0; i < outputChannelCount; i++) {
        if (!outputChannels[i].getIsActive()) {
            return i;
        }
    }
    return -1;
}

int DigitalIOController::findInputChannelByPin(int pin) {
    for (int i = 0; i < inputChannelCount; i++) {
        if (inputChannels[i].pin == pin && inputChannels[i].getIsActive()) {
            return i;
        }
    }
    return -1;
}

bool DigitalIOController::setOutput(int pin, bool level) {
    if (!initialized) begin();
    
    // 查找现有活跃通道
    int channelIndex = findOutputChannelByPin(pin);
    
    if (channelIndex >= 0) {
        outputChannels[channelIndex].stop();
        return outputChannels[channelIndex].start(pin, level);
    }
    
    // 查找可复用的停止通道
    channelIndex = findAvailableOutputChannel();
    if (channelIndex >= 0) {
        return outputChannels[channelIndex].start(pin, level);
    }
    
    // 创建新通道
    if (outputChannelCount < MAX_OUTPUT_CHANNELS) {
        outputChannels[outputChannelCount].start(pin, level);
        outputChannelCount++;
        return true;
    }
    
    return false;
}

bool DigitalIOController::scheduleOutput(int pin, bool level, unsigned long delayMs, unsigned long durationMs) {
    if (!initialized) begin();
    
    // 查找现有活跃通道
    int channelIndex = findOutputChannelByPin(pin);
    
    if (channelIndex >= 0) {
        return outputChannels[channelIndex].scheduleOutput(pin, level, delayMs, durationMs);
    }
    
    // 查找可复用的停止通道
    channelIndex = findAvailableOutputChannel();
    if (channelIndex >= 0) {
        outputChannels[channelIndex].scheduleOutput(pin, level, delayMs, durationMs);
        return true;
    }
    
    // 创建新通道
    if (outputChannelCount < MAX_OUTPUT_CHANNELS) {
        outputChannels[outputChannelCount].scheduleOutput(pin, level, delayMs, durationMs);
        outputChannelCount++;
        return true;
    }
    
    return false;
}

bool DigitalIOController::pulseOutput(int pin, unsigned long pulseWidthMs) {
    return scheduleOutput(pin, HIGH, 0, pulseWidthMs);
}

bool DigitalIOController::toggleOutput(int pin, unsigned long intervalMs, int pulseCount) {
    if (!initialized) begin();
    
    // 查找现有活跃通道
    int channelIndex = findOutputChannelByPin(pin);
    
    if (channelIndex >= 0) {
        return outputChannels[channelIndex].toggleOutput(pin, intervalMs, pulseCount);
    }
    
    // 查找可复用的停止通道
    channelIndex = findAvailableOutputChannel();
    if (channelIndex >= 0) {
        outputChannels[channelIndex].toggleOutput(pin, intervalMs, pulseCount);
        return true;
    }
    
    // 创建新通道
    if (outputChannelCount < MAX_OUTPUT_CHANNELS) {
        outputChannels[outputChannelCount].toggleOutput(pin, intervalMs, pulseCount);
        outputChannelCount++;
        return true;
    }
    
    return false;
}

void DigitalIOController::stopOutput(int pin) {
    int channelIndex = findOutputChannelByPin(pin);
    if (channelIndex >= 0) {
        outputChannels[channelIndex].stop();
    }
}

void DigitalIOController::stopAllOutputs() {
    for (int i = 0; i < outputChannelCount; i++) {
        outputChannels[i].stop();
    }
    // 重置通道计数器，允许重新使用通道数组
    outputChannelCount = 0;
}

bool DigitalIOController::startInput(int pin, InputMode mode, unsigned long intervalMs) {
    if (!initialized) begin();
    
    // 查找现有通道
    int channelIndex = findInputChannelByPin(pin);
    
    if (channelIndex >= 0) {
        return inputChannels[channelIndex].start(pin, mode, intervalMs);
    }
    
    // 创建新通道
    if (inputChannelCount < MAX_INPUT_CHANNELS) {
        inputChannels[inputChannelCount].start(pin, mode, intervalMs);
        inputChannelCount++;
        return true;
    }
    
    return false;
}

bool DigitalIOController::readInput(int pin) {
    int channelIndex = findInputChannelByPin(pin);
    if (channelIndex >= 0) {
        return inputChannels[channelIndex].readValue();
    }
    
    // 如果没有通道，直接读取
    return digitalRead(pin);
}

bool DigitalIOController::hasInputChanged(int pin) {
    int channelIndex = findInputChannelByPin(pin);
    return (channelIndex >= 0) ? inputChannels[channelIndex].hasValueChanged() : false;
}

void DigitalIOController::resetInputChange(int pin) {
    int channelIndex = findInputChannelByPin(pin);
    if (channelIndex >= 0) {
        inputChannels[channelIndex].resetChangeFlag();
    }
}

void DigitalIOController::stopInput(int pin) {
    int channelIndex = findInputChannelByPin(pin);
    if (channelIndex >= 0) {
        inputChannels[channelIndex].stop();
    }
}

void DigitalIOController::stopAllInputs() {
    for (int i = 0; i < inputChannelCount; i++) {
        inputChannels[i].stop();
    }
    // 重置通道计数器，允许重新使用通道数组
    inputChannelCount = 0;
}

void DigitalIOController::setOutputPattern(int* pins, bool* levels, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        setOutput(pins[i], levels[i]);
    }
}

void DigitalIOController::scheduleOutputSequence(int* pins, bool* levels, unsigned long* delays, unsigned long* durations, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        scheduleOutput(pins[i], levels[i], delays[i], durations[i]);
    }
}

bool DigitalIOController::isOutputActive(int pin) {
    int channelIndex = findOutputChannelByPin(pin);
    return (channelIndex >= 0) ? outputChannels[channelIndex].getIsActive() : false;
}

bool DigitalIOController::isInputActive(int pin) {
    int channelIndex = findInputChannelByPin(pin);
    return (channelIndex >= 0) ? inputChannels[channelIndex].getIsActive() : false;
}

uint8_t DigitalIOController::getActiveOutputCount() {
    uint8_t count = 0;
    for (int i = 0; i < outputChannelCount; i++) {
        if (outputChannels[i].getIsActive()) count++;
    }
    return count;
}

uint8_t DigitalIOController::getActiveInputCount() {
    uint8_t count = 0;
    for (int i = 0; i < inputChannelCount; i++) {
        if (inputChannels[i].getIsActive()) count++;
    }
    return count;
}

unsigned long DigitalIOController::getSystemUptime() {
    return MillisTimeSource::getCurrentTime();
}

unsigned long DigitalIOController::getOutputStartTime(int pin) {
    int channelIndex = findOutputChannelByPin(pin);
    return (channelIndex >= 0) ? outputChannels[channelIndex].startTime : 0;
}

unsigned long DigitalIOController::getOutputDuration(int pin) {
    int channelIndex = findOutputChannelByPin(pin);
    return (channelIndex >= 0) ? outputChannels[channelIndex].duration : 0;
}

unsigned long DigitalIOController::getOutputRemainingTime(int pin) {
    int channelIndex = findOutputChannelByPin(pin);
    return (channelIndex >= 0) ? outputChannels[channelIndex].getRemainingTime() : 0;
}

void DigitalIOController::update() {
    // 更新所有输出通道
    for (int i = 0; i < outputChannelCount; i++) {
        if (outputChannels[i].getIsActive()) {
            outputChannels[i].update();
        }
    }
    
    // 更新所有输入通道
    for (int i = 0; i < inputChannelCount; i++) {
        if (inputChannels[i].getIsActive()) {
            inputChannels[i].update();
        }
    }
}

bool DigitalIOController::processCommand(String command) {
    command.trim();
    
    // 输出命令: o24h (引脚24输出高电平)
    if (command.startsWith("o") && command.length() >= 3) {
        int pin = command.substring(1, command.length()-1).toInt();
        bool level = command.endsWith("h") || command.endsWith("H");
        return setOutput(pin, level);
    }
    
    // 脉冲命令: pulse24:1000 (引脚24脉冲1秒)
    if (command.startsWith("pulse") && command.indexOf(':') > 0) {
        int colonPos = command.indexOf(':');
        int pin = command.substring(5, colonPos).toInt();  // 从"pulse"后开始提取
        unsigned long width = command.substring(colonPos + 1).toInt();
        return pulseOutput(pin, width);
    }
    
    // 定时命令: t24h:500:2000 (引脚24，高电平，延迟500ms，持续2000ms)
    if (command.startsWith("t") && command.indexOf(':') > 0) {
        int colon1 = command.indexOf(':');
        int colon2 = command.indexOf(':', colon1 + 1);
        if (colon2 > 0) {
            String pinLevel = command.substring(1, colon1);
            int pin = pinLevel.substring(0, pinLevel.length()-1).toInt();
            bool level = pinLevel.endsWith("h") || pinLevel.endsWith("H");
            unsigned long delay = command.substring(colon1 + 1, colon2).toInt();
            unsigned long duration = command.substring(colon2 + 1).toInt();
            return scheduleOutput(pin, level, delay, duration);
        }
    }
    
    // 输入命令: i24 (读取引脚24)
    if (command.startsWith("i") && command.length() >= 3) {
        int pin = command.substring(1).toInt();
        return startInput(pin, INPUT_CHANGE, 100);
    }
    
    return false;
}

// ========================== 统一输出管理器实现 ==========================

bool UnifiedOutputManager::setOutput(int pin, bool level) {
    // 检查并处理冲突
    handleConflict(pin);
    
    // 执行数字输出
    return DigitalIOController::setOutput(pin, level);
}

bool UnifiedOutputManager::setPWM(int pin, uint8_t value) {
    // 检查并处理冲突
    handleConflict(pin);
    
    // 执行PWM输出 - 先启动通道，再设置亮度
    if (!MillisPWM::isActive(pin)) {
        MillisPWM::start(pin, value);
    } else {
        MillisPWM::setBrightness(pin, value);
    }
    return true;
}

bool UnifiedOutputManager::pulse(int pin, unsigned long widthMs) {
    // 检查并处理冲突
    handleConflict(pin);
    
    // 执行脉冲输出
    return DigitalIOController::pulseOutput(pin, widthMs);
}

bool UnifiedOutputManager::scheduleOutput(int pin, bool level, unsigned long delayMs, unsigned long durationMs) {
    // 检查并处理冲突
    handleConflict(pin);
    
    // 执行定时输出
    return DigitalIOController::scheduleOutput(pin, level, delayMs, durationMs);
}

void UnifiedOutputManager::stopOutput(int pin) {
    // 同时停止PWM和DIO（轻量级，只调用必要的停止函数）
    MillisPWM::stop(pin);
    DigitalIOController::stopOutput(pin);
}

bool UnifiedOutputManager::isOutputActive(int pin) {
    // 检查PWM或DIO是否活跃
    return MillisPWM::isActive(pin) || DigitalIOController::isOutputActive(pin);
}

String UnifiedOutputManager::getOutputType(int pin) {
    if (MillisPWM::isActive(pin)) {
        if (MillisPWM::isFading(pin)) return "PWM_FADE";
        if (MillisPWM::isBreathing(pin)) return "PWM_BREATH";
        return "PWM";
    }
    if (DigitalIOController::isOutputActive(pin)) {
        return "DIO";
    }
    return "NONE";
}

void UnifiedOutputManager::handleConflict(int pin) {
    // 轻量级冲突处理：如果有PWM活跃，停止PWM；如果有DIO活跃，停止DIO
    // 这样新的操作总是能正确执行，遵循"后来者优先"原则
    
    if (MillisPWM::isActive(pin)) {
        MillisPWM::stop(pin);
    }
    
    if (DigitalIOController::isOutputActive(pin)) {
        DigitalIOController::stopOutput(pin);
    }
} 