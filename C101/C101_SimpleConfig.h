/**
 * =============================================================================
 * C101控制器简化配置 - C101_SimpleConfig.h
 * 创建日期: 2025-01-03
 * 描述信息: C101控制器的配置，基于CSV文件实际引脚分配
 * 游戏类型: 序章综合控制器 | 房间号: 1 | 控制器: C101
 * =============================================================================
 */

#ifndef C101_SIMPLE_CONFIG_H
#define C101_SIMPLE_CONFIG_H

// ========================== C101控制器基本信息 ==========================
#define C101_CONTROLLER_ID    "C101"
#define C101_CONTROLLER_TYPE  "Arduino"

// ========================== 网络配置 ==========================
#define C101_SERVER_IP_1      192
#define C101_SERVER_IP_2      168
#define C101_SERVER_IP_3      10
#define C101_SERVER_IP_4      10
#define C101_SERVER_PORT      9000

// ========================== 游戏流程配置 ==========================
#define C101_STAGE_PREFIX     "001_"   // C101游戏环节前缀

// ========================== 主题入口门系统 ==========================
// 电磁锁控制 (C01AL01)
#define C101_DOOR_LOCK_PIN           26    // 电磁锁继电器控制

// 门禁读卡器 (C01ID01)  
#define C101_DOOR_CARD_COM_PIN       24    // 门禁读卡器COM检测

// 入口门干簧管 (C01SF01)
#define C101_DOOR_REED_PIN           23    // 门状态检测

// 指引射灯 (C01LS01)
#define C101_DOOR_LIGHT_PIN          25    // 指引射灯继电器控制

// ========================== 序章氛围系统 ==========================
// 植物灯控制 (C01LV01-04)
#define C101_PLANT_LIGHT_COUNT       4
static const int C101_PLANT_LIGHT_PINS[C101_PLANT_LIGHT_COUNT] = {
    2, 3, 6, 5  // 植物灯1-4继电器控制引脚
};

// 氛围射灯 (C01LS02) - 已废弃但保留配置
#define C101_AMBIENT_LIGHT_PIN       A1   // 氛围射灯继电器控制

// ========================== 序章嘲讽系统 ==========================
// 大平头带灯按键 (C01IJ01-04)
#define C101_TAUNT_BUTTON_COUNT      4
static const int C101_TAUNT_BUTTON_LIGHT_PINS[C101_TAUNT_BUTTON_COUNT] = {
    30, 32, 34, 36  // 按键灯光继电器控制引脚
};
static const int C101_TAUNT_BUTTON_COM_PINS[C101_TAUNT_BUTTON_COUNT] = {
    31, 33, 35, 37  // 按键COM检测引脚
};

// ========================== 序章嘲讽音频系统 (IO控制型) ==========================
// IO控制音频模块 (C01MA05-08) - 兼容C102的BY语音控制器接口
#define C101_AUDIO_MODULE_COUNT      4
static const int C101_AUDIO_IO1_PINS[C101_AUDIO_MODULE_COUNT] = {
    15, 16, A4, 20  // 音频模块IO1控制引脚
};
static const int C101_AUDIO_IO2_PINS[C101_AUDIO_MODULE_COUNT] = {
    A0, 17, A5, 21  // 音频模块IO2控制引脚
};

// 兼容C102接口的软串口配置（虽然C101不使用软串口，但保持接口一致）
#define C101_SOFT_TX_PIN             7      // 虚拟软串口TX（不实际使用）
#define C101_SOFT_RX_PIN             8      // 虚拟软串口RX（不实际使用）

// 兼容C102接口的BUSY引脚配置（C101使用IO控制，但保持接口一致）
static const int C101_BUSY_PINS[C101_AUDIO_MODULE_COUNT] = {
    A1, A2, A3, A6  // 虚拟BUSY引脚（实际用于状态监控）
};

// 设备ID数组 (4个IO控制音频模块)
static const char* C101_AUDIO_DEVICE_IDS[C101_AUDIO_MODULE_COUNT] = {
    "C01MA05", "C01MA06", "C01MA07", "C01MA08"  // 序章嘲讽音频模块
};

// ========================== 画灯谜题系统 ==========================
// 画射灯 (C01LS03-10)
#define C101_PAINTING_LIGHT_COUNT    8
static const int C101_PAINTING_LIGHT_PINS[C101_PAINTING_LIGHT_COUNT] = {
    38, 39, 40, 41, A13, 43, 44, 45  // 画射灯1-8继电器控制引脚
};

// 触摸按键 (C01IT01-02)
#define C101_TOUCH_BUTTON_COUNT      2
static const int C101_TOUCH_BUTTON_PINS[C101_TOUCH_BUTTON_COUNT] = {
    46, A10  // 触摸按键信号检测引脚
};

// 提示灯带 (C01LR01-02)
#define C101_HINT_LED_COUNT          2
static const int C101_HINT_LED_PINS[C101_HINT_LED_COUNT] = {
    A11, A12  // 提示灯带信号控制引脚
};

// ========================== 蝴蝶灯谜题系统 ==========================
// 门禁读卡器1 (C01ID02) - 其他读卡器未分配引脚
#define C101_BUTTERFLY_CARD_RELAY_PIN    27    // 门禁读卡器继电器控制
#define C101_BUTTERFLY_CARD_COM_PIN      49    // 门禁读卡器COM检测

// 蝴蝶灯 (C01LW01)
#define C101_BUTTERFLY_LIGHT_PIN         A15   // 蝴蝶灯继电器控制

