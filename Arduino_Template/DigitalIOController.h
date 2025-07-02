#ifndef DIGITAL_IO_CONTROLLER_H
#define DIGITAL_IO_CONTROLLER_H

#include <Arduino.h>

// 前向声明，使用现有的MillisTimeSource
class MillisTimeSource;

// 输出通道状态
enum OutputState {
    OUTPUT_IDLE,
    OUTPUT_ACTIVE,
    OUTPUT_WAITING
};

// 输入采样模式
enum InputMode {
    INPUT_SINGLE,      // 单次采样
    INPUT_CONTINUOUS,  // 连续采样
    INPUT_CHANGE       // 变化检测
};

// ========================== 统一输出管理器 ==========================
/**
 * @brief 统一输出管理器 - 自动协调PWM和DIO
 * 轻量级设计，只在需要时检查冲突
 */
class UnifiedOutputManager {
public:
    // 统一输出接口 - 用户只需调用这些
    static bool setOutput(int pin, bool level);                    // 数字输出
    static bool setPWM(int pin, uint8_t value);                    // PWM输出
    static bool pulse(int pin, unsigned long widthMs);             // 脉冲输出
    static bool scheduleOutput(int pin, bool level, unsigned long delayMs, unsigned long durationMs); // 定时输出
    
    // 停止输出（自动处理PWM和DIO）
    static void stopOutput(int pin);
    
    // 状态查询
    static bool isOutputActive(int pin);
    static String getOutputType(int pin);
    
private:
    // 内部冲突处理（轻量级，无额外内存占用）
    static void handleConflict(int pin);
};

/**
 * @brief 数字输出通道 - 定时控制输出
 */
class DigitalOutputChannel {
private:
    OutputState state;
    bool currentLevel;
    unsigned long delayTime;
    bool isActive;
    unsigned long toggleElapsed; // 闪烁模式累计时间
    
public:
    int8_t pin;              // 改为public，方便DigitalIOController访问
    unsigned long startTime; // 改为public
    unsigned long duration;  // 改为public
    
public:
    DigitalOutputChannel();
    
    // 基本控制
    bool start(int pin, bool level = HIGH);
    void stop();
    
    // 定时控制
    bool scheduleOutput(int pin, bool level, unsigned long delayMs, unsigned long durationMs);
    bool pulseOutput(int pin, unsigned long pulseWidthMs);
    bool toggleOutput(int pin, unsigned long intervalMs, int pulseCount = -1);
    
    // 状态查询
    bool getIsActive() const;
    bool getCurrentLevel() const;
    OutputState getState() const;
    unsigned long getRemainingTime() const;
    
    // 更新函数
    void update();
};

/**
 * @brief 数字输入通道 - 采样和监控
 */
class DigitalInputChannel {
private:
    InputMode mode;
    bool lastValue;
    bool currentValue;
    unsigned long lastSampleTime;
    unsigned long sampleInterval;
    bool hasChanged;
    bool isActive;
    
public:
    int8_t pin;  // 改为public，方便DigitalIOController访问
    
    // 连续采样缓冲区
    uint8_t sampleBuffer[8];
    uint8_t bufferIndex;
    uint8_t sampleCount;
    
public:
    DigitalInputChannel();
    
    // 基本控制
    bool start(int pin, InputMode mode = INPUT_SINGLE, unsigned long intervalMs = 100);
    void stop();
    
    // 采样控制
    bool readValue();
    bool hasValueChanged();
    void resetChangeFlag();
    
    // 连续采样
    void setSampleInterval(unsigned long intervalMs);
    uint8_t getSampleHistory(bool* buffer, uint8_t maxCount);
    
    // 状态查询
    bool getIsActive() const;
    bool getCurrentValue() const;
    bool getLastValue() const;
    InputMode getMode() const;
    
    // 更新函数
    void update();
};

/**
 * @brief 数字IO控制器主类
 */
