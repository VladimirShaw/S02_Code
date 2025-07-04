#include "SimpleGameStage.h"
#include "MillisPWM.h"
#include "GameFlowManager.h"

// 前向声明，避免循环依赖
class GameFlowManager;
extern GameFlowManager gameFlowManager;

// 全局实例定义
SimpleGameStage gameStage;

// 构造函数
SimpleGameStage::SimpleGameStage() {
    currentStage = -1;
    stageStartTime = 0;
    segmentCount = 0;
    stageRunning = false;
    pendingJumpStageId = "";
    
    // 初始化时间段数组
    for (int i = 0; i < MAX_TIME_SEGMENTS; i++) {
        timeSegments[i].startTime = 0;
        timeSegments[i].duration = 0;
        timeSegments[i].pin = -1;
        timeSegments[i].action = LED_OFF;
        timeSegments[i].value1 = 0;
        timeSegments[i].value2 = 0;
        timeSegments[i].flags = 0;  // 所有标志位清零
    }
}

// 开始指定环节
void SimpleGameStage::startStage(int stageNumber) {
    currentStage = stageNumber;
    stageStartTime = millis();
    stageRunning = true;
    
    // 重置所有时间段的执行状态
    for (int i = 0; i < segmentCount; i++) {
        timeSegments[i].flags = 0;  // 清除所有标志位
    }
    
    Serial.print("🎮 开始环节 ");
    Serial.print(stageNumber);
    Serial.print(" (共");
    Serial.print(segmentCount);
    Serial.println("个时间段)");
}

// 停止当前环节
void SimpleGameStage::stopStage() {
    // 停止所有活跃的动作
    for (int i = 0; i < segmentCount; i++) {
        if (timeSegments[i].flags & 0x04) {  // 检查isActive标志位(bit2)
            executeEndAction(i);
        }
    }
    
    stageRunning = false;
    Serial.print("⏹️ 停止环节 ");
    Serial.println(currentStage);
}

// 更新函数(在loop中调用)
void SimpleGameStage::update() {
    if (!stageRunning) return;
    
    unsigned long currentTime = millis() - stageStartTime;
    
    // 检查所有时间段
    for (int i = 0; i < segmentCount; i++) {
        TimeSegment& segment = timeSegments[i];
        
        // 位操作访问标志位
        bool startExecuted = segment.flags & 0x01;
        bool endExecuted = segment.flags & 0x02;
        bool isActive = segment.flags & 0x04;
        
        // 🚨 优先处理STAGE_JUMP：即时动作，在startTime时立即执行
        if (segment.action == STAGE_JUMP && !startExecuted && currentTime >= segment.startTime) {
            Serial.print(F("⏰ 定时跳转触发! 当前时间: "));
            Serial.print(currentTime);
            Serial.print(F("ms, 目标时间: "));
            Serial.print(segment.startTime);
            Serial.println(F("ms"));
            executeEndAction(i);  // 直接执行跳转
            segment.flags |= 0x03;  // 设置startExecuted和endExecuted
            continue;  // 跳过后续处理
        }
        
        // 检查是否需要执行开始动作
        if (!startExecuted && currentTime >= segment.startTime) {
            executeStartAction(i);
            segment.flags |= 0x01;  // 设置startExecuted
            
            // 如果是持续动作，标记为活跃
            if (segment.duration > 0) {
                segment.flags |= 0x04;  // 设置isActive
            }
        }
        
        // 检查是否需要执行结束动作（计算endTime）
        if ((segment.flags & 0x01) && !(segment.flags & 0x02) && 
            segment.duration > 0 && currentTime >= (segment.startTime + segment.duration)) {
            executeEndAction(i);
            segment.flags |= 0x02;   // 设置endExecuted
            segment.flags &= ~0x04;  // 清除isActive
        }
    }
}

// calculateEndTime方法已移除，现在运行时直接计算endTime

