#include "ArduinoSystemHelper.h"
#include <SPI.h>

// 调试开关
#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

// 全局实例
ArduinoSystemHelper systemHelper;

// ========================== 构造和析构 ==========================
ArduinoSystemHelper::ArduinoSystemHelper() {
    lastNetworkCheck = 0;
    lastConnectionState = false;
    devices = nullptr;
    deviceCount = 0;
    connectionCallback = nullptr;
    messageCallback = nullptr;
}

ArduinoSystemHelper::~ArduinoSystemHelper() {
    if (devices) {
        delete[] devices;
    }
}

// ========================== 初始化 ==========================
void ArduinoSystemHelper::begin(const String& ctrlId, uint8_t devCount) {
    controllerId = ctrlId;
    deviceCount = devCount;
    
    if (devices) {
        delete[] devices;
    }
    devices = new DeviceConfig[deviceCount];
}

void ArduinoSystemHelper::setupDevice(uint8_t index, uint8_t pin, uint8_t type, const String& id) {
    if (index < deviceCount) {
        devices[index].pin = pin;
        devices[index].type = type;
        devices[index].id = id;
    }
}

// ========================== 网络初始化 ==========================
bool ArduinoSystemHelper::initNetwork(IPAddress serverIP, uint16_t serverPort) {
    #ifdef DEBUG
    Serial.println(F("初始化网络"));
    #endif
    
    SPI.begin();
    
    // 从控制器ID生成MAC地址
    int controllerNum = controllerId.substring(1).toInt();
    byte mac[] = {
        0xDE, 0xAD, 0xBE, 0xEF, 
        (byte)((controllerNum >> 8) & 0xFF), 
        (byte)(controllerNum & 0xFF)
    };
    
    // 计算IP地址
    int lastOctet = 100 + (controllerNum % 150);
    IPAddress ip(192, 168, 10, lastOctet);
    IPAddress gateway(192, 168, 10, 1);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns(192, 168, 10, 1);
    
    // 初始化W5100
    Ethernet.init(10);
    delay(50);    // 🚀 减少到50ms，足够W5100初始化
    
    Ethernet.begin(mac, ip, dns, gateway, subnet);
    delay(1000);   // 🚀 减少到500ms，大多数情况下足够
    
    if (Ethernet.localIP() == IPAddress(0, 0, 0, 0)) {
        return false;
    }
    
    // 初始化HarbingerClient
    if (harbingerClient.begin(controllerId, "Arduino")) {
        harbingerClient.setConnectionCallback(connectionCallback);
        harbingerClient.setMessageCallback(messageCallback);
        harbingerClient.connect(serverIP, serverPort);
        return true;
    }
    
    return false;
}

// ========================== 硬件控制 ==========================
void ArduinoSystemHelper::initializeHardware() {
    for (uint8_t i = 0; i < deviceCount; i++) {
        if (devices[i].type == 0) { // PWM
            pinMode(devices[i].pin, OUTPUT);
            analogWrite(devices[i].pin, 0);
        } else if (devices[i].type == 1) { // DIGITAL OUTPUT
            pinMode(devices[i].pin, OUTPUT);
            digitalWrite(devices[i].pin, LOW);
        } else if (devices[i].type == 2) { // INPUT
            pinMode(devices[i].pin, INPUT_PULLUP);
        }
    }
}

void ArduinoSystemHelper::stopAllDevices() {
    for (uint8_t i = 0; i < deviceCount; i++) {
        if (devices[i].type == 0) { // PWM
            analogWrite(devices[i].pin, 0);
        } else if (devices[i].type == 1) { // DIGITAL
            digitalWrite(devices[i].pin, LOW);
        }
    }
}

void ArduinoSystemHelper::testDevices() {
    #ifdef DEBUG
    Serial.println(F("测试设备"));
    #endif
    
    for (uint8_t i = 0; i < deviceCount; i++) {
        if (devices[i].type == 0) { // PWM
            analogWrite(devices[i].pin, 128);
            delay(1000);
            analogWrite(devices[i].pin, 0);
        } else if (devices[i].type == 1) { // DIGITAL
            digitalWrite(devices[i].pin, HIGH);
            delay(1000);
            digitalWrite(devices[i].pin, LOW);
        }
        delay(500);
    }
}

