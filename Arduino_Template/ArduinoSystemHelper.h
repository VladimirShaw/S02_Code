#ifndef ARDUINO_SYSTEM_HELPER_H
#define ARDUINO_SYSTEM_HELPER_H

#include <Arduino.h>
#include <Ethernet.h>
#include "UniversalHarbingerClient.h"
#include "UniversalGameProtocol.h"

// ========================== 系统辅助类 ==========================
class ArduinoSystemHelper {
private:
    // 网络相关
    unsigned long lastNetworkCheck;
    bool lastConnectionState;
    
    // 硬件配置
    struct DeviceConfig {
        uint8_t pin;
        uint8_t type;  // 0=PWM, 1=DIGITAL, 2=INPUT
        String id;
    };
    
    DeviceConfig* devices;
    uint8_t deviceCount;
    String controllerId;
    
    // 回调函数
    void (*connectionCallback)(bool);
    void (*messageCallback)(String);

public:
    ArduinoSystemHelper();
    ~ArduinoSystemHelper();
    
    // 初始化函数
    void begin(const String& ctrlId, uint8_t devCount);
    void setupDevice(uint8_t index, uint8_t pin, uint8_t type, const String& id);
    
    // 网络初始化（简化版）
    bool initNetwork(IPAddress serverIP, uint16_t serverPort);
    
    // 硬件控制
    void initializeHardware();
    void stopAllDevices();
    void testDevices();
    
    // 网络健康检查
    void checkNetworkHealth();
    
    // 串口命令处理
    void handleSerialCommands();
    
    // 调试功能
    void printStatus();
    void printNetworkDiagnostics();
    void printConnectionDebug();
    
    // 设置回调
    void setConnectionCallback(void (*callback)(bool));
    void setMessageCallback(void (*callback)(String));
    
    // 内存检测
    static int freeMemory();
    
    // 手动网络控制
    void reconnect();
    void resetNetwork();
    
private:
    void reinitializeNetwork();
};

// 全局实例
extern ArduinoSystemHelper systemHelper;

#endif // ARDUINO_SYSTEM_HELPER_H 