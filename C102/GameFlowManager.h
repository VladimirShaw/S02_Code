/**
 * =============================================================================
 * GameFlowManager - C102音频控制器游戏流程管理器
 * 版本: 2.0 - C102专用版本
 * 创建日期: 2025-01-03
 * 
 * 功能:
 * - 专门管理C102音频控制器的游戏环节
 * - 支持BY语音模块4路音频控制
 * - 简洁的环节定义和执行
 * - 与协议处理器解耦
 * - 支持多个环节并行执行
 * =============================================================================
 */

#ifndef GAME_FLOW_MANAGER_H
#define GAME_FLOW_MANAGER_H

#include <Arduino.h>

// ========================== 并行环节配置 ==========================
#define MAX_PARALLEL_STAGES 4  // 最大并行环节数（根据需要调整）

// ========================== 音量管理配置 ==========================
#define DEFAULT_VOLUME 30       // 默认音量值
#define TOTAL_CHANNELS 4        // 总通道数

// ========================== C102音频环节时间配置 ==========================
// 
// 所有音频控制的时间点都在这里定义，方便查看和修改
//

// 000_0 环节：第一路音频循环播放201号音频（初始化环节，不自动跳转）
#define STAGE_000_0_CHANNEL         2        // 播放通道
#define STAGE_000_0_SONG_ID         201      // 播放歌曲ID
#define STAGE_000_0_START           0        // 启动时间(ms)
#define STAGE_000_0_STABLE_TIME     1000     // 播放稳定期(ms) - 开始播放后等待时间
#define STAGE_000_0_CHECK_INTERVAL  500      // 音频检查间隔(ms)

// ========================== 000_0环节引脚状态配置 ==========================
// C102主要负责音频控制，数字IO引脚较少，主要用于状态指示
// 根据实际硬件连接情况配置以下引脚状态

// 状态指示LED (如果有的话)
#define STAGE_000_0_STATUS_LED1_STATE       LOW     // 状态LED1 (根据实际引脚配置)
#define STAGE_000_0_STATUS_LED2_STATE       LOW     // 状态LED2 (根据实际引脚配置)
#define STAGE_000_0_STATUS_LED3_STATE       LOW     // 状态LED3 (根据实际引脚配置)

// 继电器控制 (如果有的话)
#define STAGE_000_0_RELAY1_STATE            LOW     // 继电器1状态 (根据实际引脚配置)
#define STAGE_000_0_RELAY2_STATE            LOW     // 继电器2状态 (根据实际引脚配置)

// 辅助输出 (如果有的话)
#define STAGE_000_0_AUX_OUTPUT1_STATE       LOW     // 辅助输出1状态 (根据实际引脚配置)
#define STAGE_000_0_AUX_OUTPUT2_STATE       LOW     // 辅助输出2状态 (根据实际引脚配置)

// 注意：具体引脚号需要根据C102的实际硬件配置文件来定义
// 注意：000_0环节作为初始化环节，不设置自动完成时间，等待服务器指令

// 001_2 环节：第1路播放0001，第2路3秒内音量从30减到0
#define STAGE_001_2_CHANNEL         1        // 播放通道
#define STAGE_001_2_SONG_ID         1        // 播放歌曲ID
#define STAGE_001_2_START           0        // 启动时间(ms)
#define STAGE_001_2_FADE_CHANNEL    2        // 淡出通道
#define STAGE_001_2_FADE_START_VOL  30       // 起始音量
#define STAGE_001_2_FADE_END_VOL    0        // 结束音量
#define STAGE_001_2_FADE_DURATION   3000     // 3秒淡出时间
#define STAGE_001_2_FADE_INTERVAL   100      // 100ms调整一次音量(30步*100ms=3000ms)
#define STAGE_001_2_DURATION        90347    // 90.347秒总时长
#define STAGE_001_2_NEXT_STAGE      "002_0"  // 跳转目标环节

// 002_0 环节：第1路播放0002，第2路播放0203，30秒时触发多环节跳转
#define STAGE_002_0_CHANNEL1        1        // 第1播放通道
#define STAGE_002_0_SONG_ID1        2        // 第1播放歌曲ID
#define STAGE_002_0_CHANNEL1_START  0        // 第1通道启动时间(ms)
#define STAGE_002_0_CHANNEL2        2        // 第2播放通道
#define STAGE_002_0_SONG_ID2        203      // 第2播放歌曲ID
#define STAGE_002_0_CHANNEL2_START  0        // 第2通道启动时间(ms)
#define STAGE_002_0_MULTI_JUMP_TIME 30000    // 30秒时触发多环节跳转
#define STAGE_002_0_MULTI_JUMP_STAGES "005_0,006_0"  // 多环节跳转目标
#define STAGE_002_0_DURATION        60000    // 60秒默认时长（可根据实际音频长度调整）
#define STAGE_002_0_NEXT_STAGE      ""       // 跳转目标环节（空字符串表示只报告完成，不跳转）