// 执行开始动作
void SimpleGameStage::executeStartAction(int segmentIndex) {
    TimeSegment& segment = timeSegments[segmentIndex];
    
    Serial.print("▶️ [");
    Serial.print(segment.startTime);
    Serial.print("ms] ");
    
    switch (segment.action) {
        case LED_ON:
            if (segment.pin == -2) {
                // 特殊功能：点亮所有按键 - 简化版本
                Serial.println("点亮所有按键");
                // 使用通用引脚范围，不再依赖特定的按键映射
                for (int pin = 2; pin <= 53; pin++) {
                    MillisPWM::setBrightness(pin, 128); // 使用中等亮度
                }
            } else {
                pinMode(segment.pin, OUTPUT);
                digitalWrite(segment.pin, HIGH);
                Serial.print("LED");
                Serial.print(segment.pin);
                Serial.println(" ON");
            }
            break;
            
        case LED_OFF:
            if (segment.pin == -1) {
                // 特殊功能：关闭所有按键 - 简化版本
                Serial.println("关闭所有按键");
                // 使用通用引脚范围，不再依赖特定的按键映射
                for (int pin = 2; pin <= 53; pin++) {
                    MillisPWM::setBrightness(pin, 0);
                }
            } else {
                pinMode(segment.pin, OUTPUT);
                digitalWrite(segment.pin, LOW);
                Serial.print("LED");
                Serial.print(segment.pin);
                Serial.println(" OFF");
            }
            break;
            
        case DIGITAL_HIGH:
            pinMode(segment.pin, OUTPUT);
            digitalWrite(segment.pin, HIGH);
            Serial.print("PIN");
            Serial.print(segment.pin);
            Serial.print(" HIGH");
            if (segment.duration > 0) {
                Serial.print(" (持续");
                Serial.print(segment.duration);
                Serial.print("ms)");
            }
            Serial.println();
            break;
            
        case DIGITAL_LOW:
            pinMode(segment.pin, OUTPUT);
            digitalWrite(segment.pin, LOW);
            Serial.print("PIN");
            Serial.print(segment.pin);
            Serial.print(" LOW");
            if (segment.duration > 0) {
                Serial.print(" (持续");
                Serial.print(segment.duration);
                Serial.print("ms)");
            }
            Serial.println();
            break;
            
        case PWM_SET:
            pinMode(segment.pin, OUTPUT);
            analogWrite(segment.pin, segment.value1);
            Serial.print("PWM");
            Serial.print(segment.pin);
            Serial.print(" = ");
            Serial.print(segment.value1);
            if (segment.duration > 0) {
                Serial.print(" (持续");
                Serial.print(segment.duration);
                Serial.print("ms)");
            }
            Serial.println();
            break;
            
        case LED_BREATHING:
            pinMode(segment.pin, OUTPUT);
            // value1是周期毫秒
            MillisPWM::startBreathing(segment.pin, segment.value1 / 1000.0);
            Serial.print("LED");
            Serial.print(segment.pin);
            Serial.print(" BREATHING (");
            Serial.print(segment.value1);
            Serial.print("ms周期, 持续");
            Serial.print(segment.duration);
            Serial.println("ms)");
            break;
            
        case LED_FLASH:
            pinMode(segment.pin, OUTPUT);
            // value1是间隔时间，这里开始第一次点亮
            digitalWrite(segment.pin, HIGH);
            Serial.print("LED");
            Serial.print(segment.pin);
            Serial.print(" FLASH开始 (间隔");
            Serial.print(segment.value1);
            Serial.print("ms, 持续");
            Serial.print(segment.duration);
            Serial.println("ms)");
            break;
            
        case PWM_RAMP:
            pinMode(segment.pin, OUTPUT);
            // 开始渐变，从value1到value2
            analogWrite(segment.pin, segment.value1);
            Serial.print("PWM");
            Serial.print(segment.pin);
            Serial.print(" RAMP ");
            Serial.print(segment.value1);
            Serial.print("→");
            Serial.print(segment.value2);
            Serial.print(" (");
            Serial.print(segment.duration);
            Serial.println("ms)");
            break;
            
        case AUDIO_PLAY:
            Serial.print("AUDIO PLAY ");
            Serial.print(segment.value1);
            if (segment.duration > 0) {
                Serial.print(" (持续");
                Serial.print(segment.duration);
                Serial.print("ms)");
            }
            Serial.println();
            // 这里可以集成音频模块
            break;
            
        case AUDIO_STOP:
            Serial.println("AUDIO STOP");
            break;
            
        case STAGE_JUMP:
            Serial.print("JUMP TO STAGE ");
            Serial.println(segment.value1);
            // 延迟跳转，避免在update循环中修改数据
            break;
            
        case SERVO_MOVE:
            Serial.print("SERVO");
            Serial.print(segment.pin);
            Serial.print(" MOVE TO ");
            Serial.print(segment.value1);
            Serial.println("°");
            // 这里可以集成舵机控制
            break;
            
        default:
            Serial.print("未知动作: ");
            Serial.println(segment.action);
            break;
    }
}

