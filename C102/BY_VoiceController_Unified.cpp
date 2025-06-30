/**
 * =============================================================================
 * BY_VoiceController_Unified - 统一的4路BY语音模块控制器 - 实现文件
 * 版本: v3.0
 * 创建日期: 2025-01-03
 * =============================================================================
 */

#include "BY_VoiceController_Unified.h"

// ========================== BY_VoiceModule_Unified 实现 ==========================

BY_VoiceModule_Unified::BY_VoiceModule_Unified() {
    serialPort = nullptr;
}

void BY_VoiceModule_Unified::init(Stream* serial) {
    serialPort = serial;
}

// 按照BY_Demo验证过的CRC计算方式 (XOR)
byte BY_VoiceModule_Unified::calculateCRC(byte* p, byte cNum) {
    byte CRC_Result = 0;
    for (char i = 0; i < cNum; i++) {
        CRC_Result ^= *p;
        p++;
    }
    return CRC_Result;
}

// 按照BY_Demo验证过的帧发送方式
void BY_VoiceModule_Unified::sendFrameData(byte* pData) {
    char i;
    unsigned char datLen = pData[0];
    
    // 构建完整帧
    for (i = 0; i < datLen; i++) {
        sendBuffer[1 + i] = *pData;
        pData++;
    }
    sendBuffer[datLen + 1] = 0xEF;  // 结束符
    
    // 发送数据
    if (serialPort != nullptr) {
        serialPort->write(sendBuffer, datLen + 2);
    }
}

// 基本命令发送
void BY_VoiceModule_Unified::sendCommand(byte cmdType) {
    unsigned char datLen = 3; // 数据长度
    byte pDat[datLen];
    pDat[0] = datLen;
    pDat[1] = cmdType;
    pDat[2] = calculateCRC(pDat, datLen - 1);
    sendFrameData(pDat);
}

// 参数命令发送
void BY_VoiceModule_Unified::sendCommand(byte cmdType, byte* param) {
    unsigned char datLen = param[0]; // 数据长度
    byte pDat[datLen];
    pDat[0] = datLen;
    pDat[1] = cmdType;
    for (char i = 2; i < datLen - 1; i++) {
        pDat[i] = param[i - 1];
    }
    pDat[datLen - 1] = calculateCRC(pDat, datLen - 1);
    sendFrameData(pDat);
}

// ========================== 基本控制方法 ==========================

void BY_VoiceModule_Unified::play() {
    sendCommand(BY_Commands::CMD_PLAY);
}

void BY_VoiceModule_Unified::pause() {
    sendCommand(BY_Commands::CMD_PAUSE);
}

void BY_VoiceModule_Unified::stop() {
    sendCommand(BY_Commands::CMD_STOP);
}

void BY_VoiceModule_Unified::nextSong() {
    sendCommand(BY_Commands::CMD_NEXT);
}

void BY_VoiceModule_Unified::prevSong() {
    sendCommand(BY_Commands::CMD_PREV);
}

void BY_VoiceModule_Unified::reset() {
    sendCommand(BY_Commands::CMD_RESET);
}

void BY_VoiceModule_Unified::fastForward() {
    sendCommand(BY_Commands::CMD_FFOW);
}

void BY_VoiceModule_Unified::fastBackward() {
    sendCommand(BY_Commands::CMD_FBCK);
}

// ========================== 高级控制方法 ==========================

void BY_VoiceModule_Unified::setVolume(byte volume) {
    if (volume > 30) volume = 30;
    byte param[2];
    param[0] = 4; // 数据长度
    param[1] = volume;
    sendCommand(BY_Commands::SET_VOL, param);
}

void BY_VoiceModule_Unified::setEQ(byte eq) {
    if (eq > 5) eq = 5;
    byte param[2];
    param[0] = 4; // 数据长度
    param[1] = eq;
    sendCommand(BY_Commands::SET_EQ, param);
}

void BY_VoiceModule_Unified::setCycle(byte cycle) {
    if (cycle > 4) cycle = 4;
    byte param[2];
    param[0] = 4; // 数据长度
    param[1] = cycle;
    sendCommand(BY_Commands::SET_CYCLE, param);
}

void BY_VoiceModule_Unified::selectSong(int songID) {
    if (songID < 1 || songID > 9999) return;
    
    byte param[3];
    param[0] = 5; // 数据长度
    param[1] = songID >> 8;
    param[2] = songID;
    sendCommand(BY_Commands::SEL_SONG, param);
}

