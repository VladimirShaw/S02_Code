/**
 * =============================================================================
 * TimeManager - Arduino全局时间管理库
 * 版本: 1.0
 * 创建日期: 2024-12-28
 * 
 * 特点:
 * - 统一时间管理，每个loop只调用一次millis()
 * - 非阻塞延迟功能
 * - 超时检测
 * - 时间差计算
 * - 为MillisPWM等库提供时间源
 * =============================================================================
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>

/**
 * @brief 全局时间管理器类
 * 统一管理所有时间相关操作，优化性能
 */
class TimeManager {
private:
    static unsigned long currentMillis;  // 当前时间缓存
    static bool initialized;             // 是否已初始化
    
public:
    // ========================== 初始化 ==========================
    static void begin();
    
    // ========================== 时间更新 ==========================
    /**
     * @brief 更新全局时间 - 必须在loop()开始时调用
     */
    static void update();
    
    // ========================== 时间获取 ==========================
    /**
     * @brief 获取当前时间（缓存值）
     * @return 当前时间（毫秒）
     */
    static unsigned long now();
    
    /**
     * @brief 获取实时时间（直接调用millis()）
     * @return 实时时间（毫秒）
     */
    static unsigned long realtime();
    
    // ========================== 非阻塞延迟 ==========================
    /**
     * @brief 非阻塞延迟检查
     * @param lastTime 上次时间的引用
     * @param interval 间隔时间（毫秒）
     * @return true=时间到了，false=还没到
     */
    static bool delay(unsigned long &lastTime, unsigned long interval);
    
    /**
     * @brief 非阻塞延迟检查（不修改lastTime）
     * @param lastTime 上次时间
     * @param interval 间隔时间（毫秒）
     * @return true=时间到了，false=还没到
     */
    static bool isDelayReady(unsigned long lastTime, unsigned long interval);
    
    // ========================== 超时检测 ==========================
    /**
     * @brief 检查是否超时
     * @param startTime 开始时间
     * @param timeout 超时时间（毫秒）
     * @return true=已超时，false=未超时
     */
    static bool isTimeout(unsigned long startTime, unsigned long timeout);
    
    /**
     * @brief 检查是否在时间窗口内
     * @param startTime 开始时间
     * @param duration 持续时间（毫秒）
     * @return true=在窗口内，false=不在窗口内
     */
    static bool isInWindow(unsigned long startTime, unsigned long duration);
    
    // ========================== 时间计算 ==========================
    /**
     * @brief 计算经过的时间
     * @param startTime 开始时间
     * @return 经过的时间（毫秒）
     */
    static unsigned long elapsed(unsigned long startTime);
    
    /**
     * @brief 计算剩余时间
     * @param startTime 开始时间
     * @param duration 总持续时间
     * @return 剩余时间（毫秒），0表示已结束
     */
    static unsigned long remaining(unsigned long startTime, unsigned long duration);
    
    /**
     * @brief 计算进度百分比
     * @param startTime 开始时间
     * @param duration 总持续时间
     * @return 进度百分比（0.0-1.0）
     */
    static float progress(unsigned long startTime, unsigned long duration);
    
    // ========================== 时间源接口 ==========================
    /**
     * @brief 获取时间源函数指针（供其他库使用）
     * @return 时间函数指针
     */
    static unsigned long (*getTimeSource())();
    
    // ========================== 调试功能 ==========================
    /**
     * @brief 打印时间统计信息
     */
    static void printStats();
    
    /**
     * @brief 重置统计信息
     */
    static void resetStats();
    
    // ========================== 性能监控 ==========================
    static unsigned long getUpdateCount();
    static unsigned long getMillisCallCount();
    
private:
    // 统计信息
    static unsigned long updateCount;
    static unsigned long millisCallCount;
    
    // 时间源函数（供其他库使用）
    static unsigned long timeSourceFunction();
};

// ========================== 便捷宏定义 ==========================
#define TIME_BEGIN()                TimeManager::begin()
#define TIME_UPDATE()               TimeManager::update()
#define TIME_NOW()                  TimeManager::now()
#define TIME_DELAY(last, interval)  TimeManager::delay(last, interval)
#define TIME_TIMEOUT(start, timeout) TimeManager::isTimeout(start, timeout)
#define TIME_ELAPSED(start)         TimeManager::elapsed(start)

// ========================== 全局实例 ==========================
extern TimeManager timeManager;

#endif // TIME_MANAGER_H 