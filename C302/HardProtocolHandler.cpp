#include "HardProtocolHandler.h"

// 全局实例
HardProtocolHandler hardProtocolHandler;

void HardProtocolHandler::begin(const String& controllerIdStr) {
    controllerId = controllerIdStr;
    #ifdef DEBUG
    Serial.println(F("HardProtocolHandler初始化完成"));
    #endif
}

void HardProtocolHandler::processHardMessage(const String& message) {
    #ifdef DEBUG
    Serial.print(F("处理HARD消息: "));
    Serial.println(message);
    #endif
    
    // 解析协议格式: $[HARD]@DEVICE_ID{^COMMAND^(params)}#
    int commandStart = message.indexOf("^") + 1;
    int commandEnd = message.indexOf("^", commandStart);
    
    if (commandStart <= 0 || commandEnd <= commandStart) {
        #ifdef DEBUG
        Serial.println(F("HARD消息格式错误"));
        #endif
        return;
    }
    
    String command = message.substring(commandStart, commandEnd);
    String params = extractParams(message);
    
    #ifdef DEBUG
    Serial.print(F("HARD命令: "));
    Serial.print(command);
    Serial.print(F(" 参数: "));
    Serial.println(params);
    #endif
    
    // 处理不同的HARD命令
    if (command == "SINGLE") {
        handleHardSingle(params);
    } else if (command == "MULTI") {
        handleHardMulti(params);
    } else if (command == "EMERGENCY") {
        handleHardEmergency(params);
    } else {
        #ifdef DEBUG
        Serial.print(F("未知HARD命令: "));
        Serial.println(command);
        #endif
        sendHardError("未知命令: " + command);
    }
}

void HardProtocolHandler::handleHardSingle(const String& params) {
    String componentId = extractParam(params, "component_id");
    String action = extractParam(params, "action");
    String controlParams = extractParam(params, "params");
    
    #ifdef DEBUG
    Serial.print(F("SINGLE控制: "));
    Serial.print(componentId);
    Serial.print(F(" -> "));
    Serial.print(action);
    Serial.print(F(" ("));
    Serial.print(controlParams);
    Serial.println(F(")"));
    #endif
    
    if (!validateComponentId(componentId)) {
        sendHardError("无效的元器件ID: " + componentId);
        return;
    }
    
    bool success = executeComponentControl(componentId, action, controlParams);
    
    if (success) {
        sendHardSingleAck(componentId, action);
    } else {
        sendHardError("控制失败: " + componentId);
    }
}

void HardProtocolHandler::handleHardMulti(const String& params) {
    String componentList = extractParam(params, "component_list");
    String actionList = extractParam(params, "action_list");
    String paramsList = extractParam(params, "params_list");
    
    int componentCount = countItems(componentList);
    int actionCount = countItems(actionList);
    
    if (componentCount != actionCount || componentCount == 0) {
        sendHardError("参数列表长度不匹配");
        return;
    }
    
    if (componentCount > 10) {
        sendHardError("超出批量限制(最大10个)");
        return;
    }
    
    int successCount = 0;
    for (int i = 0; i < componentCount; i++) {
        String componentId = getListItem(componentList, i);
        String action = getListItem(actionList, i);
        String controlParams = getListItem(paramsList, i);
        
        if (validateComponentId(componentId)) {
            if (executeComponentControl(componentId, action, controlParams)) {
                successCount++;
            }
        }
    }
    
    sendHardMultiAck(componentCount, successCount);
}

void HardProtocolHandler::handleHardEmergency(const String& params) {
    String scope = extractParam(params, "scope");
    if (scope.length() == 0) scope = "all";
    
    #ifdef DEBUG
    Serial.print(F("紧急停止: "));
    Serial.println(scope);
    #endif
    
    if (scope == "all") {
        systemHelper.stopAllDevices();
        MillisPWM::stopAll();
    } else if (scope == "lighting") {
        MillisPWM::stopAll();
    } else if (scope == "power") {
        systemHelper.stopAllDevices();
    }
    
    sendHardEmergencyAck(scope);
}

bool HardProtocolHandler::executeComponentControl(const String& componentId, const String& action, const String& controlParams) {
    String componentType = componentId.substring(3, 5);
    
    if (componentType == "LK" || componentType == "LD" || componentType == "LR") {
        return controlLighting(componentId, action, controlParams);
    } else if (componentType == "AL" || componentType == "RL") {
        return controlPower(componentId, action, controlParams);
    }
    
    return false;
}

