/**
 * =============================================================================
 * CommandProcessor - 命令处理器库 - 实现文件
 * 版本: 1.0
 * 创建日期: 2024-12-28
 * =============================================================================
 */

#include "CommandProcessor.h"
#include "MillisPWM.h"
#include "UniversalHarbingerClient.h"
#include "DigitalIOController.h"
#include "GameFlowManager.h"
#include "GameStageStateMachine.h"
#include "BY_VoiceController_Unified.h"

// ========================== 全局实例 ==========================
CommandProcessor commandProcessor;
extern BY_VoiceController_Unified voice;

// ========================== 构造和初始化 ==========================
CommandProcessor::CommandProcessor() : initialized(false), customCommandCallback(nullptr) {
}

void CommandProcessor::begin() {
    initialized = true;
    
    #ifdef DEBUG
    Serial.println(F("CommandProcessor初始化完成"));
    #endif
}

// ========================== 命令处理 ==========================
bool CommandProcessor::processCommand(const String& input) {
    if (!initialized || input.length() == 0) return false;
    
    String command, params;
    if (!parseCommand(input, command, params)) {
        return false;
    }
    
    debugPrint("处理命令: " + command + " 参数: " + params);
    
    // 优先处理音频命令 (c1p, c2s, c1v20 等)
    if (processVoiceCommand(command, params)) {
        return true;
    }
    
    // 优先处理简化PWM命令 (p24, b24, s24)
    if (processSimplePWMCommand(command, params)) {
        return true;
    }
    
    // PWM相关命令
    if (processPWMCommand(command, params)) {
        return true;
    }
    
    // 数字IO命令
    if (processDigitalIOCommand(command, params)) {
        return true;
    }
    
    // 游戏协议命令
    if (processGameCommand(command, params)) {
        return true;
    }
    
    // 系统命令
    if (processSystemCommand(command, params)) {
        return true;
    }
    
    // 自定义命令回调
    if (customCommandCallback) {
        customCommandCallback(command, params);
        return true;
    }
    
    debugPrint("未知命令: " + command);
    return false;
}

// ========================== 具体命令处理器 ==========================

