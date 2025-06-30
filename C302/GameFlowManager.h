/**
 * =============================================================================
 * GameFlowManager - 游戏流程管理器
 * 版本: 1.0
 * 创建日期: 2025-01-03
 * 
 * 功能:
 * - 专门管理游戏环节的定义和执行
 * - 统一的环节启动接口
 * - 支持多种环节ID格式
 * - 与协议处理器解耦
 * =============================================================================
 */

#ifndef GAME_FLOW_MANAGER_H
#define GAME_FLOW_MANAGER_H

#include <Arduino.h>

// ========================== 时间点宏定义 ==========================
// 
// 所有游戏特效的时间点都在这里定义，方便查看和修改
// 修改这些宏定义可以直接调整特效时长和节奏
//

// 072-x 胜利庆祝持续时间（结果：全部熄灭）
#define STAGE_072_1_DURATION        12000    // 第一次胜利庆祝总时长12秒
#define STAGE_072_2_DURATION        10000    // 第二次胜利庆祝总时长10秒  
#define STAGE_072_3_DURATION        10000    // 第三次胜利庆祝总时长10秒

// 080-0 最终胜利效果时刻表（按照用户提供的精确时间点）
#define STAGE_080_0_FLASH_START     0        // 全场闪烁开始时间
#define STAGE_080_0_FLASH_END       4800     // 全场闪烁结束时间
#define STAGE_080_0_FLASH_ON_TIME   800      // 每次闪烁点亮时长
#define STAGE_080_0_FLASH_OFF_TIME  800      // 每次闪烁熄灭时长
#define STAGE_080_0_FLASH_CYCLES    3        // 闪烁次数

// 蜡烛灯控制时刻表（按照用户提供的精确时间）
#define CANDLE_LEFT_OFF_TIME        10766    // 左侧蜡烛关闭时间
#define CANDLE_RIGHT_OFF_TIME       10766    // 右侧蜡烛关闭时间
#define CANDLE_LEFT_ON_TIME         13320    // 左侧蜡烛点亮时间
#define CANDLE_RIGHT_ON_TIME        13320    // 右侧蜡烛点亮时间

// 蜡烛高频闪烁参数
#define CANDLE_STROBE_START         15164    // 高频闪烁开始时间
#define CANDLE_STROBE_END           19566    // 高频闪烁结束时间
#define CANDLE_STROBE_ON_TIME       30       // 每次点亮时长30ms
#define CANDLE_STROBE_OFF_TIME      30       // 每次熄灭时长30ms
#define CANDLE_STROBE_CYCLE_TIME    60       // 完整周期时长(30+30)ms

// ========================== 错误光效跳转时间配置 ==========================
// 
// 072-7/8/9 错误光效的总时长和跳转时间点
// 修改这些宏定义可以调整错误反馈的时长
//

// 错误光效总时长
#define STAGE_072_7_DURATION        3500    // 072-7 错误效果1总时长 (ms)
#define STAGE_072_8_DURATION        3500    // 072-8 错误效果2总时长 (ms)
#define STAGE_072_9_DURATION        3500     // 072-9 错误效果3总时长 (ms)

// 错误光效时间点配置
#define ERROR_SLOW_FLASH_END        2400     // 慢闪结束时间 (ms)
#define ERROR_FAST_FLASH_END        3000     // 快闪结束时间 (ms)
#define ERROR_SLOW_FLASH_ON_TIME    400      // 慢闪亮起时间 (ms)
#define ERROR_SLOW_FLASH_OFF_TIME   400      // 慢闪熄灭时间 (ms)
#define ERROR_FAST_FLASH_ON_TIME    50       // 快闪亮起时间 (ms)
#define ERROR_FAST_FLASH_OFF_TIME   50       // 快闪熄灭时间 (ms)
#define ERROR_SLOW_FLASH_CYCLES     3        // 慢闪循环次数
#define ERROR_FAST_FLASH_CYCLES     6        // 快闪循环次数

class GameFlowManager {
private:
    String currentStageId;           // 当前环节ID
    unsigned long stageStartTime;    // 环节开始时间
    bool stageRunning;               // 环节是否运行中
    
    // 可配置的环节ID前缀
    String stagePrefix;              // 环节ID前缀（默认"072-"）
    
    // 全局输入事件标记
    static volatile bool pin25Triggered;  // 引脚25按键触发标记
    static bool lastPin25State;          // 引脚25上次状态
    
    // 遗迹地图游戏按键监控
    static volatile bool buttonPressed[25];  // 25个按键的按下标记
    static bool lastButtonState[25];         // 25个按键的上次状态
    
    // 遗迹地图游戏状态
    static int lastPressedButton;            // 上一个按下的按键编号
    static int errorCount;                   // 错误次数计数器
    static int successCount;                 // 成功次数计数器 (新增)
    static bool gameActive;                  // 游戏是否激活状态
    static int currentLevel;                 // 当前游戏关卡(1-4)
    static String lastCompletionSource;      // 上次完成来源("error"/"success")
    
    // 矩阵旋转系统
    static int currentRotation;              // 当前旋转方向 (0=原始, 1=90°, 2=180°, 3=270°)
    static int lastRotation;                 // 上次使用的旋转方向
    
    // 刷新步骤循环追踪
    static bool lastRefreshWas5;             // 记录上次刷新是否为-5（true:-5, false:-6）
    
    // 定时跳转状态
    static bool autoJumpScheduled;           // 是否已安排自动跳转
    static unsigned long autoJumpTime;       // 自动跳转时间
    static String autoJumpFromStage;         // 跳转源环节
    static String autoJumpToStage;           // 跳转目标环节
    
