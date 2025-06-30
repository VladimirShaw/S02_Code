/**
 * =============================================================================
 * C302控制器简化配置 - C302_SimpleConfig.h
 * 创建日期: 2025-01-03
 * 描述信息: C302控制器的配置，基于CSV文件实际引脚分配
 * 游戏类型: 遗迹地图 | 房间号: 3 | 控制器: C302
 * =============================================================================
 */

#ifndef C302_SIMPLE_CONFIG_H
#define C302_SIMPLE_CONFIG_H

#include <Arduino.h>

// ========================== C302控制器基本信息 ==========================
#define C302_CONTROLLER_ID    "C302"
#define C302_CONTROLLER_TYPE  "Arduino"
#define C302_DEVICE_COUNT     27

// ========================== 网络配置 ==========================
#define C302_SERVER_IP_1      192
#define C302_SERVER_IP_2      168
#define C302_SERVER_IP_3      10
#define C302_SERVER_IP_4      10
#define C302_SERVER_PORT      9000

// ========================== 设备引脚配置 ==========================
// 设备输出引脚数组 (27个设备)
static const int C302_DEVICE_PINS[C302_DEVICE_COUNT] = {
    // 蜡烛灯 (2个)
    22, 23,
    
    // 白色方形带灯按键 - 灯光控制 (25个)
    24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48,  // Pin24-Pin48 (偶数)
    A10, A12, A14,                                        // A10, A12, A14
    5, 14, 16, 18, 20,                                    // Pin5, Pin14, Pin16, Pin18, Pin20
    A0, A2, A4, A8                                        // A0, A2, A4, A8
};

// 设备ID数组 (27个设备)
static const char* C302_DEVICE_IDS[C302_DEVICE_COUNT] = {
    // 蜡烛灯 (2个)
    "C03LK01", "C03LK02",
    
    // 白色方形带灯按键 (25个)
    "C03IL01", "C03IL02", "C03IL03", "C03IL04", "C03IL05",
    "C03IL06", "C03IL07", "C03IL08", "C03IL09", "C03IL10",
    "C03IL11", "C03IL12", "C03IL13", "C03IL14", "C03IL15",
    "C03IL16", "C03IL17", "C03IL18", "C03IL19", "C03IL20",
    "C03IL21", "C03IL22", "C03IL23", "C03IL24", "C03IL25"
};

// ========================== 按键引脚映射 ==========================
// 按键检测引脚（输入上拉）- 与灯光控制引脚配对 (25个按键)
static const int C302_BUTTON_PINS[25] = {
    25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49,  // Pin25-Pin49 (奇数)
    A11, A13, A15,                                        // A11, A13, A15
    6, 15, 17, 19, 21,                                    // Pin6, Pin15, Pin17, Pin19, Pin21
    A1, A3, A5, A9                                        // A1, A3, A5, A9
};

// ========================== 设备注册字符串生成 ==========================
/**
 * @brief 生成C302设备注册字符串
 * @return 设备列表字符串
 */
inline String getC302DeviceList() {
    String deviceList = "";
    for (int i = 0; i < C302_DEVICE_COUNT; i++) {
        if (i > 0) deviceList += ",";
        deviceList += C302_DEVICE_IDS[i];
    }
    return deviceList;
}

// ========================== 硬件初始化函数 ==========================
/**
 * @brief 初始化C302所有硬件引脚
 */
inline void initC302Hardware() {
    // 初始化所有设备的输出引脚
    for (int i = 0; i < C302_DEVICE_COUNT; i++) {
        pinMode(C302_DEVICE_PINS[i], OUTPUT);
        digitalWrite(C302_DEVICE_PINS[i], LOW);
    }
    
    // 初始化所有按键的输入引脚
    for (int i = 0; i < 25; i++) {
        pinMode(C302_BUTTON_PINS[i], INPUT_PULLUP);
    }
}

#endif // C302_SIMPLE_CONFIG_H