void BY_VoiceModule_Unified::selectFolderSong(byte folderID, byte songID) {
    byte param[3];
    param[0] = 5; // 数据长度
    param[1] = folderID;
    param[2] = songID;
    sendCommand(BY_Commands::SEL_FdSong, param);
}

void BY_VoiceModule_Unified::playSong(int songID) {
    selectSong(songID);
    delay(100);  // 等待选择完成
    play();
}

// ========================== BY_VoiceController_Unified 实现 ==========================

BY_VoiceController_Unified::BY_VoiceController_Unified() {
    softSerial = nullptr;
    initialized = false;
    lastStatusCheck = 0;
    
    // 默认引脚配置
    softRX = 2; softTX = 3;
    busyPins[0] = 22; busyPins[1] = 23; busyPins[2] = 24; busyPins[3] = 25;
    
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
    Serial.print(F("🔧 设置软串口引脚: RX="));
    Serial.print(rxPin);
    Serial.print(F(", TX="));
    Serial.println(txPin);
}

void BY_VoiceController_Unified::setBusyPin(int channel, int pin) {
    if (channel >= 1 && channel <= 4) {
        busyPins[channel - 1] = pin;
        Serial.print(F("🔧 设置通道"));
        Serial.print(channel);
        Serial.print(F(" Busy引脚: "));
        Serial.println(pin);
    }
}

void BY_VoiceController_Unified::setBusyPins(int pin1, int pin2, int pin3, int pin4) {
    busyPins[0] = pin1;
    busyPins[1] = pin2;
    busyPins[2] = pin3;
    busyPins[3] = pin4;
    Serial.print(F("🔧 设置所有Busy引脚: "));
    Serial.print(pin1); Serial.print(F(", "));
    Serial.print(pin2); Serial.print(F(", "));
    Serial.print(pin3); Serial.print(F(", "));
    Serial.println(pin4);
}

bool BY_VoiceController_Unified::begin() {
    Serial.println(F("🚀 初始化统一语音控制器..."));
    
    // 创建软串口 (按照BY_Demo验证过的配置)
    if (softSerial != nullptr) {
        delete softSerial;
    }
    softSerial = new SoftwareSerial(softRX, softTX);
    
    // 初始化串口
    Serial.println(F("🔗 初始化串口:"));
    Serial1.begin(9600);
    Serial.println(F("  ✅ Serial1 (通道1)"));
    Serial2.begin(9600);
    Serial.println(F("  ✅ Serial2 (通道2)"));
    Serial3.begin(9600);
    Serial.println(F("  ✅ Serial3 (通道3)"));
    softSerial->begin(9600);
    Serial.print(F("  ✅ SoftwareSerial (通道4) RX="));
    Serial.print(softRX);
    Serial.print(F(", TX="));
    Serial.println(softTX);
    
    // 初始化语音模块
    Serial.println(F("🎵 初始化语音模块:"));
    modules[0].init(&Serial1);
    Serial.println(F("  ✅ 通道1 → Serial1"));
    modules[1].init(&Serial2);
    Serial.println(F("  ✅ 通道2 → Serial2"));
    modules[2].init(&Serial3);
    Serial.println(F("  ✅ 通道3 → Serial3"));
    modules[3].init(softSerial);
    Serial.println(F("  ✅ 通道4 → SoftwareSerial"));
    
    // 初始化Busy引脚
    Serial.println(F("📍 初始化Busy引脚:"));
    for (int i = 0; i < 4; i++) {
        pinMode(busyPins[i], INPUT_PULLUP);
        Serial.print(F("  ✅ 通道"));
        Serial.print(i + 1);
        Serial.print(F(" → Pin"));
        Serial.println(busyPins[i]);
    }
    
    // 重置所有模块
    Serial.println(F("🔄 重置所有语音模块..."));
    for (int i = 0; i < 4; i++) {
        modules[i].reset();
        delay(500);
    }
    
    initialized = true;
    Serial.println(F("✅ 统一语音控制器初始化完成"));
    
    return true;
}

// ========================== 语音控制接口 ==========================

void BY_VoiceController_Unified::play(int channel) {
    if (channel < 1 || channel > 4 || !initialized) return;
    modules[channel - 1].play();
}

void BY_VoiceController_Unified::stop(int channel) {
    if (channel < 1 || channel > 4 || !initialized) return;
    modules[channel - 1].stop();
}

void BY_VoiceController_Unified::pause(int channel) {
    if (channel < 1 || channel > 4 || !initialized) return;
    modules[channel - 1].pause();
}

