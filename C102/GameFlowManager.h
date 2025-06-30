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
 * =============================================================================
 */

#ifndef GAME_FLOW_MANAGER_H
#define GAME_FLOW_MANAGER_H

#include <Arduino.h>

// ========================== C102音频环节时间配置 ==========================
// 
// 所有音频控制的时间点都在这里定义，方便查看和修改
//

// 000_0 环节：第一路音频循环播放201号音频
#define STAGE_000_0_REPORT_DELAY    1000     // 1秒后报告完成

// 001_2 环节：第1路播放0001，第2路音量淡出
#define STAGE_001_2_DURATION        83347    // 83.347秒总时长
#define STAGE_001_2_FADE_DURATION   5000     // 5秒淡出时间

// 002_0 环节：第1路播放0002，第2路播放0201
#define STAGE_002_0_DURATION        60000    // 60秒默认时长（可根据实际音频长度调整）

class GameFlowManager {
private:
    String currentStageId;           // 当前环节ID
    unsigned long stageStartTime;    // 环节开始时间
    bool stageRunning;               // 环节是否运行中
    bool jumpRequested;              // 是否已请求跳转
    bool globalStopped;              // 全局停止标志
    
    // 环节完成通知
    void notifyStageComplete(const String& currentStep, const String& nextStep, unsigned long duration);
    void notifyStageComplete(const String& currentStep, unsigned long duration);  // 重载版本，不指定下一步
    
    // C102音频环节定义方法
    void updateStep000();            // 更新000_0环节：第一路音频循环播放201号音频
    void updateStep001_0();          // 更新001_0环节：第1路播放0001号音频
    void updateStep001_1();          // 更新001_1环节：第1路播放0001，第2路播放0002
    void updateStep001_2();          // 更新001_2环节：第1路播放0001，第2路音量淡出
    void updateStep002();            // 更新002_0环节：第1路播放0002，第2路播放0201
    
    // 工具方法
    String normalizeStageId(const String& stageId);  // 标准化环节ID格式
    
public:
    // ========================== 构造和初始化 ==========================
    GameFlowManager();
    void begin();
    
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
};

// 全局实例声明
extern GameFlowManager gameFlowManager;

#endif // GAME_FLOW_MANAGER_H 