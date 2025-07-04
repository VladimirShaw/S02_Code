/**
 * =============================================================================
 * BY_VoiceController_Unified - 统一的4路BY语音模块控制器 - 实现文件
 * 版本: v3.0 - C101 IO控制适配版本
 * 创建日期: 2025-01-03
 * 
 * 说明: C101版本使用IO控制音频模块，但保持与C102相同的外部接口
 * =============================================================================
 */

#include "BY_VoiceController_Unified.h"
#include "C101_SimpleConfig.h"  // 引入C101配置

// ========================== BY_VoiceModule_Unified 实现 ==========================
// C101版本：保持接口但不实际使用串口

BY_VoiceModule_Unified::BY_VoiceModule_Unified() {
    serialPort = nullptr;
}

void BY_VoiceModule_Unified::init(Stream* serial) {
    serialPort = serial;
    // C101版本：不实际使用串口，但保持接口兼容
}

// 按照BY_Demo验证过的CRC计算方式 (XOR) - 保持兼容
byte BY_VoiceModule_Unified::calculateCRC(byte* p, byte cNum) {
    byte CRC_Result = 0;
    for (char i = 0; i < cNum; i++) {
        CRC_Result ^= *p;
        p++;
    }
    return CRC_Result;
}

// 按照BY_Demo验证过的帧发送方式 - C101版本不实际发送
void BY_VoiceModule_Unified::sendFrameData(byte* pData) {
    // C101版本：保持接口但不实际发送数据
    // 实际的IO控制在BY_VoiceController_Unified中处理
}

// 基本命令发送 - C101版本不实际发送
void BY_VoiceModule_Unified::sendCommand(byte cmdType) {
    // C101版本：保持接口但不实际发送命令
}

// 参数命令发送 - C101版本不实际发送
void BY_VoiceModule_Unified::sendCommand(byte cmdType, byte* param) {
    // C101版本：保持接口但不实际发送命令
}

// ========================== 基本控制方法 - C101版本不实际控制 ==========================