void BY_VoiceController_Unified::nextSong(int channel) {
    if (channel < 1 || channel > 4 || !initialized) return;
    modules[channel - 1].nextSong();
}

void BY_VoiceController_Unified::prevSong(int channel) {
    if (channel < 1 || channel > 4 || !initialized) return;
    modules[channel - 1].prevSong();
}

void BY_VoiceController_Unified::setVolume(int channel, int volume) {
    if (channel < 1 || channel > 4 || !initialized) return;
    modules[channel - 1].setVolume((byte)volume);
}

void BY_VoiceController_Unified::playSong(int channel, int songID) {
    if (channel < 1 || channel > 4 || !initialized) return;
    modules[channel - 1].playSong(songID);
}

// ========================== 批量控制 ==========================

void BY_VoiceController_Unified::playAll() {
    for (int i = 1; i <= 4; i++) {
        play(i);
        delay(50);
    }
}

void BY_VoiceController_Unified::stopAll() {
    for (int i = 1; i <= 4; i++) {
        stop(i);
        delay(50);
    }
}

void BY_VoiceController_Unified::setVolumeAll(int volume) {
    for (int i = 1; i <= 4; i++) {
        setVolume(i, volume);
        delay(50);
    }
}

// ========================== 状态查询 ==========================

bool BY_VoiceController_Unified::isBusy(int channel) {
    if (channel < 1 || channel > 4 || !initialized) return false;
    return digitalRead(busyPins[channel - 1]) == HIGH;  // 先改回HIGH，测试确认逻辑
}

void BY_VoiceController_Unified::update() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    if (currentTime - lastStatusCheck >= STATUS_CHECK_INTERVAL) {
        lastStatusCheck = currentTime;
        
        // 检查所有通道的Busy状态变化
        for (int i = 0; i < 4; i++) {
            busyStates[i] = digitalRead(busyPins[i]) == HIGH;  // 先改回HIGH，测试确认逻辑
            
            if (busyStates[i] != lastBusyStates[i]) {
                Serial.print(F("通道"));
                Serial.print(i + 1);
                Serial.print(F(" 状态: "));
                Serial.println(busyStates[i] ? F("播放中") : F("空闲"));
                lastBusyStates[i] = busyStates[i];
            }
        }
    }
}

void BY_VoiceController_Unified::printStatus() {
    if (!initialized) {
        Serial.println(F("❌ 控制器未初始化"));
        return;
    }
    
    Serial.println(F("\n===== 统一语音控制器状态 ====="));
    
    for (int i = 0; i < 4; i++) {
        Serial.print(F("通道"));
        Serial.print(i + 1);
        Serial.println(F(":"));
        
        // Busy状态
        bool busy = digitalRead(busyPins[i]) == HIGH;  // 先改回HIGH，测试确认逻辑
        Serial.print(F("  播放状态: "));
        Serial.println(busy ? F("播放中") : F("空闲"));
        Serial.print(F("  Busy引脚: "));
        Serial.println(busyPins[i]);
        
        // 串口信息
        Serial.print(F("  串口: "));
        if (i == 0) Serial.println(F("Serial1"));
        else if (i == 1) Serial.println(F("Serial2"));
        else if (i == 2) Serial.println(F("Serial3"));
        else Serial.println(F("SoftwareSerial"));
    }
    
    Serial.print(F("软串口配置: RX="));
    Serial.print(softRX);
    Serial.print(F(", TX="));
    Serial.println(softTX);
    Serial.println(F("==============================\n"));
}

// ========================== 命令处理 ==========================

