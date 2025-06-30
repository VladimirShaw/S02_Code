/**
 * =============================================================================
 * 通用Harbinger客户端实现 - UniversalHarbingerClient.cpp
 * 创建日期: 2025-01-03
 * 描述信息: 通用的Harbinger协议客户端实现
 * =============================================================================
 */

#include "UniversalHarbingerClient.h"

// ========================== 调试开关 ==========================
// 注释掉这行可以关闭所有调试信息
#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

// ========================== 全局实例 ==========================
UniversalHarbingerClient harbingerClient;

// ========================== 构造函数和析构函数 ==========================
UniversalHarbingerClient::UniversalHarbingerClient() {
    connectionState = CONN_DISCONNECTED;
    serverPort = 0;
    lastHeartbeat = 0;
    lastReconnectAttempt = 0;
    ethernetInitTime = 0;
    networkInitialized = false;
    firstConnectionAttempted = false;
    connectionCallback = nullptr;
    messageCallback = nullptr;
}

UniversalHarbingerClient::~UniversalHarbingerClient() {
    disconnect();
}

// ========================== 初始化和连接 ==========================
bool UniversalHarbingerClient::begin(const String& controllerId, const String& deviceType) {
    this->controllerId = controllerId;
    this->deviceType = deviceType;
    connectionState = CONN_INITIALIZING;
    
    // 根据控制器ID生成唯一MAC地址
    String idStr = controllerId;
    int controllerNum = idStr.substring(1).toInt(); // C303 -> 303
    byte mac[] = {
        0xDE, 0xAD, 0xBE, 0xEF, 
        (byte)((controllerNum >> 8) & 0xFF), 
        (byte)(controllerNum & 0xFF)
    };
    
    // 计算IP地址
    int lastOctet = 100 + (controllerNum % 150); // 防止超出255
    IPAddress clientIP(192, 168, 10, lastOctet);
    IPAddress gateway(192, 168, 10, 1);
    IPAddress subnet(255, 255, 255, 0);
    IPAddress dns(192, 168, 10, 1);
    
    // 初始化以太网
    Ethernet.begin(mac, clientIP, dns, gateway, subnet);
    
    // 检查以太网硬件
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        connectionState = CONN_ERROR;
        return false;
    }
    
    // 等待网络稳定
    delay(ETHERNET_STABILIZE_TIME);
    
    // 检查网络状态
    IPAddress localIP = Ethernet.localIP();
    
    if (Ethernet.linkStatus() != LinkOFF && localIP != IPAddress(0, 0, 0, 0)) {
        networkInitialized = true;
        connectionState = CONN_STABILIZING;
        ethernetInitTime = millis();
        return true;
    } else {
        connectionState = CONN_ERROR;
        return false;
    }
}

bool UniversalHarbingerClient::connect(IPAddress serverIP, uint16_t serverPort) {
    if (!networkInitialized) return false;
    
    this->serverIP = serverIP;
    this->serverPort = serverPort;
    
    return true;
}

bool UniversalHarbingerClient::connectToServer() {
    if (!networkInitialized) return false;
    
    // 检查是否需要等待
    if (lastReconnectAttempt != 0 && millis() - lastReconnectAttempt < RECONNECT_INTERVAL) {
        return false;
    }
    lastReconnectAttempt = millis();
    
    // 强制断开现有连接
    if (client.connected()) {
        DEBUG_PRINTLN(F("断开现有连接"));
        client.stop();
        delay(100);
    }
    
    // 确保socket完全关闭
    client.flush();
    delay(100);
    
    DEBUG_PRINT(F("尝试连接到 "));
    DEBUG_PRINT(serverIP);
    DEBUG_PRINT(F(":"));
    DEBUG_PRINTLN(serverPort);
    
    // 尝试连接（设置较短的超时）
    client.setTimeout(5000);  // 5秒超时
    
    if (client.connect(serverIP, serverPort)) {
        // 验证连接是否真正建立
        delay(100);
        if (client.connected()) {
            connectionState = CONN_CONNECTED;
            
            DEBUG_PRINTLN(F("连接成功！"));
            
            // 立即发送注册消息
            sendRegistration();
            
            // 重置心跳计时器
            lastHeartbeat = millis();
            
            // 触发连接回调
            if (connectionCallback) {
                connectionCallback(true);
            }
            
            return true;
        } else {
            DEBUG_PRINTLN(F("连接验证失败"));
            connectionState = CONN_CONNECTING;  // 保持尝试状态
            return false;
        }
    } else {
        DEBUG_PRINTLN(F("连接失败，将在3秒后重试"));
        connectionState = CONN_CONNECTING;  // 保持尝试状态而不是DISCONNECTED
        return false;
    }
}