// ========================== 网络健康检查 ==========================
void ArduinoSystemHelper::checkNetworkHealth() {
    if (millis() - lastNetworkCheck < 2000) {
        return;
    }
    lastNetworkCheck = millis();
    
    bool clientConnected = harbingerClient.isConnected();
    
    if (clientConnected != lastConnectionState) {
        if (clientConnected) {
            #ifdef DEBUG
            Serial.println(F("网络已恢复"));
            #endif
        } else {
            #ifdef DEBUG
            Serial.println(F("网络断开"));
            #endif
        }
        lastConnectionState = clientConnected;
    }
}

// ========================== 串口命令处理 ==========================
void ArduinoSystemHelper::handleSerialCommands() {
    if (!Serial.available()) return;
    
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "status") {
        printStatus();
    } else if (command == "test") {
        testDevices();
    } else if (command == "stop") {
        stopAllDevices();
    } else if (command == "network") {
        printNetworkDiagnostics();
    } else if (command == "debug") {
        printConnectionDebug();
    } else if (command == "reconnect") {
        reconnect();
    } else if (command == "reset") {
        resetNetwork();
    }
}

// ========================== 调试功能 ==========================
void ArduinoSystemHelper::printStatus() {
    Serial.print(F("ID: "));
    Serial.println(controllerId);
    Serial.print(F("网络: "));
    Serial.println(harbingerClient.isConnected() ? F("ON") : F("OFF"));
    Serial.print(F("内存: "));
    Serial.println(freeMemory());
}

void ArduinoSystemHelper::printNetworkDiagnostics() {
    Serial.println(F("=== 网络诊断 ==="));
    Serial.print(F("本地IP: "));
    Serial.println(Ethernet.localIP());
    Serial.print(F("链路状态: "));
    switch (Ethernet.linkStatus()) {
        case Unknown: Serial.println(F("未知")); break;
        case LinkON: Serial.println(F("已连接")); break;
        case LinkOFF: Serial.println(F("未连接")); break;
    }
    Serial.print(F("连接: "));
    Serial.println(harbingerClient.isConnected() ? F("已连接") : F("未连接"));
    Serial.println(F("=== 诊断完成 ==="));
}

void ArduinoSystemHelper::printConnectionDebug() {
    Serial.println(F("=== 连接调试 ==="));
    Serial.print(F("连接状态: "));
    Serial.println(harbingerClient.isConnected() ? F("已连接") : F("未连接"));
    Serial.print(F("运行时间: "));
    Serial.print(millis() / 1000);
    Serial.println(F("秒"));
    Serial.println(F("=== 调试完成 ==="));
}

// ========================== 设置回调 ==========================
void ArduinoSystemHelper::setConnectionCallback(void (*callback)(bool)) {
    connectionCallback = callback;
}

void ArduinoSystemHelper::setMessageCallback(void (*callback)(String)) {
    messageCallback = callback;
}

// ========================== 内存检测 ==========================
int ArduinoSystemHelper::freeMemory() {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// ========================== 手动网络控制 ==========================
void ArduinoSystemHelper::reconnect() {
    #ifdef DEBUG
    Serial.println(F("手动重连"));
    #endif
    
    harbingerClient.disconnect();
    delay(1000);
    
    IPAddress serverIP(192, 168, 10, 10);
    harbingerClient.connect(serverIP, 9000);
}

void ArduinoSystemHelper::resetNetwork() {
    Serial.println(F("重置网络"));
    reinitializeNetwork();
}

void ArduinoSystemHelper::reinitializeNetwork() {
    harbingerClient.disconnect();
    delay(1000);
    
    IPAddress serverIP(192, 168, 10, 10);
    initNetwork(serverIP, 9000);
} 