// ========================== 音频命令处理器 ==========================
bool CommandProcessor::processVoiceCommand(const String& command, const String& params) {
    // 通道播放命令: c1p, c2p, c3p, c4p
    if (command.length() == 3 && command.startsWith("c") && command.endsWith("p")) {
        int channel = command.charAt(1) - '0';
        if (channel >= 1 && channel <= 4) {
            voice.play(channel);
            Serial.print(F("🎵 播放通道"));
            Serial.println(channel);
            return true;
        }
    }
    
    // 通道停止命令: c1s, c2s, c3s, c4s
    if (command.length() == 3 && command.startsWith("c") && command.endsWith("s")) {
        int channel = command.charAt(1) - '0';
        if (channel >= 1 && channel <= 4) {
            voice.stop(channel);
            Serial.print(F("⏹️ 停止通道"));
            Serial.println(channel);
            return true;
        }
    }
    
    // 音量设置命令: c1v20, c2v15, 等
    if (command.length() >= 4 && command.startsWith("c") && command.charAt(2) == 'v') {
        int channel = command.charAt(1) - '0';
        if (channel >= 1 && channel <= 4) {
            int volume = command.substring(3).toInt();
            if (volume >= 0 && volume <= 30) {
                voice.setVolume(channel, volume);
                Serial.print(F("🔊 通道"));
                Serial.print(channel);
                Serial.print(F(" 音量设置为 "));
                Serial.println(volume);
                return true;
            }
        }
    }
    
    // 播放指定歌曲命令: c1:1234, c2:0001, 等
    if (command.length() >= 4 && command.startsWith("c") && command.charAt(2) == ':') {
        int channel = command.charAt(1) - '0';
        if (channel >= 1 && channel <= 4) {
            int songID = command.substring(3).toInt();
            if (songID > 0) {
                voice.playSong(channel, songID);
                Serial.print(F("🎵 通道"));
                Serial.print(channel);
                Serial.print(F(" 播放歌曲 "));
                Serial.println(songID);
                return true;
            }
        }
    }
    
    // 下一首命令: c1n, c2n, c3n, c4n
    if (command.length() == 3 && command.startsWith("c") && command.endsWith("n")) {
        int channel = command.charAt(1) - '0';
        if (channel >= 1 && channel <= 4) {
            voice.nextSong(channel);
            Serial.print(F("⏭️ 通道"));
            Serial.print(channel);
            Serial.println(F(" 下一首"));
            return true;
        }
    }
    
    // 上一首命令: c1b, c2b, c3b, c4b
    if (command.length() == 3 && command.startsWith("c") && command.endsWith("b")) {
        int channel = command.charAt(1) - '0';
        if (channel >= 1 && channel <= 4) {
            voice.prevSong(channel);
            Serial.print(F("⏮️ 通道"));
            Serial.print(channel);
            Serial.println(F(" 上一首"));
            return true;
        }
    }
    
    // 批量命令
    if (command == "playall") {
        voice.playAll();
        Serial.println(F("🎵 播放所有通道"));
        return true;
    }
    
    if (command == "stopall") {
        voice.stopAll();
        Serial.println(F("⏹️ 停止所有通道"));
        return true;
    }
    
    // 批量音量设置: volall:20
    if (command.startsWith("volall:")) {
        int volume = command.substring(7).toInt();
        if (volume >= 0 && volume <= 30) {
            voice.setVolumeAll(volume);
            Serial.print(F("🔊 所有通道音量设置为 "));
            Serial.println(volume);
            return true;
        }
    }
    
    // 测试命令
    if (command == "test1") {
        voice.playSong(1, 1);
        Serial.println(F("🎵 测试播放: 通道1播放歌曲1"));
        return true;
    }
    
    if (command == "test201") {
        voice.playSong(1, 201);
        Serial.println(F("🎵 测试播放: 通道1播放歌曲201"));
        return true;
    }
    
    if (command == "testall") {
        for (int i = 1; i <= 4; i++) {
            voice.playSong(i, i);
        }
        Serial.println(F("🎵 测试播放: 所有通道播放对应歌曲"));
        return true;
    }
    
    // 显示音频状态
    if (command == "vstatus" || command == "voice_status") {
        voice.printStatus();
        return true;
    }
    
    return false;
}

bool CommandProcessor::processPWMCommand(const String& command, const String& params) {
    int pin, value;
    
    if (command == "pwm_set") {
        if (parsePWMParams(params, pin, value)) {
            MillisPWM::setBrightness(pin, value);
            return true;
        }
        
    } else if (command == "pwm_breathing") {
        if (parsePWMParams(params, pin, value)) {
            float cyclePeriodSeconds = value / 1000.0;
            MillisPWM::startBreathing(pin, cyclePeriodSeconds);
            return true;
        }
        
    } else if (command == "pwm_stop") {
        if (parsePWMParams(params, pin, value)) {
            MillisPWM::stop(pin);
            return true;
        }
        
    } else if (command == "pwm_stop_all") {
        MillisPWM::stopAll();
        return true;
        
    } else if (command == "pwm_range_breathing") {
        // 解析范围参数 startPin,endPin,minCycle,maxCycle
        int commaPos1 = params.indexOf(',');
        int commaPos2 = params.indexOf(',', commaPos1 + 1);
        int commaPos3 = params.indexOf(',', commaPos2 + 1);
        
        if (commaPos1 > 0 && commaPos2 > commaPos1 && commaPos3 > commaPos2) {
            int startPin = params.substring(0, commaPos1).toInt();
            int endPin = params.substring(commaPos1 + 1, commaPos2).toInt();
            float minCycle = params.substring(commaPos2 + 1, commaPos3).toFloat();
            float maxCycle = params.substring(commaPos3 + 1).toFloat();
            
            MillisPWM::startRangeBreathing(startPin, endPin, minCycle, maxCycle);
            return true;
        }
        
    // ========================== Fade渐变命令 ==========================
    } else if (command == "pwm_fadein") {
        // 解析参数 pin,targetValue,durationMs 或 pin,durationMs
        int commaPos1 = params.indexOf(',');
        int commaPos2 = params.indexOf(',', commaPos1 + 1);
        
        if (commaPos1 > 0) {
            int pin = params.substring(0, commaPos1).toInt();
            if (commaPos2 > commaPos1) {
                // 三个参数: pin,targetValue,durationMs
                int targetValue = params.substring(commaPos1 + 1, commaPos2).toInt();
                int durationMs = params.substring(commaPos2 + 1).toInt();
                MillisPWM::fadeIn(pin, targetValue, durationMs);
            } else {
                // 两个参数: pin,durationMs (目标值默认255)
                int durationMs = params.substring(commaPos1 + 1).toInt();
                MillisPWM::fadeIn(pin, 255, durationMs);
            }
            return true;
        }
        
    } else if (command == "pwm_fadeout") {
        // 解析参数 pin,durationMs
        if (parsePWMParams(params, pin, value)) {
            MillisPWM::fadeOut(pin, value);
            return true;
        }
        
    } else if (command == "pwm_fadeto") {
        // 解析参数 pin,targetValue,durationMs
        int commaPos1 = params.indexOf(',');
        int commaPos2 = params.indexOf(',', commaPos1 + 1);
        
        if (commaPos1 > 0 && commaPos2 > commaPos1) {
            int pin = params.substring(0, commaPos1).toInt();
            int targetValue = params.substring(commaPos1 + 1, commaPos2).toInt();
            int durationMs = params.substring(commaPos2 + 1).toInt();
            MillisPWM::fadeTo(pin, targetValue, durationMs);
            return true;
        }
        
    } else if (command == "pwm_stop_fade") {
        if (parsePWMParams(params, pin, value)) {
            MillisPWM::stopFade(pin);
            return true;
        }
    }
    
    return false;
}

