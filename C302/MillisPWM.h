/**
 * =============================================================================
 * MillisPWM - 基于millis()的超低CPU占用PWM库
 * 版本: 1.0
 * 创建日期: 2024-12-28
 * 
 * 特点:
 * - 超低CPU占用 (31路PWM仅0.1%)
 * - 完全非阻塞
 * - 支持呼吸灯效果
 * - 简单易用的API
 * - 基于millis()，无需micros()
 * =============================================================================
 */

#ifndef MILLIS_PWM_H
#define MILLIS_PWM_H

#include <Arduino.h>

// 时间管理器接口 - 允许外部提供时间源
class MillisTimeSource {
public:
    static unsigned long (*getTime)();  // 函数指针，默认使用millis()
    static unsigned long getCurrentTime() {
        return getTime ? getTime() : millis();
    }
    static void setTimeSource(unsigned long (*timeFunc)()) {
        getTime = timeFunc;
    }
};

// 默认配置
#define MPWM_MAX_CHANNELS 30        // 最大PWM通道数 (从50减少到30)
#define MPWM_DEFAULT_PERIOD 10      // 默认PWM周期(ms) - 50Hz
#define MPWM_BREATHING_TABLE_SIZE 100 // 呼吸灯查找表大小 (从200减少到100)

/**
 * @brief PWM通道类 - 单个PWM通道的完整功能
 */
class PWMChannel {
private:
    uint8_t dutyCycle;
    uint16_t pwmPeriod;              // 改为uint16_t，最大65535ms足够
    unsigned long lastToggle;
    bool isActive;
    bool currentState;
    uint16_t onTime;                 // 改为uint16_t
    
    // 呼吸灯相关
    bool breathingEnabled;
    uint16_t breathingCyclePeriod;   // 改为uint16_t，最大65秒
    unsigned long breathingStartTime;
    unsigned long breathingLastUpdate;
    uint16_t breathingUpdateInterval; // 改为uint16_t
    
    // Fade渐变相关
    bool fadeEnabled;
    uint8_t fadeStartValue;
    uint8_t fadeTargetValue;
    uint16_t fadeDuration;           // fade持续时间(ms)
    unsigned long fadeStartTime;
    unsigned long fadeLastUpdate;
    
    // 电压不稳定相关
    bool unstableEnabled;
    uint8_t baseVoltage;
    uint8_t currentVoltage;
    uint8_t targetVoltage;
    int8_t flickerIntensity;         // 改为int8_t，-128到127足够
    bool inDropout;
    unsigned long lastVoltageChange;
    unsigned long lastFlicker;
    unsigned long lastRandomShift;
    unsigned long dropoutStart;
    uint8_t instabilityLevel;        // 不稳定程度 1-5
    
    void updateTiming();
    void updateBreathing();
    void updateUnstable();
    void updateFade();
    
public:
    int8_t pin;  // 改为int8_t，Arduino引脚号不会超过127
    PWMChannel();
    
    // 基本PWM控制
    bool start(int pin, uint8_t dutyCycle = 128, unsigned long periodMs = MPWM_DEFAULT_PERIOD);
    void stop();
    void setDutyCycle(uint8_t dutyCycle);
    void setPeriod(unsigned long periodMs);
    
    // 呼吸灯控制
    void startBreathing(unsigned long cyclePeriodMs = 2000, unsigned long startDelayMs = 0);
    void stopBreathing();
    
    // Fade渐变控制
    void fadeIn(uint8_t targetValue, unsigned long durationMs);
    void fadeOut(unsigned long durationMs);
    void fadeTo(uint8_t targetValue, unsigned long durationMs);
    void stopFade();
    
    // 电压不稳定控制
    void startUnstable(uint8_t baseVoltage = 180, uint8_t instabilityLevel = 3);
    void stopUnstable();
    void setInstabilityLevel(uint8_t level);
    
    // 状态查询
    bool getIsActive() const;
    uint8_t getDutyCycle() const;
    unsigned long getPeriod() const;
    bool isBreathing() const;
    bool isUnstable() const;
    bool isFading() const;
    