// 执行结束动作
void SimpleGameStage::executeEndAction(int segmentIndex) {
    TimeSegment& segment = timeSegments[segmentIndex];
    
    Serial.print("⏹️ [");
    Serial.print(segment.startTime + segment.duration);  // 运行时计算endTime
    Serial.print("ms] 结束: ");
    
    switch (segment.action) {
        case DIGITAL_HIGH:
            digitalWrite(segment.pin, LOW);
            Serial.print("PIN");
            Serial.print(segment.pin);
            Serial.println(" → LOW");
            break;
            
        case PWM_SET:
            analogWrite(segment.pin, 0);
            Serial.print("PWM");
            Serial.print(segment.pin);
            Serial.println(" → 0");
            break;
            
        case LED_BREATHING:
            MillisPWM::stopBreathing(segment.pin);
            digitalWrite(segment.pin, LOW);
            Serial.print("LED");
            Serial.print(segment.pin);
            Serial.println(" BREATHING STOP");
            break;
            
        case LED_FLASH:
            digitalWrite(segment.pin, LOW);
            Serial.print("LED");
            Serial.print(segment.pin);
            Serial.println(" FLASH STOP");
            break;
            
        case PWM_RAMP:
            // 渐变结束，设置为最终值
            analogWrite(segment.pin, segment.value2);
            Serial.print("PWM");
            Serial.print(segment.pin);
            Serial.print(" RAMP完成 → ");
            Serial.println(segment.value2);
            break;
            
        case AUDIO_PLAY:
            Serial.print("AUDIO ");
            Serial.print(segment.value1);
            Serial.println(" STOP");
            break;
            
        case STAGE_JUMP:
            // 发送环节完成通知给服务器，请求跳转
            Serial.print("📤 请求跳转到环节: ");
            
            String nextStage;
            if (segment.value1 == -1) {
                // 使用字符串版本
                nextStage = pendingJumpStageId;
                Serial.print("(字符串版本) ");
                Serial.println(nextStage);
                
                // 调试：检查pendingJumpStageId是否为空
                if (nextStage.length() == 0) {
                    Serial.println("❌ 错误：pendingJumpStageId为空！");
                    return;
                }
                
                pendingJumpStageId = "";  // 清空待跳转ID
            } else {
                // 使用数字版本（向后兼容）
                nextStage = String(segment.value1);
                Serial.print("(数字版本) ");
                Serial.println(nextStage);
            }
            
            Serial.print("🔄 调用 gameFlowManager.requestStageJump(");
            Serial.print(nextStage);
            Serial.println(")");
            
            // 通过GameFlowManager请求跳转
            gameFlowManager.requestStageJump(nextStage);
            
            Serial.println("✅ requestStageJump 调用完成");
            return;
            
        default:
            // 其他动作不需要结束处理
            break;
    }
}