bool CommandProcessor::processGameCommand(const String& command, const String& params) {
    // UGP协议命令
    if (command == "INIT" || command == "START" || command == "STOP" || 
        command == "PAUSE" || command == "RESUME" || command == "EMERGENCY_STOP") {
        return gameStateMachine.processGameCommand(command, params);
    }
    
    // 环节测试命令 - 委托给GameFlowManager处理
    if (command.startsWith("stage_") || command.indexOf("-") > 0) {
        return gameFlowManager.startStage(command);
    }
    
    // 游戏流程控制命令
    if (command == "game_stop" || command == "stop_game") {
        gameFlowManager.stopAllStages();
        return true;
    } else if (command == "game_status") {
        gameFlowManager.printStatus();
        return true;
    } else if (command == "game_stages") {
        gameFlowManager.printAvailableStages();
        return true;
    } else if (command == "game_debug" || command == "debug_segments") {
        gameStage.printAllSegments();
        return true;
    }
    
    return false;
}

bool CommandProcessor::processDigitalIOCommand(const String& command, const String& params) {
    // 重构完整命令字符串传递给DigitalIOController
    String fullCommand = command;
    if (params.length() > 0) {
        fullCommand += ":" + params;
    }
    
    if (DigitalIOController::processCommand(fullCommand)) {
        return true;
    }
    
    // 自定义数字IO命令
    int pin, value;
    
    // 输出命令: o24h (引脚24输出高电平)
    if (command.startsWith("o") && command.length() >= 3) {
        int pin = command.substring(1, command.length()-1).toInt();
        bool level = command.endsWith("h") || command.endsWith("H");
        return DigitalIOController::setOutput(pin, level);
        
    // 脉冲命令: p24:1000 (引脚24脉冲1秒) - 注意与PWM命令区分
    } else if (command.indexOf(':') > 0 && command.startsWith("pulse")) {
        int colonPos = command.indexOf(':');
        int pin = command.substring(5, colonPos).toInt();
        unsigned long width = command.substring(colonPos + 1).toInt();
        return DigitalIOController::pulseOutput(pin, width);
        
    // 定时命令: t24h:500:2000 (引脚24，高电平，延迟500ms，持续2000ms)
    } else if (command.startsWith("t") && command.indexOf(':') > 0) {
        int colon1 = command.indexOf(':');
        int colon2 = command.indexOf(':', colon1 + 1);
        if (colon2 > 0) {
            String pinLevel = command.substring(1, colon1);
            int pin = pinLevel.substring(0, pinLevel.length()-1).toInt();
            bool level = pinLevel.endsWith("h") || pinLevel.endsWith("H");
            unsigned long delay = command.substring(colon1 + 1, colon2).toInt();
            unsigned long duration = command.substring(colon2 + 1).toInt();
            return DigitalIOController::scheduleOutput(pin, level, delay, duration);
        }
        
    // 输入监控命令: i24 (监控引脚24)
    } else if (command.startsWith("i") && command.length() >= 3) {
        int pin = command.substring(1).toInt();
        return DigitalIOController::startInput(pin, INPUT_CHANGE, 100);
        
    // 数字IO状态查询
    } else if (command == "dio_status") {
        Serial.print(F("活跃输出通道: "));
        Serial.println(DigitalIOController::getActiveOutputCount());
        Serial.print(F("活跃输入通道: "));
        Serial.println(DigitalIOController::getActiveInputCount());
        Serial.print(F("系统运行时间: "));
        Serial.println(DigitalIOController::getSystemUptime());
        return true;
        
    // 停止所有数字IO
    } else if (command == "dio_stop_all") {
        DigitalIOController::stopAllOutputs();
        DigitalIOController::stopAllInputs();
        return true;
    }
    
    return false;
}