    // 频闪状态机（用于080-0蜡烛高频闪烁）
    static bool strobeActive;                // 频闪是否激活
    static bool strobeState;                 // 当前频闪状态（true=亮，false=灭）
    static unsigned long strobeNextTime;     // 下次状态切换时间
    static unsigned long strobeEndTime;      // 频闪结束时间
    
    // 环节完成通知
    void notifyStageComplete(const String& currentStep, const String& nextStep, unsigned long duration);
    
    // 具体环节定义方法
    void defineStage072_0();         // 环节072-0：游戏初始化
    void defineStage072_0_5();       // 环节072-0.5：准备阶段
    void defineStage072_1();         // 环节072-1：第一次正确庆祝
    void defineStage072_2();         // 环节072-2：第二次正确庆祝
    void defineStage072_3();         // 环节072-3：第三次正确庆祝
    void defineStage072_4();         // 环节072-4：第3关
    void defineStage072_5();         // 环节072-5：刷新光效1
    void defineStage072_6();         // 环节072-6：刷新光效2
    void defineStage072_7();         // 环节072-7：错误效果1
    void defineStage072_8();         // 环节072-8：错误效果2
    void defineStage072_9();         // 环节072-9：错误效果3
    void defineStage080_0();         // 环节080-0：最终胜利
    
    // 输入处理方法
    void checkInputs();              // 检查所有输入状态
    void processInputEvents();       // 处理输入事件
    
    // 动态效果管理
    void stopDynamicEffects();       // 停止动态效果，保持静态状态
    
    // 工具方法
    String normalizeStageId(const String& stageId);  // 标准化环节ID格式
    void resetGameState();                           // 重置游戏状态变量
    
    // 遗迹地图游戏辅助方法（私有）
    int getButtonInputPin(int buttonNumber);         // 获取按键对应的输入引脚号
    void setupLevel1();                              // 设置Level 1初始状态
    void setupLevel2();                              // 设置Level 2初始状态
    void setupLevel3();                              // 设置Level 3初始状态
    void setupLevel4();                              // 设置Level 4初始状态
    bool areButtonsAdjacent(int button1, int button2); // 检查按键是否相邻
    void handleMapButtonPress(int buttonNumber);     // 处理遗迹地图按键按下事件
    bool isButtonLit(int buttonNumber);              // 检查按键是否已经亮着
    void handleGameError(int failedButton);          // 处理游戏错误
    bool checkGameComplete();                        // 检查游戏是否完成
    void handleGameComplete();                       // 处理游戏完成
    void executeLastButtonEffect();                  // 执行最后按下按键的闪烁效果
    void scheduleAutoJump(const String& fromStage, const String& toStage, unsigned long delayMs); // 安排自动跳转
    
    // 矩阵旋转系统（私有）
    int generateRandomRotation();                    // 生成不重复的随机旋转方向
    int rotateButtonNumber(int originalButton, int rotation); // 根据旋转方向转换按键编号
    int reverseRotateButtonNumber(int rotatedButton, int rotation); // 反向旋转：从旋转后坐标获取原始坐标
    void applyRotationToLevel(int level, int rotation);      // 对指定Level应用旋转

public:
    // ========================== 构造和初始化 ==========================
    GameFlowManager();
    void begin();
    
    // ========================== 环节前缀配置 ==========================
    void setStagePrefix(const String& prefix);      // 设置环节ID前缀
    const String& getStagePrefix() const;           // 获取环节ID前缀
    String buildStageId(const String& suffix);      // 构建完整环节ID
    
    // ========================== 环节控制 ==========================
    bool startStage(const String& stageId);         // 启动指定环节
    void stopCurrentStage();                        // 停止当前环节
    void stopAllStages();                           // 停止所有环节
    
    // ========================== 状态查询 ==========================
    const String& getCurrentStageId() const;        // 获取当前环节ID
    bool isStageRunning() const;                     // 是否有环节在运行
    unsigned long getStageElapsedTime() const;      // 获取环节运行时间
    
    // ========================== 环节列表 ==========================
    bool isValidStageId(const String& stageId);     // 检查环节ID是否有效
    void printAvailableStages();                    // 打印所有可用环节
    
    // ========================== 更新和调试功能 ==========================
    void update();                                  // 游戏流程更新（必须调用）
    void printStatus();                             // 打印当前状态
    
    // ========================== 环节跳转请求 ==========================
    void requestStageJump(const String& nextStage); // 请求跳转到指定环节（通过服务器）
    
    // ========================== 刷新步骤循环管理 ==========================
    String getNextRefreshStage();                    // 获取下一个刷新步骤(-5或-6)
    void recordRefreshStage(const String& stageId);  // 记录执行的刷新步骤
    void resetRefreshCycle();                        // 重置刷新循环（从-5开始）
    
    // ========================== Level管理系统 ==========================
    int getCurrentLevel() const;                     // 获取当前Level
    void setCurrentLevel(int level);                 // 设置当前Level
    void advanceToNextLevel();                       // 进入下一个Level (1→2→4→3→4→3...)
    void keepCurrentLevel();                         // 保持当前Level (错误时使用)
    String getNextSuccessStage();                    // 获取成功后的下一个步骤(072-1/2/3)
    String getRefreshTargetStage();                  // 获取刷新后的目标步骤
    void setCompletionSource(const String& source);  // 设置完成来源("error"/"success")
    
    // ========================== 工具方法（公有） ==========================
    int getButtonPin(int buttonNumber);              // 获取按键对应的输出引脚号
};

// 全局实例
extern GameFlowManager gameFlowManager;

#endif // GAME_FLOW_MANAGER_H 