// ==========================================
// 核心方法：统一的时间段添加接口
// ==========================================

void SimpleGameStage::addSegment(unsigned long startTime, unsigned long duration, int pin, 
                                 ActionType action, int value1, int value2) {
    if (segmentCount >= MAX_TIME_SEGMENTS) {
        Serial.println("❌ 时间段数量已达上限！");
        return;
    }
    
    // 检查时间范围（uint16_t最大65535ms ≈ 65秒）
    if (startTime > 65535 || duration > 65535) {
        Serial.println("❌ 时间超出范围（最大65秒）！");
        return;
    }
    
    timeSegments[segmentCount].startTime = (uint16_t)startTime;
    timeSegments[segmentCount].duration = (uint16_t)duration;
    timeSegments[segmentCount].pin = pin;
    timeSegments[segmentCount].action = action;
    timeSegments[segmentCount].value1 = value1;
    timeSegments[segmentCount].value2 = value2;
    timeSegments[segmentCount].flags = 0;  // 清零所有标志位
    
    segmentCount++;
}

// ==========================================
// 便捷方法：常用动作的简化接口
// ==========================================

void SimpleGameStage::instant(unsigned long startTime, int pin, ActionType action, int value) {
    addSegment(startTime, 0, pin, action, value, 0);
}

void SimpleGameStage::duration(unsigned long startTime, unsigned long duration, int pin, 
                              ActionType action, int value1, int value2) {
    addSegment(startTime, duration, pin, action, value1, value2);
}

// ==========================================
// 专用快捷方法（最常用的几种）
// ==========================================

void SimpleGameStage::ledBreathing(unsigned long startTime, unsigned long duration, int pin, float cycleSeconds) {
    int cycleMs = (int)(cycleSeconds * 1000);
    addSegment(startTime, duration, pin, LED_BREATHING, cycleMs, 0);
}

void SimpleGameStage::ledFlash(unsigned long startTime, unsigned long duration, int pin, int intervalMs) {
    addSegment(startTime, duration, pin, LED_FLASH, intervalMs, 0);
}

void SimpleGameStage::pwmRamp(unsigned long startTime, unsigned long duration, int pin, int fromValue, int toValue) {
    addSegment(startTime, duration, pin, PWM_RAMP, fromValue, toValue);
}

void SimpleGameStage::digitalPulse(unsigned long startTime, unsigned long duration, int pin) {
    addSegment(startTime, duration, pin, DIGITAL_HIGH, 0, 0);
}

void SimpleGameStage::jumpToStage(unsigned long startTime, int nextStage) {
    Serial.print(F("⏰ 设置定时跳转: "));
    Serial.print(startTime);
    Serial.print(F("ms → Stage "));
    Serial.println(nextStage);
    addSegment(startTime, 0, -1, STAGE_JUMP, nextStage, 0);
}

// 新增：字符串版本的jumpToStage方法  
void SimpleGameStage::jumpToStage(unsigned long startTime, const String& nextStageId) {
    // 将字符串存储到临时变量中，在executeEndAction中使用
    pendingJumpStageId = nextStageId;
    Serial.print(F("⏰ 设置定时跳转: "));
    Serial.print(startTime);
    Serial.print(F("ms → Stage "));
    Serial.println(nextStageId);
    addSegment(startTime, 0, -1, STAGE_JUMP, -1, 0);  // value1=-1表示使用字符串版本
}

// 清空当前环节的所有时间段
void SimpleGameStage::clearStage() {
    segmentCount = 0;
    Serial.println("🧹 清空环节时间段");
}

// 状态查询
int SimpleGameStage::getCurrentStage() {
    return currentStage;
}

unsigned long SimpleGameStage::getStageTime() {
    return stageRunning ? (millis() - stageStartTime) : 0;
}

bool SimpleGameStage::isRunning() {
    return stageRunning;
}

int SimpleGameStage::getSegmentCount() {
    return segmentCount;
}

