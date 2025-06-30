#ifndef HARD_PROTOCOL_HANDLER_H
#define HARD_PROTOCOL_HANDLER_H

#include <Arduino.h>
#include "MillisPWM.h"
#include "ArduinoSystemHelper.h"
#include "UniversalHarbingerClient.h"
#include "TimeManager.h"

class HardProtocolHandler {
private:
    String controllerId;
    
    // 内部处理函数
    void handleHardSingle(const String& params);
    void handleHardMulti(const String& params);
    void handleHardEmergency(const String& params);
    
    // 组件控制函数
    bool executeComponentControl(const String& componentId, const String& action, const String& controlParams);
    bool controlLighting(const String& componentId, const String& action, const String& controlParams);
    bool controlPower(const String& componentId, const String& action, const String& controlParams);
    
    // 响应发送函数
    void sendHardSingleAck(const String& componentId, const String& action);
    void sendHardMultiAck(int total, int success);
    void sendHardEmergencyAck(const String& scope);
    void sendHardError(const String& errorMsg);
    
    // 验证和辅助函数
    bool validateComponentId(const String& componentId);
    int getComponentPin(const String& componentId);
    
    // 参数解析辅助函数
    String extractParams(const String& message);
    String extractParam(const String& params, const String& paramName);
    int extractParamValue(const String& controlParams, const String& paramName, int defaultValue);
    float extractParamValue(const String& controlParams, const String& paramName, float defaultValue);
    int countItems(const String& list);
    String getListItem(const String& list, int index);

public:
    void begin(const String& controllerIdStr);
    void processHardMessage(const String& message);
};

// 全局实例
extern HardProtocolHandler hardProtocolHandler;

#endif 