class GameFlowManager {
private:
    // 环节状态结构体
    struct StageState {
        String stageId;              // 环节ID
        unsigned long startTime;     // 开始时间
        bool running;                // 是否运行中
        bool jumpRequested;          // 是否已请求跳转
        
        // 环节特定状态（使用union节省内存）
        union {
            struct {  // 000_0环节状态
                bool channelStarted;
                unsigned long lastCheckTime;
            } stage000;
            
            struct {  // 001_2环节状态
                bool channelStarted;
                unsigned long lastVolumeUpdate;
                int currentVolume;
                bool volumeUpdateComplete;
            } stage001_2;
            
            struct {  // 002_0环节状态
                bool channel1Started;
                bool channel2Started;
                bool multiJumpTriggered;  // 多环节跳转是否已触发
            } stage002;
        } state;
        
        // 构造函数
        StageState() : stageId(""), startTime(0), running(false), jumpRequested(false) {
            memset(&state, 0, sizeof(state));
        }
    };
    
    // 并行环节数组
    StageState stages[MAX_PARALLEL_STAGES];
    int activeStageCount;            // 当前活跃环节数
    bool globalStopped;              // 全局停止标志
    
    // 兼容旧接口的当前环节引用
    String currentStageId;           // 当前环节ID（指向第一个活跃环节）
    unsigned long stageStartTime;    // 环节开始时间（指向第一个活跃环节）
    bool stageRunning;               // 环节是否运行中（任意环节运行即为true）
    bool jumpRequested;              // 是否已请求跳转（任意环节请求即为true）
    
    // 查找环节索引
    int findStageIndex(const String& stageId);
    int findEmptySlot();
    
    // 环节完成通知
    void notifyStageComplete(const String& currentStep, const String& nextStep, unsigned long duration);
    void notifyStageComplete(const String& currentStep, unsigned long duration);  // 重载版本，不指定下一步
    
    // C102音频环节定义方法（带索引参数）
    void updateStep000(int index);            // 更新000_0环节
    void updateStep001_2(int index);          // 更新001_2环节
    void updateStep002(int index);            // 更新002_0环节
    
    // 工具方法
    String normalizeStageId(const String& stageId);  // 标准化环节ID格式
    void updateCompatibilityVars();                  // 更新兼容性变量
    
    // 音量管理方法
    void initializeAllVolumes();                     // 初始化所有通道音量为默认值
    void resetChannelVolume(int channel);            // 重置指定通道音量为默认值
    void resetAllVolumes();                          // 重置所有通道音量为默认值

public:
    // ========================== 构造和初始化 ==========================
    GameFlowManager();
    void begin();                                   // 初始化（包含音量设置）
    
    // ========================== 环节控制 ==========================
    bool startStage(const String& stageId);         // 启动指定环节
    bool startMultipleStages(const String& stageIds); // 启动多个环节（逗号分隔）
    void stopStage(const String& stageId);          // 停止指定环节
    void stopCurrentStage();                        // 停止当前环节（兼容旧接口）
    void stopAllStages();                           // 停止所有环节
    
    // ========================== 状态查询 ==========================
    const String& getCurrentStageId() const;        // 获取当前环节ID（兼容旧接口）
    bool isStageRunning() const;                    // 是否有环节在运行
    bool isStageRunning(const String& stageId);     // 指定环节是否在运行
    unsigned long getStageElapsedTime() const;      // 获取环节运行时间（兼容旧接口）
    unsigned long getStageElapsedTime(const String& stageId); // 获取指定环节运行时间
    int getActiveStageCount() const;                // 获取活跃环节数
    void getActiveStages(String stages[], int maxCount); // 获取所有活跃环节ID
    
    // ========================== 环节列表 ==========================
    bool isValidStageId(const String& stageId);     // 检查环节ID是否有效
    void printAvailableStages();                    // 打印所有可用环节
    
    // ========================== 更新和调试功能 ==========================
    void update();                                  // 游戏流程更新（必须调用）
    void printStatus();                             // 打印当前状态
    
    // ========================== 环节跳转请求 ==========================
    void requestStageJump(const String& nextStage); // 请求跳转到指定环节（通过服务器）
    void requestMultiStageJump(const String& currentStep, const String& nextSteps); // 请求跳转到多个环节
};

// 全局实例声明
extern GameFlowManager gameFlowManager;

#endif // GAME_FLOW_MANAGER_H 