void UniversalHarbingerClient::disconnect() {
    DEBUG_PRINTLN(F("断开连接"));
    
    if (client.connected()) {
        client.stop();
    }
    
    // 强制清理socket
    client.flush();
    delay(100);
    
    connectionState = CONN_DISCONNECTED;
    
    if (connectionCallback) {
        connectionCallback(false);
    }
}

// ========================== 状态查询 ==========================
bool UniversalHarbingerClient::isConnected() const {
    // 首先检查内部状态
    if (connectionState < CONN_CONNECTED) {
        return false;
    }
    
    // 然后检查socket状态
    bool socketConnected = client.connected();
    
    // 如果socket报告未连接，立即更新状态
    if (!socketConnected && connectionState >= CONN_CONNECTED) {
        // 注意：这里不能直接修改状态，因为是const方法
        // 但可以返回false，让调用者知道连接已断开
        return false;
    }
    
    return socketConnected;
}

ConnectionState UniversalHarbingerClient::getConnectionState() const {
    return connectionState;
}

String UniversalHarbingerClient::getLocalIP() const {
    IPAddress ip = Ethernet.localIP();
    return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

String UniversalHarbingerClient::getServerInfo() const {
    String info = "服务器: " + String(serverIP[0]) + "." + String(serverIP[1]) + "." + String(serverIP[2]) + "." + String(serverIP[3]) + ":" + String(serverPort) + "\n";
    info += "状态: " + String(client.connected() ? "已连接" : "未连接");
    return info;
}

// ========================== 回调设置 ==========================
void UniversalHarbingerClient::setConnectionCallback(ConnectionChangeCallback callback) {
    this->connectionCallback = callback;
}

void UniversalHarbingerClient::setMessageCallback(MessageReceivedCallback callback) {
    this->messageCallback = callback;
}

// ========================== 消息处理 ==========================
void UniversalHarbingerClient::sendRegistration() {
    String deviceList = buildDeviceList();
    String msg = "$[INFO]@" + controllerId + "{^REGISTER^(type=" + deviceType + ",devices=" + 
                deviceList + ",version=2.0,client_id=" + controllerId + ")}#";
    
    DEBUG_PRINT(F("发送: "));
    DEBUG_PRINTLN(msg);
    
    client.print(msg);
}

void UniversalHarbingerClient::sendHeartbeat() {
    if (!isConnected()) return;
    
    if (millis() - lastHeartbeat < HEARTBEAT_INTERVAL) return;
    lastHeartbeat = millis();
    
    String msg = "$[INFO]@" + controllerId + "{^HEARTBEAT^(client_id=" + 
                controllerId + ",timestamp=" + String(millis()) + ",status=OK)}#";
    
    DEBUG_PRINT(F("发送: "));
    DEBUG_PRINTLN(msg);
    
    // 尝试发送，如果失败则断开连接
    if (!client.print(msg)) {
        DEBUG_PRINTLN(F("心跳发送失败，连接可能已断开"));
        client.stop();
        connectionState = CONN_CONNECTING;
        lastReconnectAttempt = 0;
        if (connectionCallback) {
            connectionCallback(false);
        }
    }
}

void UniversalHarbingerClient::handleIncomingData() {
    if (!client.connected()) return;
    
    static String buffer = "";
    
    while (client.available()) {
        char c = client.read();
        
        // 忽略换行符和回车符
        if (c == '\n' || c == '\r') {
            continue;
        }
        
        buffer += c;
        
        // 检查是否收到完整消息 (以#结尾)
        if (c == '#' && buffer.startsWith("$")) {
            // 触发消息回调
            if (messageCallback) {
                messageCallback(buffer);
            }
            buffer = "";
        }
        
        // 防止缓冲区溢出
        if (buffer.length() > MAX_MESSAGE_LENGTH) {
            DEBUG_PRINT(F("消息过长: "));
            DEBUG_PRINTLN(buffer.length());
            buffer = "";
        }
    }
}

// ========================== 消息发送 ==========================
bool UniversalHarbingerClient::sendMessage(const String& message) {
    if (!isConnected()) return false;
    
    client.print(message);
    return true;
}

bool UniversalHarbingerClient::sendINFOMessage(const String& command, const String& params) {
    String msg = "$[INFO]@" + controllerId + "{^" + command + "^(" + params + ")}#";
    return sendMessage(msg);
}

bool UniversalHarbingerClient::sendGAMEResponse(const String& command, const String& result) {
    String msg = "$[GAME]@" + controllerId + "{^" + command + "^(result=" + result + ")}#";
    
    DEBUG_PRINT(F("发送: "));
    DEBUG_PRINTLN(msg);
    
    return sendMessage(msg);
}

bool UniversalHarbingerClient::sendHARDResponse(const String& command, const String& result) {
    String msg = "$[HARD]@" + controllerId + "{^" + command + "^(result=" + result + ")}#";
    
    DEBUG_PRINT(F("发送: "));
    DEBUG_PRINTLN(msg);
    
    return sendMessage(msg);
}

// ========================== 主循环处理 ==========================
void UniversalHarbingerClient::handleAllNetworkOperations() {
    // 状态机处理连接
    switch (connectionState) {
        case CONN_STABILIZING:
            // 等待网络稳定
            if (millis() - ethernetInitTime >= ETHERNET_STABILIZE_TIME) {
                connectionState = CONN_CONNECTING;
                lastReconnectAttempt = 0;  // 重置重连计时器
            }
            break;
            
        case CONN_CONNECTING:
            // 持续尝试连接服务器
            connectToServer();
            break;
            
        case CONN_CONNECTED:
            // 处理已连接状态
            if (!client.connected()) {
                DEBUG_PRINTLN(F("检测到连接断开"));
                connectionState = CONN_CONNECTING;
                lastReconnectAttempt = 0;  // 立即重连
                if (connectionCallback) {
                    connectionCallback(false);
                }
            } else {
                // 额外检查：如果长时间没有收到服务器响应，认为连接已断开
                if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL * 3) {
                    // 测试连接是否真的还活着
                    if (client.available() == 0 && !client.connected()) {
                        DEBUG_PRINTLN(F("连接超时，强制重连"));
                        client.stop();
                        connectionState = CONN_CONNECTING;
                        lastReconnectAttempt = 0;
                        if (connectionCallback) {
                            connectionCallback(false);
                        }
                        break;
                    }
                }
                
                // 处理消息和心跳
                handleIncomingData();
                sendHeartbeat();
            }
            break;
            
        case CONN_ERROR:
            // 错误状态，尝试重新初始化
            if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL * 3) {
                begin(controllerId, deviceType);
            }
            break;
            
        default:
            break;
    }
}

// ========================== 工具方法 ==========================
String UniversalHarbingerClient::buildDeviceList() {
    // 构建设备列表，格式为逗号分隔的设备ID列表
    // C302有27个设备：2个蜡烛灯 + 25个迷宫按键灯
    String deviceList = "";
    
    // 蜡烛灯：C03LK01, C03LK02
    deviceList += "C03LK01,C03LK02";
    
    // 迷宫按键灯：C03IL01-C03IL25
    for (int i = 1; i <= 25; i++) {
        deviceList += ",C03IL";
        if (i < 10) deviceList += "0";
        deviceList += String(i);
    }
    
    return deviceList;
}

bool UniversalHarbingerClient::validateMessageFormat(const String& message) {
    // 简单的消息格式验证
    return message.startsWith("$[") && message.endsWith("]#");
}

void UniversalHarbingerClient::printStatus() {
    Serial.print(F("ID: "));
    Serial.println(controllerId);
    Serial.print(F("IP: "));
    Serial.println(getLocalIP());
    Serial.print(F("连接: "));
    Serial.println(isConnected() ? F("ON") : F("OFF"));
} 