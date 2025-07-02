#ifndef SIMPLE_GAME_STAGE_H
#define SIMPLE_GAME_STAGE_H

#include <Arduino.h>

// 最大时间段数量 - 优化内存占用
#define MAX_TIME_SEGMENTS 40  // 从100减少到40，节省1440字节

// 动作类型 - 更细化的分类
enum ActionType : uint8_t {  // 明确指定为uint8_t，节省3字节
    // === 瞬时动作 ===
    LED_ON,              // LED点亮
    LED_OFF,             // LED关闭
    DIGITAL_HIGH,        // 数字输出高
    DIGITAL_LOW,         // 数字输出低
    PWM_SET,             // 设置PWM值
    AUDIO_PLAY,          // 播放音频
    AUDIO_STOP,          // 停止音频
    
    // === 持续动作（有自动结束） ===
    LED_BREATHING,       // LED呼吸效果
    LED_FLASH,           // LED闪烁效果
    PWM_RAMP,           // PWM渐变效果
    SERVO_MOVE,         // 舵机移动
    
    // === 控制动作 ===
    STAGE_JUMP,         // 跳转环节
    DELAY_ACTION        // 延时动作
};

// 时间段动作结构 - 内存优化版本
struct TimeSegment {
    uint16_t startTime;             // 开始时间(毫秒) - 改为uint16_t，最大65秒
    uint16_t duration;              // 持续时间(毫秒) - 改为uint16_t
    int8_t pin;                     // 目标引脚/元器件
    ActionType action;              // 具体动作类型 (1字节)
    int16_t value1;                 // 第一个参数
    int16_t value2;                 // 第二个参数（可选）
    uint8_t flags;                  // 状态标志位：bit0=startExecuted, bit1=endExecuted, bit2=isActive
    // 移除了endTime字段，运行时计算
};

// 游戏环节类
class SimpleGameStage {
private:
    int currentStage;                           // 当前环节号
    unsigned long stageStartTime;              // 环节开始时间
    TimeSegment timeSegments[MAX_TIME_SEGMENTS]; // 时间段数组
    int segmentCount;                          // 时间段数量
    bool stageRunning;                         // 环节是否运行中
    String pendingJumpStageId;                 // 待跳转的环节ID（字符串版本）
    
    // 内部方法
    void executeStartAction(int segmentIndex);  // 执行开始动作
    void executeEndAction(int segmentIndex);    // 执行结束动作
    // calculateEndTime已移除，运行时直接计算
    
public:
    SimpleGameStage();
    
    // 环节控制
    void startStage(int stageNumber);          // 开始指定环节
    void stopStage();                          // 停止当前环节
    void update();                             // 更新(需要在loop中调用)
    void clearStage();                         // 清空当前环节
    
    // ==========================================
    // 核心方法：统一的时间段添加接口
    // ==========================================
    
    // 基础方法：完全自定义
    void addSegment(unsigned long startTime, unsigned long duration, int pin, 
                   ActionType action, int value1 = 0, int value2 = 0);
    
    // ==========================================
    // 便捷方法：常用动作的简化接口
    // ==========================================
    
    // === 瞬时动作 ===
    void instant(unsigned long startTime, int pin, ActionType action, int value = 0);
    
    // === 持续动作 ===
    void duration(unsigned long startTime, unsigned long duration, int pin, 
                 ActionType action, int value1 = 0, int value2 = 0);
    
    // === 专用快捷方法（最常用的几种） ===
    void ledBreathing(unsigned long startTime, unsigned long duration, int pin, float cycleSeconds = 2.0);
    void ledFlash(unsigned long startTime, unsigned long duration, int pin, int intervalMs = 100);
    void pwmRamp(unsigned long startTime, unsigned long duration, int pin, int fromValue, int toValue);
    void digitalPulse(unsigned long startTime, unsigned long duration, int pin);
    void jumpToStage(unsigned long startTime, int nextStage);                    // 数字版本（向后兼容）
    void jumpToStage(unsigned long startTime, const String& nextStageId);       // 字符串版本（推荐）
    
    // 状态查询
    int getCurrentStage();
    unsigned long getStageTime();
    bool isRunning();
    int getSegmentCount();
    int getActiveSegmentCount();
    
    // 调试
    void printStageInfo();
    void printActiveSegments();
    void printAllSegments();  // 显示所有时间段信息
    
    // 初始化方法
    void begin();
};

// 全局实例声明
extern SimpleGameStage gameStage;

#endif // SIMPLE_GAME_STAGE_H 