bool CommandProcessor::processSystemCommand(const String& command, const String& params) {
    if (command == "help" || command == "h") {
        showHelp();
        return true;
        
    } else if (command == "status") {
        showStatus();
        return true;
        
    } else if (command == "reset") {
        // 重置所有系统
        MillisPWM::stopAll();
        gameStateMachine.setState(GAME_IDLE);
        debugPrint("系统已重置");
        return true;
        
    } else if (command == "debug") {
        #ifdef DEBUG
        Serial.println(F("=== 调试信息 ==="));
        gameStateMachine.printStatus();
        Serial.print(F("活跃PWM通道: "));
        Serial.println(MillisPWM::getActiveCount());
        #endif
        return true;
    }
    
    return false;
}

bool CommandProcessor::processSimplePWMCommand(const String& command, const String& params) {
    // 排除数字IO命令 (o24h, t24h, i24等)
    if (command.startsWith("o") && (command.endsWith("h") || command.endsWith("l"))) {
        return false; // 这是数字IO命令，不是PWM命令
    }
    if (command.startsWith("t") && (command.endsWith("h") || command.endsWith("l"))) {
        return false; // 这是定时输出命令
    }
    if (command.startsWith("i") && command.length() >= 3 && params.length() == 0) {
        return false; // 这是输入监控命令
    }
    
    int pin = extractPinFromCommand(command);
    if (pin == -1) return false;
    
    // p24 128 - 设置PWM
    if (command.startsWith("p") && !command.startsWith("pa")) {
        int value = params.toInt();
        if (value >= 0 && value <= 255) {
            MillisPWM::setBrightness(pin, value);
            return true;
        }
        
    // b24 1000 - 呼吸灯 (单个)
    } else if (command.startsWith("b") && !command.startsWith("ba")) {
        int cyclePeriodMs = params.toInt();
        if (cyclePeriodMs > 0) {
            float cyclePeriodSeconds = cyclePeriodMs / 1000.0;
            MillisPWM::startBreathing(pin, cyclePeriodSeconds);
            return true;
        }
        
    // ba10 1000 - 批量呼吸灯 (从pin开始的10个连续引脚)
    } else if (command.startsWith("ba")) {
        int cyclePeriodMs = params.toInt();
        if (cyclePeriodMs > 0) {
            // 从指定引脚开始，启动连续10个引脚的呼吸灯
            float cyclePeriodSeconds = cyclePeriodMs / 1000.0;
            for (int i = 0; i < 10; i++) {
                MillisPWM::startBreathing(pin + i, cyclePeriodSeconds);
            }
            return true;
        }
        
    // pa24 128 - 批量设置PWM (从pin开始的10个连续引脚)
    } else if (command.startsWith("pa")) {
        int value = params.toInt();
        if (value >= 0 && value <= 255) {
            // 从指定引脚开始，设置连续10个引脚的PWM
            for (int i = 0; i < 10; i++) {
                MillisPWM::setBrightness(pin + i, value);
            }
            return true;
        }
        
    // s24 - 停止单个
    } else if (command.startsWith("s") && !command.startsWith("sa")) {
        MillisPWM::stop(pin);
        return true;
        
    // sa24 - 批量停止 (从pin开始的10个连续引脚)
    } else if (command.startsWith("sa")) {
        // 从指定引脚开始，停止连续10个引脚
        for (int i = 0; i < 10; i++) {
            MillisPWM::stop(pin + i);
        }
        return true;
        
    // ========================== Fade渐变简化命令 ==========================
    // f24 1000 - 淡入效果 (pin24, 1秒)
    } else if (command.startsWith("f") && !command.startsWith("fo") && !command.startsWith("ft")) {
        int durationMs = params.toInt();
        if (durationMs > 0) {
            MillisPWM::fadeIn(pin, 255, durationMs);
            return true;
        }
        
    // fo24 1000 - 淡出效果 (pin24, 1秒)
    } else if (command.startsWith("fo")) {
        int durationMs = params.toInt();
        if (durationMs > 0) {
            MillisPWM::fadeOut(pin, durationMs);
            return true;
        }
        
    // ft24 128 1000 - 渐变到指定亮度 (pin24, 亮度128, 1秒)
    } else if (command.startsWith("ft")) {
        int spacePos = params.indexOf(' ');
        if (spacePos > 0) {
            int targetValue = params.substring(0, spacePos).toInt();
            int durationMs = params.substring(spacePos + 1).toInt();
            if (targetValue >= 0 && targetValue <= 255 && durationMs > 0) {
                MillisPWM::fadeTo(pin, targetValue, durationMs);
                return true;
            }
        }
        
    // fs24 - 停止渐变效果
    } else if (command.startsWith("fs")) {
        MillisPWM::stopFade(pin);
        return true;
    }
    
    return false;
}

