/**
 * =============================================================================
 * BY_VoiceController_Unified - 统一的4路BY语音模块控制器
 * 版本: v3.0
 * 创建日期: 2025-01-03
 * 
 * 功能:
 * - 支持4路语音模块控制 (3路硬件串口 + 1路软串口)
 * - 灵活的引脚配置 (分别设置TX/RX和Busy引脚)
 * - 统一的API接口
 * - 完整的状态监控
 * =============================================================================
 */

#ifndef BY_VOICECONTROLLER_UNIFIED_H
#define BY_VOICECONTROLLER_UNIFIED_H

#include <Arduino.h>
#include <SoftwareSerial.h>

// ========================== BY命令定义 ==========================
struct BY_Commands {
    // 基本指令
    static const byte CMD_SOF = 0x7E;
    static const byte CMD_EOF = 0XEF;
    static const byte CMD_PLAY = 0X01;
    static const byte CMD_PAUSE = 0x02;
    static const byte CMD_NEXT = 0x03;
    static const byte CMD_PREV = 0x04;
    static const byte CMD_VolUP = 0x05;
    static const byte CMD_VolDOWN = 0x06;
    static const byte CMD_RESET = 0x09;
    static const byte CMD_FFOW = 0x0A;
    static const byte CMD_FBCK = 0x0B;
    static const byte CMD_STOP = 0x0E;
    
    // 3byte参数命令
    static const byte SET_VOL = 0x31;      // 0-30
    static const byte SET_EQ = 0x32;       // 0-5
    static const byte SET_CYCLE = 0x33;    // 0-4
    static const byte SET_Folder = 0x34;
    static const byte SET_Device = 0x35;
    static const byte CMD_Baud = 0x36;
    
    // 4字节命令
    static const byte SEL_SONG = 0x41;
    static const byte SEL_FdSong = 0x42;
    static const byte IST_SONG = 0x43;
    static const byte IST_FdSong = 0x44;
};

// ========================== 单个语音模块类 ==========================
class BY_VoiceModule_Unified {
private:
    Stream* serialPort;
    byte sendBuffer[8] = {0x7E}; // 发送缓冲区
    
    // CRC计算 (XOR方式)
    byte calculateCRC(byte* p, byte cNum);
    
    // 帧发送
    void sendFrameData(byte* pData);
    
    // 命令发送
    void sendCommand(byte cmdType);
    void sendCommand(byte cmdType, byte* param);

public:
    BY_VoiceModule_Unified();
    void init(Stream* serial);
    
    // 基本控制方法
    void play();
    void pause();
    void stop();
    void nextSong();
    void prevSong();
    void reset();
    void fastForward();
    void fastBackward();
    
    // 高级控制方法
    void setVolume(byte volume);    // 0-30
    void setEQ(byte eq);           // 0-5
    void setCycle(byte cycle);     // 0-4
    void selectSong(int songID);   // 1-9999
    void selectFolderSong(byte folderID, byte songID);
    void playSong(int songID);     // 选择并播放
};

// ========================== 统一语音控制器类 ==========================
class BY_VoiceController_Unified {
private:
    BY_VoiceModule_Unified modules[4];    // 4个语音模块
    SoftwareSerial* softSerial;           // 软串口指针
    bool initialized;
    
    // 配置参数
    int softRX, softTX;                   // 软串口引脚
    int busyPins[4];                      // Busy引脚数组
    
    // 状态监控
    bool busyStates[4];
    bool lastBusyStates[4];
    unsigned long lastStatusCheck;
    static const unsigned long STATUS_CHECK_INTERVAL = 100;

public:
    BY_VoiceController_Unified();
    ~BY_VoiceController_Unified();
    
    // ========================== 灵活配置接口 ==========================
    
    // 设置软串口引脚 (通道4使用)
    void setSoftSerialPins(int rxPin, int txPin);
    
    // 设置单个Busy引脚
    void setBusyPin(int channel, int pin);
    
    // 批量设置所有Busy引脚
    void setBusyPins(int pin1, int pin2, int pin3, int pin4);
    
    // 初始化系统 (在设置完引脚后调用)
    bool begin();
    
    // ========================== 语音控制接口 ==========================
    
    // 单通道控制
    void play(int channel);
    void stop(int channel);
    void pause(int channel);
    void nextSong(int channel);
    void prevSong(int channel);
    void setVolume(int channel, int volume);
    void playSong(int channel, int songID);
    
    // 批量控制
    void playAll();
    void stopAll();
    void setVolumeAll(int volume);
    
    // ========================== 状态查询接口 ==========================
    
    bool isBusy(int channel);
    void update();              // 状态更新 (在loop中调用)
    void printStatus();         // 打印状态信息
    
    // ========================== C101 IO控制接口 ==========================
    
    void reset();                           // 重置所有音频通道
    void playIOAudio(int channel);          // 播放指定通道音频
    void stopIOAudio(int channel);          // 停止指定通道音频
    void resetIOAudio(int channel);         // 重置指定通道到默认状态
    void resetAllIOAudio();                 // 重置所有通道到默认状态
    void sendCommand(byte cmdType);         // 发送基本命令（兼容接口）
    
    // ========================== 命令处理接口 ==========================
    
    void processSerialCommand(String command);
    void printHelp();
    
    // ========================== 配置查询接口 ==========================
    
    bool isInitialized() { return initialized; }
    int getSoftRX() { return softRX; }
    int getSoftTX() { return softTX; }
    int getBusyPin(int channel) { return (channel >= 1 && channel <= 4) ? busyPins[channel-1] : -1; }
};

#endif // BY_VOICECONTROLLER_UNIFIED_H 