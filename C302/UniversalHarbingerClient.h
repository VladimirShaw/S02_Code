/**
 * =============================================================================
 * 通用Harbinger客户端 - UniversalHarbingerClient.h
 * 创建日期: 2025-01-03
 * 描述信息: 通用的Harbinger协议客户端，支持任意控制器ID
 * =============================================================================
 */

#ifndef UNIVERSAL_HARBINGER_CLIENT_H
#define UNIVERSAL_HARBINGER_CLIENT_H

#include <Arduino.h>
#include <Ethernet.h>

// ========================== 配置常量 ==========================
#define MAX_MESSAGE_LENGTH    200
#define CONNECTION_TIMEOUT    5000
#define HEARTBEAT_INTERVAL    3000
#define RECONNECT_INTERVAL    5000
#define ETHERNET_STABILIZE_TIME 800  // 🚀 从2秒减少到0.8秒

// ========================== 连接状态 ==========================
enum ConnectionState {
    CONN_DISCONNECTED = 0,
    CONN_INITIALIZING = 1,
    CONN_STABILIZING = 2,
    CONN_CONNECTING = 3,
    CONN_CONNECTED = 4,
    CONN_AUTHENTICATED = 5,
    CONN_ERROR = 255
};

// ========================== 回调函数类型 ==========================
typedef void (*ConnectionChangeCallback)(bool connected);
typedef void (*MessageReceivedCallback)(String message);

// ========================== UniversalHarbingerClient类 ==========================
class UniversalHarbingerClient {
private:
    // 网络配置
    String controllerId;
    String deviceType;
    IPAddress serverIP;
    uint16_t serverPort;
    
    // 连接管理
    EthernetClient client;
    ConnectionState connectionState;
    unsigned long lastHeartbeat;
    unsigned long lastReconnectAttempt;
    unsigned long ethernetInitTime;
    bool networkInitialized;
    bool firstConnectionAttempted;
    
    // 回调函数
    ConnectionChangeCallback connectionCallback;
    MessageReceivedCallback messageCallback;
    
    // 内部方法
    bool connectToServer();
    void handleIncomingData();
    void sendHeartbeat();
    void sendRegistration();
    bool validateMessageFormat(const String& message);
    String buildDeviceList();
    
public:
    // 构造函数和析构函数
    UniversalHarbingerClient();
    ~UniversalHarbingerClient();
    
    // 初始化和连接
    bool begin(const String& controllerId, const String& deviceType);
    bool connect(IPAddress serverIP, uint16_t serverPort);
    void disconnect();
    
    // 状态查询
    bool isConnected() const;
    ConnectionState getConnectionState() const;
    String getLocalIP() const;
    String getServerInfo() const;
    
    // 回调设置
    void setConnectionCallback(ConnectionChangeCallback callback);
    void setMessageCallback(MessageReceivedCallback callback);
    
    // 消息发送
    bool sendMessage(const String& message);
    bool sendINFOMessage(const String& command, const String& params = "");
    bool sendGAMEResponse(const String& command, const String& result = "OK");
    bool sendHARDResponse(const String& command, const String& result = "OK");
    
    // 主循环处理
    void handleAllNetworkOperations();
    
    // 调试信息
    void printStatus();
};

// 全局实例
extern UniversalHarbingerClient harbingerClient;

#endif // UNIVERSAL_HARBINGER_CLIENT_H 