void BY_VoiceModule_Unified::play() {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::pause() {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::stop() {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::nextSong() {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::prevSong() {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::reset() {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::fastForward() {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::fastBackward() {
    // C101版本：保持接口兼容
}

// ========================== 高级控制方法 - C101版本不实际控制 ==========================

void BY_VoiceModule_Unified::setVolume(byte volume) {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::setEQ(byte eq) {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::setCycle(byte cycle) {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::selectSong(int songID) {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::selectFolderSong(byte folderID, byte songID) {
    // C101版本：保持接口兼容
}

void BY_VoiceModule_Unified::playSong(int songID) {
    // C101版本：保持接口兼容
}

// ========================== BY_VoiceController_Unified 实现 ==========================
// C101版本：使用IO控制音频模块

BY_VoiceController_Unified::BY_VoiceController_Unified() {
    softSerial = nullptr;
    initialized = false;
    lastStatusCheck = 0;
    
    // 使用C101配置的引脚
    softRX = C101_SOFT_RX_PIN; 
    softTX = C101_SOFT_TX_PIN;
    
    // 使用C101配置的BUSY引脚
    for (int i = 0; i < 4; i++) {
        busyPins[i] = C101_BUSY_PINS[i];
    }
    
    // 初始化状态
    for (int i = 0; i < 4; i++) {
        busyStates[i] = false;
        lastBusyStates[i] = false;
    }
}

BY_VoiceController_Unified::~BY_VoiceController_Unified() {
    if (softSerial != nullptr) {
        delete softSerial;
    }
}

// ========================== 灵活配置接口 ==========================

void BY_VoiceController_Unified::setSoftSerialPins(int rxPin, int txPin) {
    softRX = rxPin;
    softTX = txPin;
    Serial.print(F("🔧 C101设置虚拟软串口引脚: RX="));
    Serial.print(rxPin);
    Serial.print(F(", TX="));
    Serial.println(txPin);
}

void BY_VoiceController_Unified::setBusyPin(int channel, int pin) {
    if (channel >= 1 && channel <= 4) {
        busyPins[channel - 1] = pin;
        Serial.print(F("🔧 C101设置通道"));
        Serial.print(channel);
        Serial.print(F(" 状态监控引脚: "));
        Serial.println(pin);
    }
}

void BY_VoiceController_Unified::setBusyPins(int pin1, int pin2, int pin3, int pin4) {
    busyPins[0] = pin1;
    busyPins[1] = pin2;
    busyPins[2] = pin3;
    busyPins[3] = pin4;
    Serial.print(F("🔧 C101批量设置状态监控引脚: "));
    Serial.print(pin1); Serial.print(F(","));
    Serial.print(pin2); Serial.print(F(","));
    Serial.print(pin3); Serial.print(F(","));
    Serial.println(pin4);
}

// ========================== 初始化和系统控制 ==========================

bool BY_VoiceController_Unified::begin() {
    Serial.println(F("=== C101 IO控制音频模块初始化 ==="));
    
    // C101版本：不使用软串口，但保持接口兼容
    Serial.println(F("📢 C101使用IO控制，不需要软串口"));
    
    // 初始化所有状态监控引脚
    for (int i = 0; i < 4; i++) {
        pinMode(busyPins[i], INPUT_PULLUP);
        Serial.print(F("✓ 通道"));
        Serial.print(i + 1);
        Serial.print(F(" 状态监控引脚"));
        Serial.print(busyPins[i]);
        Serial.println(F(" 初始化完成"));
    }
    
    // 初始化IO控制引脚（在C101_SimpleConfig.h的initC101Hardware中已完成）
    Serial.println(F("✓ IO控制引脚已在硬件初始化中完成"));
    
    initialized = true;
    Serial.println(F("✅ C101 IO控制音频模块初始化成功"));
    return true;
}

// ========================== C101 IO控制实现 ==========================

void BY_VoiceController_Unified::playIOAudio(int channel) {
    if (channel < 1 || channel > 4) return;
    
    int idx = channel - 1;
    int io1Pin = C101_AUDIO_IO1_PINS[idx];
    int io2Pin = C101_AUDIO_IO2_PINS[idx];
    
    Serial.print(F("🎵 C101播放音频通道"));
    Serial.print(channel);
    Serial.print(F(" IO1="));
    Serial.print(io1Pin);
    Serial.print(F(" IO2="));
    Serial.println(io2Pin);
    
    // IO控制逻辑：(0,1)播放音频
    digitalWrite(io1Pin, LOW);   // IO1=0
    digitalWrite(io2Pin, HIGH);  // IO2=1
    delay(1000);  // 等待1秒
    
    // 复位到默认状态 (1,1)
    digitalWrite(io1Pin, HIGH);  // IO1=1
    digitalWrite(io2Pin, HIGH);  // IO2=1
}

void BY_VoiceController_Unified::stopIOAudio(int channel) {
    if (channel < 1 || channel > 4) return;
    
    int idx = channel - 1;
    int io1Pin = C101_AUDIO_IO1_PINS[idx];
    int io2Pin = C101_AUDIO_IO2_PINS[idx];
    
    Serial.print(F("⏹️ C101停止音频通道"));
    Serial.print(channel);
    Serial.print(F(" IO1="));
    Serial.print(io1Pin);
    Serial.print(F(" IO2="));
    Serial.println(io2Pin);
    
    // IO控制逻辑：(1,0)停止音频（切换到空文件）
    digitalWrite(io1Pin, HIGH);  // IO1=1
    digitalWrite(io2Pin, LOW);   // IO2=0
    delay(1000);  // 等待1秒
    
    // 复位到默认状态 (1,1)
    digitalWrite(io1Pin, HIGH);  // IO1=1
    digitalWrite(io2Pin, HIGH);  // IO2=1
}

void BY_VoiceController_Unified::resetIOAudio(int channel) {
    if (channel < 1 || channel > 4) return;
    
    int idx = channel - 1;
    int io1Pin = C101_AUDIO_IO1_PINS[idx];
    int io2Pin = C101_AUDIO_IO2_PINS[idx];
    
    Serial.print(F("🔄 C101重置音频通道"));
    Serial.println(channel);
    
    // 直接设置为默认状态 (1,1)
    digitalWrite(io1Pin, HIGH);  // IO1=1
    digitalWrite(io2Pin, HIGH);  // IO2=1
}

void BY_VoiceController_Unified::resetAllIOAudio() {
    Serial.println(F("🔄 C101重置所有音频通道"));
    for (int i = 1; i <= 4; i++) {
        resetIOAudio(i);
    }
}

// ========================== 兼容C102接口的语音控制 ==========================

void BY_VoiceController_Unified::play(int channel) {
    playIOAudio(channel);  // C101使用IO控制
}

void BY_VoiceController_Unified::stop(int channel) {
    stopIOAudio(channel);  // C101使用IO控制
}

void BY_VoiceController_Unified::pause(int channel) {
    stopIOAudio(channel);  // C101版本：暂停等同于停止
}

void BY_VoiceController_Unified::nextSong(int channel) {
    // C101版本：不支持下一首，但保持接口兼容
    Serial.print(F("⚠️ C101通道"));
    Serial.print(channel);
    Serial.println(F(" 不支持下一首功能"));
}

void BY_VoiceController_Unified::prevSong(int channel) {
    // C101版本：不支持上一首，但保持接口兼容
    Serial.print(F("⚠️ C101通道"));
    Serial.print(channel);
    Serial.println(F(" 不支持上一首功能"));
}

void BY_VoiceController_Unified::setVolume(int channel, int volume) {
    // C101版本：不支持音量调节，但保持接口兼容
    Serial.print(F("⚠️ C101通道"));
    Serial.print(channel);
    Serial.println(F(" 不支持音量调节功能"));
}

void BY_VoiceController_Unified::playSong(int channel, int songID) {
    // C101版本：直接播放（不支持选歌）
    Serial.print(F("🎵 C101通道"));
    Serial.print(channel);
    Serial.print(F(" 播放音频（歌曲ID:"));
    Serial.print(songID);
    Serial.println(F("）"));
    playIOAudio(channel);
}

// ========================== 批量控制 ==========================

void BY_VoiceController_Unified::playAll() {
    Serial.println(F("🎵 C101播放所有音频通道"));
    for (int i = 1; i <= 4; i++) {
        playIOAudio(i);
        delay(100);  // 间隔100ms避免冲突
    }
}

void BY_VoiceController_Unified::stopAll() {
    Serial.println(F("⏹️ C101停止所有音频通道"));
    for (int i = 1; i <= 4; i++) {
        stopIOAudio(i);
        delay(100);  // 间隔100ms避免冲突
    }
}

void BY_VoiceController_Unified::setVolumeAll(int volume) {
    // C101版本：不支持音量调节，但保持接口兼容
    Serial.println(F("⚠️ C101不支持音量调节功能"));
}

// ========================== 状态查询 ==========================

bool BY_VoiceController_Unified::isBusy(int channel) {
    if (channel < 1 || channel > 4 || !initialized) return false;
    return digitalRead(busyPins[channel - 1]) == LOW;  // C101使用LOW表示忙碌
}

void BY_VoiceController_Unified::update() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    if (currentTime - lastStatusCheck >= STATUS_CHECK_INTERVAL) {
        lastStatusCheck = currentTime;
        
        // 检查所有通道的状态变化
        for (int i = 0; i < 4; i++) {
            busyStates[i] = digitalRead(busyPins[i]) == LOW;  // C101使用LOW表示忙碌
            
            if (busyStates[i] != lastBusyStates[i]) {
                Serial.print(F("C101通道"));
                Serial.print(i + 1);
                Serial.print(F(" 状态: "));
                Serial.println(busyStates[i] ? F("忙碌") : F("空闲"));
                lastBusyStates[i] = busyStates[i];
            }
        }
    }
}

void BY_VoiceController_Unified::printStatus() {
    if (!initialized) {
        Serial.println(F("❌ C101 IO控制音频模块未初始化"));
        return;
    }
    
    Serial.println(F("=== C101 IO控制音频模块状态 ==="));
    Serial.print(F("虚拟软串口: RX="));
    Serial.print(softRX);
    Serial.print(F(", TX="));
    Serial.println(softTX);
    
    for (int i = 0; i < 4; i++) {
        Serial.print(F("通道"));
        Serial.print(i + 1);
        Serial.print(F(": IO1="));
        Serial.print(C101_AUDIO_IO1_PINS[i]);
        Serial.print(F(", IO2="));
        Serial.print(C101_AUDIO_IO2_PINS[i]);
        Serial.print(F(", 状态监控="));
        Serial.print(busyPins[i]);
        Serial.print(F(" ("));
        Serial.print(isBusy(i + 1) ? F("忙碌") : F("空闲"));
        Serial.println(F(")"));
    }
}

// ========================== 兼容接口 ==========================

void BY_VoiceController_Unified::reset() {
    resetAllIOAudio();
}

void BY_VoiceController_Unified::sendCommand(byte cmdType) {
    // C101版本：保持接口兼容但不实际发送命令
    Serial.print(F("⚠️ C101不支持串口命令发送: 0x"));
    Serial.println(cmdType, HEX);
}

// ========================== 命令处理接口 ==========================

void BY_VoiceController_Unified::processSerialCommand(String command) {
    command.trim();
    
    if (command == "help") {
        printHelp();
    } else if (command == "status") {
        printStatus();
    } else if (command == "vstatus") {
        printStatus();  // C101版本：vstatus等同于status
    } else if (command == "playall") {
        playAll();
    } else if (command == "stopall") {
        stopAll();
    } else if (command == "reset") {
        reset();
    } else if (command.startsWith("c") && command.endsWith("p")) {
        // 解析 c1p, c2p, c3p, c4p
        int channel = command.substring(1, command.length()-1).toInt();
        if (channel >= 1 && channel <= 4) {
            play(channel);
        }
    } else if (command.startsWith("c") && command.endsWith("s")) {
        // 解析 c1s, c2s, c3s, c4s
        int channel = command.substring(1, command.length()-1).toInt();
        if (channel >= 1 && channel <= 4) {
            stop(channel);
        }
    } else if (command.startsWith("c") && command.indexOf(":") > 0) {
        // 解析 c1:1001, c2:1002 等
        int colonPos = command.indexOf(":");
        int channel = command.substring(1, colonPos).toInt();
        int songID = command.substring(colonPos + 1).toInt();
        if (channel >= 1 && channel <= 4) {
            playSong(channel, songID);
        }
    } else if (command == "test1") {
        Serial.println(F("🧪 C101测试通道1"));
        play(1);
        delay(2000);
        stop(1);
    } else if (command == "testall") {
        Serial.println(F("🧪 C101测试所有通道"));
        playAll();
        delay(3000);
        stopAll();
    } else {
        Serial.print(F("❓ C101未知命令: "));
        Serial.println(command);
    }
}

void BY_VoiceController_Unified::printHelp() {
    Serial.println(F("=== C101 IO控制音频模块命令帮助 ==="));
    Serial.println(F("基础命令:"));
    Serial.println(F("  help      - 显示帮助信息"));
    Serial.println(F("  status    - 显示模块状态"));
    Serial.println(F("  vstatus   - 显示详细状态"));
    Serial.println(F("  reset     - 重置所有通道"));
    Serial.println(F(""));
    Serial.println(F("音频控制:"));
    Serial.println(F("  c1p, c2p, c3p, c4p  - 播放通道1-4"));
    Serial.println(F("  c1s, c2s, c3s, c4s  - 停止通道1-4"));
    Serial.println(F("  c1:1001             - 播放通道1音频1001"));
    Serial.println(F("  playall             - 播放所有通道"));
    Serial.println(F("  stopall             - 停止所有通道"));
    Serial.println(F(""));
    Serial.println(F("测试命令:"));
    Serial.println(F("  test1     - 测试通道1"));
    Serial.println(F("  testall   - 测试所有通道"));
    Serial.println(F(""));
    Serial.println(F("注意: C101使用IO控制，不支持音量调节和选歌功能"));
} 