bool HardProtocolHandler::controlLighting(const String& componentId, const String& action, const String& controlParams) {
    int pin = getComponentPin(componentId);
    if (pin == -1) return false;
    
    if (action == "on") {
        int brightness = extractParamValue(controlParams, "brightness", 100);
        MillisPWM::setBrightnessPercent(pin, brightness);
        return true;
    } else if (action == "off") {
        MillisPWM::stop(pin);
        return true;
    } else if (action == "breath") {
        float cycle = extractParamValue(controlParams, "cycle", 2.0f);
        MillisPWM::startBreathing(pin, cycle);
        return true;
    }
    
    return false;
}

bool HardProtocolHandler::controlPower(const String& componentId, const String& action, const String& controlParams) {
    int pin = getComponentPin(componentId);
    if (pin == -1) return false;
    
    if (action == "on" || action == "open") {
        digitalWrite(pin, HIGH);
        return true;
    } else if (action == "off" || action == "close") {
        digitalWrite(pin, LOW);
        return true;
    }
    
    return false;
}

// ========================== 参数解析辅助函数 ==========================
String HardProtocolHandler::extractParams(const String& message) {
    int start = message.indexOf("(") + 1;
    int end = message.lastIndexOf(")");
    if (start > 0 && end > start) {
        return message.substring(start, end);
    }
    return "";
}

String HardProtocolHandler::extractParam(const String& params, const String& paramName) {
    String searchStr = paramName + "=";
    int start = params.indexOf(searchStr);
    if (start == -1) return "";
    
    start += searchStr.length();
    int end = params.indexOf(",", start);
    if (end == -1) end = params.length();
    
    return params.substring(start, end);
}

int HardProtocolHandler::extractParamValue(const String& controlParams, const String& paramName, int defaultValue) {
    String value = extractParam(controlParams, paramName);
    return value.length() > 0 ? value.toInt() : defaultValue;
}

float HardProtocolHandler::extractParamValue(const String& controlParams, const String& paramName, float defaultValue) {
    String value = extractParam(controlParams, paramName);
    return value.length() > 0 ? value.toFloat() : defaultValue;
}

bool HardProtocolHandler::validateComponentId(const String& componentId) {
    if (componentId.length() != 7) return false;
    if (componentId.charAt(0) != 'C') return false;
    if (!isDigit(componentId.charAt(1)) || !isDigit(componentId.charAt(2))) return false;
    if (!isAlpha(componentId.charAt(3)) || !isAlpha(componentId.charAt(4))) return false;
    if (!isDigit(componentId.charAt(5)) || !isDigit(componentId.charAt(6))) return false;
    return true;
}

int HardProtocolHandler::getComponentPin(const String& componentId) {
    if (componentId.endsWith("01")) return 22;
    if (componentId.endsWith("02")) return 23;
    return -1;
}

int HardProtocolHandler::countItems(const String& list) {
    if (list.length() == 0) return 0;
    int count = 1;
    for (int i = 0; i < list.length(); i++) {
        if (list.charAt(i) == ',') count++;
    }
    return count;
}

String HardProtocolHandler::getListItem(const String& list, int index) {
    if (index < 0) return "";
    
    int start = 0;
    int currentIndex = 0;
    
    while (currentIndex < index && start < list.length()) {
        int commaPos = list.indexOf(',', start);
        if (commaPos == -1) return "";
        start = commaPos + 1;
        currentIndex++;
    }
    
    int end = list.indexOf(',', start);
    if (end == -1) end = list.length();
    
    return list.substring(start, end);
}

// ========================== HARD协议响应函数 ==========================
void HardProtocolHandler::sendHardSingleAck(const String& componentId, const String& action) {
    String result = "component_id=" + componentId + ",action=" + action + ",status=success";
    harbingerClient.sendHARDResponse("SINGLE_ACK", result);
}

void HardProtocolHandler::sendHardMultiAck(int total, int success) {
    String result = "total=" + String(total) + ",success=" + String(success) + ",status=completed";
    harbingerClient.sendHARDResponse("MULTI_ACK", result);
}

void HardProtocolHandler::sendHardEmergencyAck(const String& scope) {
    String result = "scope=" + scope + ",status=stopped,timestamp=" + String(TimeManager::now());
    harbingerClient.sendHARDResponse("EMERGENCY_ACK", result);
}

void HardProtocolHandler::sendHardError(const String& errorMsg) {
    harbingerClient.sendHARDResponse("ERROR", "message=" + errorMsg);
} 