// ========================== 帮助和状态 ==========================
void CommandProcessor::showHelp() {
    Serial.println(F("=== 命令帮助 ==="));
    Serial.println(F("简化PWM命令:"));
    Serial.println(F("  p<pin> <value>   - 设置PWM (如: p24 128)"));
    Serial.println(F("  b<pin> <period>  - 呼吸灯毫秒 (如: b24 1000)"));
    Serial.println(F("  s<pin>           - 停止PWM (如: s24)"));
    Serial.println();
    Serial.println(F("简化Fade渐变命令:"));
    Serial.println(F("  f<pin> <duration>      - 淡入到最亮 (如: f24 1000)"));
    Serial.println(F("  fo<pin> <duration>     - 淡出到0 (如: fo24 1000)"));
    Serial.println(F("  ft<pin> <target> <dur> - 渐变到指定亮度 (如: ft24 128 1000)"));
    Serial.println(F("  fs<pin>                - 停止渐变 (如: fs24)"));
    Serial.println();
    Serial.println(F("批量PWM命令 (连续10个引脚):"));
    Serial.println(F("  pa<pin> <value>  - 批量设置PWM (如: pa20 128)"));
    Serial.println(F("  ba<pin> <period> - 批量呼吸灯 (如: ba10 1000)"));
    Serial.println(F("  sa<pin>          - 批量停止 (如: sa20)"));
    Serial.println();
    Serial.println(F("完整PWM命令:"));
    Serial.println(F("  pwm_set:<pin>,<value>"));
    Serial.println(F("  pwm_breathing:<pin>,<period_ms>"));
    Serial.println(F("  pwm_stop:<pin>"));
    Serial.println(F("  pwm_stop_all"));
    Serial.println();
    Serial.println(F("完整Fade命令:"));
    Serial.println(F("  pwm_fadein:<pin>,<target>,<duration>   - 淡入"));
    Serial.println(F("  pwm_fadeout:<pin>,<duration>           - 淡出"));
    Serial.println(F("  pwm_fadeto:<pin>,<target>,<duration>   - 渐变至"));
    Serial.println(F("  pwm_stop_fade:<pin>                    - 停止渐变"));
    Serial.println();
    Serial.println(F("数字IO命令:"));
    Serial.println(F("  o<pin>h/l            - 输出高/低电平 (如: o24h)"));
    Serial.println(F("  pulse<pin>:<width>   - 脉冲输出 (如: pulse24:1000)"));
    Serial.println(F("  t<pin>h/l:<delay>:<duration> - 定时输出 (如: t24h:500:2000)"));
    Serial.println(F("  i<pin>               - 监控输入变化 (如: i25)"));
    Serial.println(F("  dio_status           - 数字IO状态"));
    Serial.println(F("  dio_stop_all         - 停止所有数字IO"));
    Serial.println();
    Serial.println(F("游戏命令:"));
    Serial.println(F("  INIT, START, STOP, PAUSE, RESUME"));
    Serial.println(F("  <stage_id>    - 启动指定环节 (如: 001-0, stage_001_1)"));
    Serial.println(F("  game_stop     - 停止所有游戏环节"));
    Serial.println(F("  game_status   - 查看游戏流程状态"));
    Serial.println(F("  game_stages   - 查看所有可用环节"));
    Serial.println(F("  game_debug    - 显示时间段调试信息"));
    Serial.println();
    Serial.println(F("语音控制命令:"));
    Serial.println(F("  c1p, c2p, c3p, c4p   - 播放通道1-4"));
    Serial.println(F("  c1s, c2s, c3s, c4s   - 停止通道1-4"));
    Serial.println(F("  c1:1234              - 播放指定歌曲 (如: c1:1234)"));
    Serial.println(F("  c1v20                - 设置音量 (如: c1v20, 音量0-30)"));
    Serial.println(F("  c1n, c1b             - 下一首/上一首"));
    Serial.println(F("  s/status             - 显示播放状态"));
    Serial.println();
    Serial.println(F("系统命令:"));
    Serial.println(F("  h/help    - 显示帮助"));
    Serial.println(F("  status    - 显示状态"));
    Serial.println(F("  reset     - 重置系统"));
    Serial.println(F("  debug     - 调试信息"));
    Serial.println(F("  time      - 显示系统时间"));
    Serial.println(F("  test_unified - 统一输出管理器测试"));
    Serial.println();
    Serial.println(F("网络命令 (如果启用):"));
    Serial.println(F("  network   - 显示网络状态"));
    Serial.println(F("  send <msg> - 发送测试消息"));
}