// 广告风扇 (C01LG01)  
#define C101_AD_FAN_PIN                  A14   // 广告风扇继电器控制

// ========================== 设备注册字符串生成 ==========================
/**
 * @brief 生成C101设备注册字符串
 * @return 设备列表字符串
 */
inline String getC101DeviceList() {
    String deviceList = "";
    
    // 添加主要系统设备
    deviceList += "C01AL01,C01ID01,C01SF01,C01LS01,";  // 入口门系统
    deviceList += "C01LV01,C01LV02,C01LV03,C01LV04,";  // 植物灯系统
    deviceList += "C01IJ01,C01IJ02,C01IJ03,C01IJ04,";  // 嘲讽按键系统
    
    // 添加音频模块
    for (int i = 0; i < C101_AUDIO_MODULE_COUNT; i++) {
        if (i > 0) deviceList += ",";
        deviceList += C101_AUDIO_DEVICE_IDS[i];
    }
    deviceList += ",";
    
    // 添加画灯谜题系统
    deviceList += "C01LS03,C01LS04,C01LS05,C01LS06,C01LS07,C01LS08,C01LS09,C01LS10,";
    deviceList += "C01IT01,C01IT02,C01LR01,C01LR02,";
    
    // 添加蝴蝶灯谜题系统
    deviceList += "C01ID02,C01LW01,C01LG01";
    
    return deviceList;
}

// ========================== 硬件初始化函数 ==========================
/**
 * @brief 初始化C101所有硬件引脚
 */
inline void initC101Hardware() {
    // 初始化入口门系统
    pinMode(C101_DOOR_LOCK_PIN, OUTPUT);
    pinMode(C101_DOOR_CARD_COM_PIN, INPUT_PULLUP);
    pinMode(C101_DOOR_REED_PIN, INPUT_PULLUP);
    pinMode(C101_DOOR_LIGHT_PIN, OUTPUT);
    
    // 初始化植物灯系统
    for (int i = 0; i < C101_PLANT_LIGHT_COUNT; i++) {
        pinMode(C101_PLANT_LIGHT_PINS[i], OUTPUT);
        digitalWrite(C101_PLANT_LIGHT_PINS[i], LOW);  // 默认关闭
    }
    
    // 初始化氛围射灯
    pinMode(C101_AMBIENT_LIGHT_PIN, OUTPUT);
    digitalWrite(C101_AMBIENT_LIGHT_PIN, LOW);
    
    // 初始化嘲讽按键系统
    for (int i = 0; i < C101_TAUNT_BUTTON_COUNT; i++) {
        pinMode(C101_TAUNT_BUTTON_LIGHT_PINS[i], OUTPUT);
        pinMode(C101_TAUNT_BUTTON_COM_PINS[i], INPUT_PULLUP);
        digitalWrite(C101_TAUNT_BUTTON_LIGHT_PINS[i], LOW);  // 默认关闭灯光
    }
    
    // 初始化IO控制音频模块（兼容C102接口）
    for (int i = 0; i < C101_AUDIO_MODULE_COUNT; i++) {
        pinMode(C101_AUDIO_IO1_PINS[i], OUTPUT);
        pinMode(C101_AUDIO_IO2_PINS[i], OUTPUT);
        digitalWrite(C101_AUDIO_IO1_PINS[i], HIGH);  // 默认高电平(1,1)不触发
        digitalWrite(C101_AUDIO_IO2_PINS[i], HIGH);  // 默认高电平(1,1)不触发
        
        // 初始化虚拟BUSY引脚（用于状态监控）
        pinMode(C101_BUSY_PINS[i], INPUT_PULLUP);
    }
    
    // 初始化画灯谜题系统
    for (int i = 0; i < C101_PAINTING_LIGHT_COUNT; i++) {
        pinMode(C101_PAINTING_LIGHT_PINS[i], OUTPUT);
        digitalWrite(C101_PAINTING_LIGHT_PINS[i], LOW);  // 默认关闭
    }
    
    for (int i = 0; i < C101_TOUCH_BUTTON_COUNT; i++) {
        pinMode(C101_TOUCH_BUTTON_PINS[i], INPUT_PULLUP);
    }
    
    for (int i = 0; i < C101_HINT_LED_COUNT; i++) {
        pinMode(C101_HINT_LED_PINS[i], OUTPUT);
        digitalWrite(C101_HINT_LED_PINS[i], LOW);  // 默认关闭
    }
    
    // 初始化蝴蝶灯谜题系统
    pinMode(C101_BUTTERFLY_CARD_RELAY_PIN, OUTPUT);
    pinMode(C101_BUTTERFLY_CARD_COM_PIN, INPUT_PULLUP);
    pinMode(C101_BUTTERFLY_LIGHT_PIN, OUTPUT);
    pinMode(C101_AD_FAN_PIN, OUTPUT);
    
    // 默认状态设置
    digitalWrite(C101_DOOR_LOCK_PIN, HIGH);          // 电磁锁默认上锁（通电）
    digitalWrite(C101_DOOR_LIGHT_PIN, LOW);          // 指引射灯默认关闭
    digitalWrite(C101_BUTTERFLY_CARD_RELAY_PIN, LOW); // 门禁读卡器默认关闭
    digitalWrite(C101_BUTTERFLY_LIGHT_PIN, HIGH);    // 蝴蝶灯初始化时置为HIGH
    digitalWrite(C101_AD_FAN_PIN, LOW);              // 广告风扇默认关闭
}

#endif // C101_SIMPLE_CONFIG_H