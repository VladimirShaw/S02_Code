/**
 * =============================================================================
 * TimeManager - Arduino全局时间管理库 - 实现文件
 * 版本: 1.0
 * 创建日期: 2024-12-28
 * =============================================================================
 */

#include "TimeManager.h"

// ========================== 静态成员变量定义 ==========================
unsigned long TimeManager::currentMillis = 0;
bool TimeManager::initialized = false;
unsigned long TimeManager::updateCount = 0;
unsigned long TimeManager::millisCallCount = 0;

// ========================== 全局实例 ==========================
TimeManager timeManager;

// ========================== 初始化 ==========================
void TimeManager::begin() {
    if (!initialized) {
        currentMillis = millis();
        millisCallCount++;
        updateCount = 0;
        initialized = true;
        
        #ifdef DEBUG
        Serial.println(F("TimeManager初始化完成"));
        #endif
    }
}

// ========================== 时间更新 ==========================
void TimeManager::update() {
    currentMillis = millis();
    millisCallCount++;
    updateCount++;
}

// ========================== 时间获取 ==========================
unsigned long TimeManager::now() {
    return currentMillis;
}

unsigned long TimeManager::realtime() {
    millisCallCount++;
    return millis();
}

// ========================== 非阻塞延迟 ==========================
bool TimeManager::delay(unsigned long &lastTime, unsigned long interval) {
    if (currentMillis - lastTime >= interval) {
        lastTime = currentMillis;
        return true;
    }
    return false;
}

bool TimeManager::isDelayReady(unsigned long lastTime, unsigned long interval) {
    return (currentMillis - lastTime) >= interval;
}

// ========================== 超时检测 ==========================
bool TimeManager::isTimeout(unsigned long startTime, unsigned long timeout) {
    return (currentMillis - startTime) >= timeout;
}

bool TimeManager::isInWindow(unsigned long startTime, unsigned long duration) {
    unsigned long elapsed = currentMillis - startTime;
    return elapsed < duration;
}

// ========================== 时间计算 ==========================
unsigned long TimeManager::elapsed(unsigned long startTime) {
    return currentMillis - startTime;
}

unsigned long TimeManager::remaining(unsigned long startTime, unsigned long duration) {
    unsigned long elapsed = currentMillis - startTime;
    if (elapsed >= duration) {
        return 0;
    }
    return duration - elapsed;
}

float TimeManager::progress(unsigned long startTime, unsigned long duration) {
    if (duration == 0) return 1.0;
    
    unsigned long elapsed = currentMillis - startTime;
    if (elapsed >= duration) return 1.0;
    
    return (float)elapsed / (float)duration;
}

// ========================== 时间源接口 ==========================
unsigned long (*TimeManager::getTimeSource())() {
    return timeSourceFunction;
}

unsigned long TimeManager::timeSourceFunction() {
    return currentMillis;
}

// ========================== 调试功能 ==========================
void TimeManager::printStats() {
    #ifdef DEBUG
    Serial.println(F("=== TimeManager统计 ==="));
    Serial.print(F("当前时间: "));
    Serial.print(currentMillis);
    Serial.println(F("ms"));
    Serial.print(F("更新次数: "));
    Serial.println(updateCount);
    Serial.print(F("millis()调用: "));
    Serial.println(millisCallCount);
    if (updateCount > 0) {
        Serial.print(F("平均每次更新millis()调用: "));
        Serial.println((float)millisCallCount / updateCount, 2);
    }
    Serial.println(F("====================="));
    #endif
}

void TimeManager::resetStats() {
    updateCount = 0;
    millisCallCount = 0;
}

// ========================== 性能监控 ==========================
unsigned long TimeManager::getUpdateCount() {
    return updateCount;
}

unsigned long TimeManager::getMillisCallCount() {
    return millisCallCount;
} 