int SimpleGameStage::getActiveSegmentCount() {
    int count = 0;
    for (int i = 0; i < segmentCount; i++) {
        if (timeSegments[i].flags & 0x04) count++;  // 检查isActive标志位
    }
    return count;
}

// 调试方法
void SimpleGameStage::printStageInfo() {
    Serial.println("=== 环节信息 ===");
    Serial.println("当前环节: " + String(currentStage));
    Serial.println("运行状态: " + String(stageRunning ? "运行中" : "已停止"));
    Serial.println("环节时间: " + String(getStageTime()) + "ms");
    Serial.println("时间段数: " + String(segmentCount));
    Serial.println("活跃段数: " + String(getActiveSegmentCount()));
    Serial.println("================");
}

void SimpleGameStage::printActiveSegments() {
    Serial.println("=== 活跃时间段 ===");
    for (int i = 0; i < segmentCount; i++) {
        if (timeSegments[i].flags & 0x04) {  // 检查isActive标志位
            TimeSegment& seg = timeSegments[i];
            Serial.print("段");
            Serial.print(i);
            Serial.print(": ");
            Serial.print(seg.startTime);
            Serial.print("ms-");
            Serial.print(seg.startTime + seg.duration);  // 运行时计算endTime
            Serial.print("ms, Pin");
            Serial.print(seg.pin);
            Serial.print(", Action");
            Serial.println(seg.action);
        }
    }
}

// 新增：显示所有时间段信息（包括未激活的）
void SimpleGameStage::printAllSegments() {
    Serial.println(F("=== 所有时间段信息 ==="));
    Serial.print(F("环节: "));
    Serial.print(currentStage);
    Serial.print(F(", 总段数: "));
    Serial.print(segmentCount);
    Serial.print(F(", 运行状态: "));
    Serial.println(stageRunning ? F("运行中") : F("已停止"));
    
    if (!stageRunning) {
        Serial.println(F("环节未运行，无时间段信息"));
        return;
    }
    
    unsigned long currentTime = millis() - stageStartTime;
    Serial.print(F("当前时间: "));
    Serial.print(currentTime);
    Serial.println(F("ms"));
    
    for (int i = 0; i < segmentCount; i++) {
        TimeSegment& seg = timeSegments[i];
        Serial.print(F("段"));
        Serial.print(i);
        Serial.print(F(": "));
        Serial.print(seg.startTime);
        Serial.print(F("ms"));
        
        if (seg.duration > 0) {
            Serial.print(F("-"));
            Serial.print(seg.startTime + seg.duration);  // 运行时计算endTime
            Serial.print(F("ms"));
        } else {
            Serial.print(F("(瞬时)"));
        }
        
        Serial.print(F(", Pin"));
        Serial.print(seg.pin);
        Serial.print(F(", Action"));
        Serial.print(seg.action);
        
        // 特别标记STAGE_JUMP
        if (seg.action == STAGE_JUMP) {
            Serial.print(F(" [JUMP"));
            if (seg.value1 == -1) {
                Serial.print(F(" to '"));
                Serial.print(pendingJumpStageId);
                Serial.print(F("'"));
            } else {
                Serial.print(F(" to "));
                Serial.print(seg.value1);
            }
            Serial.print(F("]"));
        }
        
        Serial.print(F(", 状态: "));
        if (!(seg.flags & 0x01)) {
            Serial.print(F("等待"));
        } else if (seg.flags & 0x04) {
            Serial.print(F("活跃"));
        } else {
            Serial.print(F("完成"));
        }
        
        Serial.println();
    }
    Serial.println(F("========================"));
}

// 初始化方法
void SimpleGameStage::begin() {
    currentStage = -1;
    stageStartTime = 0;
    segmentCount = 0;
    stageRunning = false;
    
    #ifdef DEBUG
    Serial.println(F("SimpleGameStage初始化完成"));
    #endif
} 