void CommandProcessor::showStatus() {
    Serial.println(F("=== 系统状态 ==="));
    
    Serial.print(F("游戏状态: "));
    Serial.println(GameStateMachine::getStateString(gameStateMachine.getState()));
    
    Serial.print(F("活跃PWM通道: "));
    Serial.println(MillisPWM::getActiveCount());
    
    Serial.print(F("活跃输出通道: "));
    Serial.println(DigitalIOController::getActiveOutputCount());
    
    Serial.print(F("活跃输入通道: "));
    Serial.println(DigitalIOController::getActiveInputCount());
    
    Serial.print(F("系统运行时间: "));
    Serial.print(DigitalIOController::getSystemUptime());
    Serial.println(F("ms"));
    
    Serial.print(F("会话ID: "));
    Serial.println(gameStateMachine.getSessionId());
    
    #ifdef DEBUG
    extern int freeMemory();
    Serial.print(F("自由内存: "));
    Serial.println(freeMemory());
    #endif
}

// ========================== 回调设置 ==========================
void CommandProcessor::setCustomCommandCallback(void (*callback)(const String& command, const String& params)) {
    customCommandCallback = callback;
}

// ========================== 内部解析函数 ==========================
bool CommandProcessor::parseCommand(const String& input, String& command, String& params) {
    String trimmedInput = input;
    trimmedInput.trim();
    
    int colonPos = trimmedInput.indexOf(':');
    int spacePos = trimmedInput.indexOf(' ');
    
    // 优先使用冒号分隔
    if (colonPos > 0) {
        command = trimmedInput.substring(0, colonPos);
        params = trimmedInput.substring(colonPos + 1);
        params.trim();
        return true;
    }
    
    // 其次使用空格分隔
    if (spacePos > 0) {
        command = trimmedInput.substring(0, spacePos);
        params = trimmedInput.substring(spacePos + 1);
        params.trim();
        return true;
    }
    
    // 无参数命令
    command = trimmedInput;
    params = "";
    return true;
}

