/**
 * =============================================================================
 * CommandProcessor - 命令处理器库
 * 版本: 1.0
 * 创建日期: 2024-12-28
 * 
 * 功能:
 * - 统一的命令解析和处理
 * - 支持多种命令格式
 * - 集成PWM控制和游戏状态管理
 * - 简化的命令接口
 * =============================================================================
 */

#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <Arduino.h>
#include "MillisPWM.h"
#include "GameStateMachine.h"
#include "UniversalGameProtocol.h"
#include "SimpleGameStage.h"

/**
 * @brief 命令处理器类
 * 统一处理各种输入命令
 */
class CommandProcessor {
private:
    bool initialized;
    
    // 命令回调函数指针
    void (*customCommandCallback)(const String& command, const String& params);
    
public:
    // ========================== 构造和初始化 ==========================
    CommandProcessor();
    void begin();
    
    // ========================== 命令处理 ==========================
    /**
     * @brief 处理输入的命令字符串
     * @param input 完整的命令字符串
     * @return true=处理成功，false=命令无效
     */
    bool processCommand(const String& input);
    
    // ========================== 具体命令处理器 ==========================
    /**
     * @brief 处理PWM相关命令
     * @param command 命令名
     * @param params 参数
     * @return true=处理成功
     */
    bool processPWMCommand(const String& command, const String& params);
    
    /**
     * @brief 处理游戏协议命令
     * @param command 命令名
     * @param params 参数
     * @return true=处理成功
     */
    bool processGameCommand(const String& command, const String& params);
    
    /**
     * @brief 处理系统命令
     * @param command 命令名
     * @param params 参数
     * @return true=处理成功
     */
    bool processSystemCommand(const String& command, const String& params);
    
    /**
     * @brief 处理简化PWM命令
     * @param command 命令名
     * @param params 参数
     * @return true=处理成功
     */
    bool processSimplePWMCommand(const String& command, const String& params);
    
    /**
     * @brief 处理数字IO命令
     * @param command 命令名
     * @param params 参数
     * @return true=处理成功
     */
    bool processDigitalIOCommand(const String& command, const String& params);
    
    /**
     * @brief 处理音频控制命令
     * @param command 命令名
     * @param params 参数
     * @return true=处理成功
     */
    bool processVoiceCommand(const String& command, const String& params);
    
    // ========================== 帮助和状态 ==========================
    /**
     * @brief 显示帮助信息
     */
    void showHelp();
    
    /**
     * @brief 显示系统状态
     */
    void showStatus();
    
    // ========================== 回调设置 ==========================
    /**
     * @brief 设置自定义命令回调
     * @param callback 回调函数
     */
    void setCustomCommandCallback(void (*callback)(const String& command, const String& params));
    
    // ========================== 手动协议发送 ==========================
    /**
     * @brief 处理手动协议发送命令
     * @param command 命令字符串
     * @param params 参数字符串
     * @return true=处理成功，false=未处理
     */
    bool processManualProtocol(const String& command, const String& params);
    
private:
    // ========================== 内部解析函数 ==========================
    /**
     * @brief 解析命令字符串
     * @param input 输入字符串
     * @param command 输出命令名
     * @param params 输出参数
     * @return true=解析成功
     */
    bool parseCommand(const String& input, String& command, String& params);
    
    /**
     * @brief 解析PWM参数
     * @param params 参数字符串
     * @param pin 输出引脚号
     * @param value 输出值
     * @return true=解析成功
     */
    bool parsePWMParams(const String& params, int& pin, int& value);
    
    /**
     * @brief 解析简化命令参数
     * @param params 参数字符串
     * @param pin 输出引脚号
     * @param value 输出值
     * @return true=解析成功
     */
    bool parseSimpleParams(const String& params, int& pin, int& value);
    
    /**
     * @brief 提取引脚号从命令中
     * @param command 命令字符串
     * @return 引脚号，-1表示无效
     */
    int extractPinFromCommand(const String& command);
    
    // ========================== 调试功能 ==========================
    void debugPrint(const String& message);
};

// ========================== 全局实例 ==========================
extern CommandProcessor commandProcessor;

#endif // COMMAND_PROCESSOR_H 