    // 更新函数 - 必须在loop()中调用
    void update();
};

/**
 * @brief MillisPWM主类 - 管理多个PWM通道
 */
class MillisPWM {
private:
    static PWMChannel channels[MPWM_MAX_CHANNELS];
    static uint8_t breathingTable[MPWM_BREATHING_TABLE_SIZE];
    static bool initialized;
    static int channelCount;
    
    static void generateBreathingTable();
    static int findChannelByPin(int pin);
    
public:
    // 初始化
    static void begin();
    
    // 简单PWM控制
    static bool start(int pin, uint8_t dutyCycle = 128);
    static bool start(int pin, uint8_t dutyCycle, unsigned long periodMs);
    static void stop(int pin);
    static void stopAll();
    
    // 亮度控制
    static void setBrightness(int pin, uint8_t brightness);
    static void setBrightnessPercent(int pin, float percentage); // 0.0-100.0%
    
    // 呼吸灯控制
    static bool startBreathing(int pin, float cyclePeriodSeconds = 2.0);
    static bool startBreathing(int pin, float cyclePeriodSeconds, float startDelaySeconds);
    static void stopBreathing(int pin);
    
    // Fade渐变控制
    static bool fadeIn(int pin, uint8_t targetValue = 255, unsigned long durationMs = 1000);
    static bool fadeOut(int pin, unsigned long durationMs = 1000);
    static bool fadeTo(int pin, uint8_t targetValue, unsigned long durationMs = 1000);
    static void stopFade(int pin);
    
    // 电压不稳定控制
    static bool startUnstable(int pin, uint8_t baseVoltage = 180, uint8_t instabilityLevel = 3);
    static void stopUnstable(int pin);
    static void setInstabilityLevel(int pin, uint8_t level);
    
    // 批量操作
    static void startAllBreathing(int* pins, int count, float minCycle = 0.75, float maxCycle = 3.0);
    static void startRangeBreathing(int startPin, int endPin, float minCycle = 0.75, float maxCycle = 3.0);
    
    // 高级预设功能
    static void initializeMultiChannel(int startPin, int endPin, int basePeriodMs = 15);
    static void startStaggeredBreathing(int startPin, int endPin, int minCycleMs = 750, int maxCycleMs = 3000);
    static void printDetailedStatus();  // 已移除打印功能，使用getter方法获取状态
    static void printSimpleStatus();    // 已移除打印功能，使用getter方法获取状态
    
    // 智能命令处理器
    static bool processCommand(String command, int startPin = 22, int endPin = 52);
    
    // 状态查询
    static bool isActive(int pin);
    static uint8_t getBrightness(int pin);
    static bool isBreathing(int pin);
    static bool isUnstable(int pin);
    static bool isFading(int pin);
    static int getActiveCount();
    static int getBreathingCount();
    static int getUnstableCount();
    static int getFadingCount();
    
    // 更新函数 - 必须在loop()中调用
    static void update();
    
    // 性能监控
    static unsigned long getUpdateCount();
    static void resetUpdateCount();
    
    // 获取呼吸表值 (内部使用)
    static uint8_t getBreathingValue(uint8_t index);
};

// 便捷宏定义
#define MPWM_BEGIN()                MillisPWM::begin()
#define MPWM_UPDATE()               MillisPWM::update()
#define MPWM_START(pin, brightness) MillisPWM::start(pin, brightness)
#define MPWM_STOP(pin)              MillisPWM::stop(pin)
#define MPWM_BREATHING(pin, cycle)  MillisPWM::startBreathing(pin, cycle)
#define MPWM_BRIGHT(pin, val)       MillisPWM::setBrightness(pin, val)
#define MPWM_FADEIN(pin, target, duration)  MillisPWM::fadeIn(pin, target, duration)
#define MPWM_FADEOUT(pin, duration) MillisPWM::fadeOut(pin, duration)
#define MPWM_FADETO(pin, target, duration)  MillisPWM::fadeTo(pin, target, duration)

#endif // MILLIS_PWM_H 