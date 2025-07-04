# C101 通用Arduino控制器

## 项目概述

C101是基于C302项目清理后的通用Arduino控制器模板，移除了所有C302特定的游戏流程代码，保留了完整的基础框架和通用功能。

## 主要功能

### 核心系统
- **PWM控制系统** - 支持渐变、呼吸效果、批量控制
- **数字IO控制系统** - 支持输出控制、脉冲输出、定时输出、输入监控
- **游戏状态管理** - 标准的游戏状态机(IDLE/INIT/PLAYING/PAUSED/ERROR)
- **网络通信** - 支持Harbinger协议栈(INFO/GAME/HARD)
- **串口命令处理** - 完整的命令解析和处理系统

### 游戏流程框架
- **环节管理系统** - 支持任意环节ID的启动和管理
- **时间轴系统** - 支持复杂的时间点控制和动作序列
- **协议处理** - 支持游戏协议和硬件协议的处理

## 文件结构

### 主程序
- `C101.ino` - 主程序文件

### 核心系统
- `MillisPWM.h/cpp` - PWM控制系统
- `DigitalIOController.h/cpp` - 数字IO控制系统
- `CommandProcessor.h/cpp` - 串口命令处理器
- `GameStateMachine.h/cpp` - 游戏状态机
- `GameFlowManager.h/cpp` - 游戏流程管理器(已清理)
- `SimpleGameStage.h/cpp` - 时间轴系统

### 网络通信
- `UniversalHarbingerClient.h/cpp` - 网络客户端
- `GameProtocolHandler.h/cpp` - 游戏协议处理器
- `HardProtocolHandler.h/cpp` - 硬件协议处理器
- `UniversalGameProtocol.h/cpp` - 通用游戏协议

### 工具类
- `ArduinoSystemHelper.h/cpp` - 系统辅助工具
- `TimeManager.h/cpp` - 时间管理器
- `GameStageStateMachine.h/cpp` - 环节状态机

### 配置文件
- `C302_SimpleConfig.h` - 配置文件样板(保留作为参考)

### 文档
- `串口命令手册.md` - 完整的串口命令说明
- `test_commands.md` - 数字IO测试命令

## 已清理的内容

### 删除的C302特定代码
- 遗迹地图游戏逻辑(25个按键的矩阵控制)
- Level管理系统(1→2→4→3→4→3循环)
- 矩阵旋转系统(4方向旋转)
- 成功/错误计数系统
- 刷新步骤循环管理
- 具体的环节定义(072-0到080-0)
- 蜡烛灯和按键的硬件控制逻辑

### 删除的文档文件
- `test_rotation_logic.md`
- `test_rotation_fix.md` 
- `test_rotation.md`
- `REFRESH_CYCLE_IMPLEMENTATION.md`
- `STAGE_PREFIX_FIX.md`

### 保留的通用功能
- 基础的环节管理框架
- 完整的PWM和数字IO控制系统
- 网络通信和协议处理
- 游戏状态机和命令处理器
- 时间轴系统和工具类

## 使用方法

### 1. 硬件配置
在主程序的硬件初始化部分添加具体的引脚配置：
```cpp
// TODO: 在这里添加具体的硬件初始化代码
pinMode(LED_PIN, OUTPUT);
// ... 其他硬件初始化
```

### 2. 设备列表配置
在`UniversalHarbingerClient.cpp`的`buildDeviceList()`方法中添加设备列表：
```cpp
String deviceList = "";
// 添加具体的设备ID
deviceList += "C01LK01,C01LK02";
```

### 3. 环节定义
在`GameFlowManager.cpp`的`startStage()`方法中添加具体的环节逻辑：
```cpp
if (normalizedId == "001-0") {
    // 添加环节001-0的具体实现
    return true;
}
```

### 4. 环节前缀配置
默认环节前缀为"001-"，可以通过以下方式修改：
```cpp
gameFlowManager.setStagePrefix("002-");
```

## 串口命令

### 基础命令
- `help` - 显示帮助信息
- `status` - 显示系统状态
- `reset` - 重置系统

### PWM控制
- `p24 128` - 设置引脚24的PWM值为128
- `b24 1000` - 引脚24呼吸效果，周期1000ms
- `f24 1000` - 引脚24渐入效果，用时1000ms

### 数字IO控制
- `o24h` - 引脚24输出高电平
- `pulse24:1000` - 引脚24脉冲1秒

### 游戏控制
- `INIT/START/STOP/PAUSE/RESUME` - 游戏状态控制
- `001-0` - 启动环节001-0
- `game_status` - 查看游戏流程状态

## 开发建议

1. **模块化开发** - 保持各系统的独立性和可扩展性
2. **配置驱动** - 通过配置文件管理硬件和游戏参数
3. **协议兼容** - 遵循Harbinger协议规范
4. **错误处理** - 完善的错误检测和恢复机制
5. **文档更新** - 及时更新配置和使用文档

## 版本信息

- **基础版本**: 基于C302 v1.0
- **清理日期**: 2025-01-03
- **当前状态**: 通用框架，待具体实现 