class DigitalIOController {
private:
    static const uint8_t MAX_OUTPUT_CHANNELS = 16;
    static const uint8_t MAX_INPUT_CHANNELS = 16;
    
    static DigitalOutputChannel outputChannels[MAX_OUTPUT_CHANNELS];
    static DigitalInputChannel inputChannels[MAX_INPUT_CHANNELS];
    static uint8_t outputChannelCount;
    static uint8_t inputChannelCount;
    static bool initialized;
    
    static int findOutputChannelByPin(int pin);
    static int findAvailableOutputChannel();
    static int findInputChannelByPin(int pin);
    
public:
    // 初始化
    static void begin();
    
    // 输出控制
    static bool setOutput(int pin, bool level);
    static bool scheduleOutput(int pin, bool level, unsigned long delayMs, unsigned long durationMs);
    static bool pulseOutput(int pin, unsigned long pulseWidthMs);
    static bool toggleOutput(int pin, unsigned long intervalMs, int pulseCount = -1);
    static void stopOutput(int pin);
    static void stopAllOutputs();
    
    // 输入控制
    static bool startInput(int pin, InputMode mode = INPUT_SINGLE, unsigned long intervalMs = 100);
    static bool readInput(int pin);
    static bool hasInputChanged(int pin);
    static void resetInputChange(int pin);
    static void stopInput(int pin);
    static void stopAllInputs();
    
    // 批量操作
    static void setOutputPattern(int* pins, bool* levels, uint8_t count);
    static void scheduleOutputSequence(int* pins, bool* levels, unsigned long* delays, unsigned long* durations, uint8_t count);
    
    // 状态查询
    static bool isOutputActive(int pin);
    static bool isInputActive(int pin);
    static uint8_t getActiveOutputCount();
    static uint8_t getActiveInputCount();
    
    // 时间相关
    static unsigned long getSystemUptime();
    static unsigned long getOutputStartTime(int pin);
    static unsigned long getOutputDuration(int pin);
    static unsigned long getOutputRemainingTime(int pin);
    
    // 更新函数 - 必须在loop()中调用
    static void update();
    
    // 命令处理
    static bool processCommand(String command);
};

// 便捷宏定义 - 统一输出管理
#define DIO_BEGIN()                    DigitalIOController::begin()
#define DIO_UPDATE()                   DigitalIOController::update()

// 统一输出接口宏 - 用户主要使用这些
#define OUTPUT_SET(pin, level)         UnifiedOutputManager::setOutput(pin, level)
#define OUTPUT_PWM(pin, value)         UnifiedOutputManager::setPWM(pin, value)
#define OUTPUT_PULSE(pin, width)       UnifiedOutputManager::pulse(pin, width)
#define OUTPUT_SCHEDULE(pin, level, delay, duration) UnifiedOutputManager::scheduleOutput(pin, level, delay, duration)
#define OUTPUT_STOP(pin)               UnifiedOutputManager::stopOutput(pin)
#define OUTPUT_ACTIVE(pin)             UnifiedOutputManager::isOutputActive(pin)

// 输入接口宏
#define INPUT_READ(pin)                DigitalIOController::readInput(pin)
#define INPUT_START(pin, mode, interval) DigitalIOController::startInput(pin, mode, interval)
#define INPUT_CHANGED(pin)             DigitalIOController::hasInputChanged(pin)

// 传统兼容宏（内部使用）
#define DIO_SET(pin, level)            DigitalIOController::setOutput(pin, level)
#define DIO_PULSE(pin, width)          DigitalIOController::pulseOutput(pin, width)
#define DIO_SCHEDULE(pin, level, delay, duration) DigitalIOController::scheduleOutput(pin, level, delay, duration)
#define DIO_READ(pin)                  DigitalIOController::readInput(pin)
#define DIO_START_INPUT(pin, mode, interval) DigitalIOController::startInput(pin, mode, interval)

#endif // DIGITAL_IO_CONTROLLER_H 