bool CommandProcessor::parsePWMParams(const String& params, int& pin, int& value) {
    int commaPos = params.indexOf(',');
    int spacePos = params.indexOf(' ');
    
    // 支持逗号和空格分隔
    int separatorPos = -1;
    if (commaPos > 0) separatorPos = commaPos;
    else if (spacePos > 0) separatorPos = spacePos;
    
    if (separatorPos > 0) {
        pin = params.substring(0, separatorPos).toInt();
        value = params.substring(separatorPos + 1).toInt();
        return true;
    }
    
    return false;
}

bool CommandProcessor::parseSimpleParams(const String& params, int& pin, int& value) {
    // 简化命令的参数解析
    String trimmedParams = params;
    trimmedParams.trim();
    
    if (trimmedParams.length() > 0) {
        value = trimmedParams.toInt();
        return true;
    }
    
    return false;
}

int CommandProcessor::extractPinFromCommand(const String& command) {
    // 从p24, b24, s24, ba10, pa24, sa24, fo24, ft24, fs24这样的命令中提取引脚号
    if (command.length() < 2) return -1;
    
    String pinStr;
    
    // 处理双字母命令 (ba, pa, sa, fo, ft, fs)
    if (command.startsWith("ba") || command.startsWith("pa") || command.startsWith("sa") ||
        command.startsWith("fo") || command.startsWith("ft") || command.startsWith("fs")) {
        if (command.length() < 3) return -1;
        pinStr = command.substring(2);
    }
    // 处理单字母命令 (b, p, s, f)
    else {
        pinStr = command.substring(1);
    }
    
    int pin = pinStr.toInt();
    
    // 验证引脚号范围
    if (pin >= 0 && pin <= 99) {
        return pin;
    }
    
    return -1;
}

// ========================== 手动协议发送 ==========================
bool CommandProcessor::processManualProtocol(const String& command, const String& params) {
    if (command == "send_init") {
        return gameStateMachine.processGameCommand("INIT", "");
    } else if (command == "send_start") {
        return gameStateMachine.processGameCommand("START", "session_id=TEST_001");
    } else if (command == "send_stop") {
        return gameStateMachine.processGameCommand("STOP", "session_id=" + gameStateMachine.getSessionId());
    } else if (command == "send_pause") {
        return gameStateMachine.processGameCommand("PAUSE", "session_id=" + gameStateMachine.getSessionId());
    } else if (command == "send_resume") {
        return gameStateMachine.processGameCommand("RESUME", "session_id=" + gameStateMachine.getSessionId());
    } else if (command == "send_emergency") {
        return gameStateMachine.processGameCommand("EMERGENCY_STOP", "session_id=" + gameStateMachine.getSessionId());
    } else if (command == "send_heartbeat") {
        harbingerClient.sendINFOMessage("HEARTBEAT", "status=OK");
        return true;
    } else if (command == "send_game_end") {
        String params = "result=COMPLETED,session_id=" + gameStateMachine.getSessionId();
        harbingerClient.sendGAMEResponse("GAME_END", params);
        return true;
    } else if (command.startsWith("send_trigger:")) {
        String deviceId = command.substring(13);
        harbingerClient.sendHARDResponse(deviceId, "TRIGGERED");
        return true;
    } else if (command.startsWith("send_custom:")) {
        String msg = command.substring(12);
        harbingerClient.sendMessage(msg);
        return true;
    }
    
    return false;
}

// ========================== 调试功能 ==========================
void CommandProcessor::debugPrint(const String& message) {
    #ifdef DEBUG
    Serial.print(F("CommandProcessor: "));
    Serial.println(message);
    #endif
} 