void BY_VoiceController_Unified::processSerialCommand(String command) {
    command.trim();
    
    Serial.print(F("🎵 统一控制器处理命令: '"));
    Serial.print(command);
    Serial.println(F("'"));
    
    if (command.length() == 0) return;
    
    // 检查初始化状态
    if (!initialized) {
        Serial.println(F("❌ 控制器未初始化！"));
        return;
    }
    
    // 帮助命令
    if (command == "help" || command == "h") {
        printHelp();
        return;
    }
    
    // 状态命令
    if (command == "status" || command == "s") {
        printStatus();
        return;
    }
    
    // 批量控制命令
    if (command == "stopall") {
        stopAll();
        Serial.println(F("🛑 所有通道已停止"));
        return;
    }
    
    if (command == "playall") {
        playAll();
        Serial.println(F("▶️ 所有通道开始播放"));
        return;
    }
    
    if (command.startsWith("volall:")) {
        int vol = command.substring(7).toInt();
        if (vol >= 0 && vol <= 30) {
            setVolumeAll(vol);
            Serial.print(F("🔊 所有通道音量设置为: "));
            Serial.println(vol);
        } else {
            Serial.println(F("❌ 音量范围应为0-30"));
        }
        return;
    }
    
    // 测试命令
    if (command == "test1") {
        playSong(1, 1);
        Serial.println(F("🎵 通道1播放测试音频1"));
        return;
    }
    
    if (command == "test201") {
        playSong(1, 201);
        Serial.println(F("🎵 通道1播放测试音频201"));
        return;
    }
    
    if (command == "testall") {
        playSong(1, 1);
        delay(100);
        playSong(2, 2);
        delay(100);
        playSong(3, 3);
        delay(100);
        playSong(4, 4);
        Serial.println(F("🎵 所有通道播放测试音频 (1,2,3,4)"));
        return;
    }
    
    // 通道命令 c[1-4][操作]
    if (command.length() >= 2 && command[0] == 'c' && 
        command[1] >= '1' && command[1] <= '4') {
        int channel = command[1] - '0';
        String operation = command.substring(2);
        
        Serial.print(F("📻 通道"));
        Serial.print(channel);
        Serial.print(F(" 操作: '"));
        Serial.print(operation);
        Serial.println(F("'"));
        
        if (operation == "p") {
            play(channel);
            Serial.print(F("通道")); Serial.print(channel); Serial.println(F(": 播放"));
        } else if (operation == "s") {
            stop(channel);
            Serial.print(F("通道")); Serial.print(channel); Serial.println(F(": 停止"));
        } else if (operation == "n") {
            nextSong(channel);
            Serial.print(F("通道")); Serial.print(channel); Serial.println(F(": 下一首"));
        } else if (operation == "r") {
            prevSong(channel);
            Serial.print(F("通道")); Serial.print(channel); Serial.println(F(": 上一首"));
        } else if (operation.startsWith("v")) {
            int vol = operation.substring(1).toInt();
            if (vol >= 0 && vol <= 30) {
                setVolume(channel, vol);
                Serial.print(F("通道")); Serial.print(channel); 
                Serial.print(F(": 音量=")); Serial.println(vol);
            } else {
                Serial.println(F("❌ 音量范围应为0-30"));
            }
        } else if (operation.startsWith(":")) {
            int songID = operation.substring(1).toInt();
            if (songID >= 1 && songID <= 9999) {
                playSong(channel, songID);
                Serial.print(F("通道")); Serial.print(channel); 
                Serial.print(F(": 播放歌曲")); Serial.println(songID);
            } else {
                Serial.println(F("❌ 歌曲序号范围应为1-9999"));
            }
        } else {
            Serial.println(F("❌ 未知的通道操作命令"));
        }
    } else {
        Serial.println(F("❌ 未知命令格式"));
    }
}

void BY_VoiceController_Unified::printHelp() {
    Serial.println(F("\n=============== C102 4路语音控制器命令帮助 ==============="));
    Serial.println(F("📻 单通道控制命令:"));
    Serial.println(F("  c1p - c4p        : 通道1-4播放"));
    Serial.println(F("  c1s - c4s        : 通道1-4停止"));
    Serial.println(F("  c1v[0-30]        : 设置音量 (例: c1v15, c2v20)"));
    Serial.println(F("  c1:[1-9999]      : 播放指定歌曲 (例: c1:201, c2:1234)"));
    Serial.println(F("  c1n - c4n        : 通道1-4下一首"));
    Serial.println(F("  c1r - c4r        : 通道1-4上一首"));
    Serial.println(F(""));
    Serial.println(F("🎵 批量控制命令:"));
    Serial.println(F("  stopall          : 停止所有通道"));
    Serial.println(F("  playall          : 播放所有通道"));
    Serial.println(F("  volall:[0-30]    : 设置所有通道音量 (例: volall:15)"));
    Serial.println(F(""));
    Serial.println(F("🎯 常用音频测试:"));
    Serial.println(F("  test1            : 通道1播放音频1"));
    Serial.println(F("  test201          : 通道1播放音频201"));
    Serial.println(F("  testall          : 所有通道播放测试音频"));
    Serial.println(F(""));
    Serial.println(F("📊 系统命令:"));
    Serial.println(F("  status 或 s      : 显示系统状态"));
    Serial.println(F("  help 或 h        : 显示此帮助"));
    Serial.println(F("========================================================\n"));
} 