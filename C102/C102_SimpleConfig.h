/**
 * =============================================================================
 * C102控制器简化配置 - C102_SimpleConfig.h
 * 创建日期: 2025-01-03
 * 描述信息: C102控制器的配置，基于CSV文件实际引脚分配
 * 游戏类型: 4路语音控制器 | 房间号: 1 | 控制器: C102
 * =============================================================================
 */

#ifndef C102_SIMPLE_CONFIG_H
#define C102_SIMPLE_CONFIG_H

#include <Arduino.h>

// ========================== C102控制器基本信息 ==========================
#define C102_CONTROLLER_ID    "C102"
#define C102_CONTROLLER_TYPE  "Arduino"
#define C102_DEVICE_COUNT     4        // 4路语音模块

// ========================== 网络配置 ==========================
#define C102_SERVER_IP_1      192
#define C102_SERVER_IP_2      168
#define C102_SERVER_IP_3      10
#define C102_SERVER_IP_4      10
#define C102_SERVER_PORT      9000

// ========================== 游戏流程配置 ==========================
#define C102_STAGE_PREFIX     "001-"   // C102游戏环节前缀

// ========================== 语音模块配置 ==========================
// 4路语音模块Busy引脚
static const int C102_BUSY_PINS[C102_DEVICE_COUNT] = {
    22, 23, 24, 25  // 通道1-4的Busy引脚
};

// 软串口引脚配置 (通道4)
#define C102_SOFT_RX_PIN      2
#define C102_SOFT_TX_PIN      3

// 设备ID数组 (4个语音模块)
static const char* C102_DEVICE_IDS[C102_DEVICE_COUNT] = {
    "C01MA01", "C01MA02", "C01MA03", "C01MA04"  // 4路音频模块
};

// ========================== 设备注册字符串生成 ==========================
/**
 * @brief 生成C102设备注册字符串
 * @return 设备列表字符串
 */
inline String getC102DeviceList() {
    String deviceList = "";
    for (int i = 0; i < C102_DEVICE_COUNT; i++) {
        if (i > 0) deviceList += ",";
        deviceList += C102_DEVICE_IDS[i];
    }
    return deviceList;
}

// ========================== 硬件初始化函数 ==========================
/**
 * @brief 初始化C102语音模块硬件引脚
 */
inline void initC102Hardware() {
    // 初始化所有Busy引脚为输入上拉
    for (int i = 0; i < C102_DEVICE_COUNT; i++) {
        pinMode(C102_BUSY_PINS[i], INPUT_PULLUP);
    }
}

#endif // C102_SIMPLE_CONFIG_H