/**
 * =============================================================================
 * GameFlowManager - C101音频控制器游戏流程管理器
 * 版本: 2.0 - C101专用版本
 * 创建日期: 2025-01-03
 * 
 * 功能:
 * - 专门管理C101音频控制器的游戏环节
 * - 支持BY语音模块4路音频控制
 * - 简洁的环节定义和执行
 * - 与协议处理器解耦
 * - 支持多个环节并行执行
 * =============================================================================
 */

#ifndef GAME_FLOW_MANAGER_H
#define GAME_FLOW_MANAGER_H

#include <Arduino.h>

// ========================== 并行环节配置 ==========================
#define MAX_PARALLEL_STAGES 4  // 最大并行环节数（根据需要调整）

// ========================== 音量管理配置 ==========================
#define DEFAULT_VOLUME 30       // 默认音量值
#define TOTAL_CHANNELS 4        // 总通道数

// ========================== 紧急开门配置 ==========================
#define EMERGENCY_UNLOCK_DURATION 10000  // 紧急解锁持续时间(10秒)
#define EMERGENCY_DEBOUNCE_TIME   100     // 紧急开门防抖时间(100ms)

// ========================== C101音频环节时间配置 ==========================
// 
// 所有音频控制的时间点都在这里定义，方便查看和修改
//

// 000_0 环节：C101初始化环节 - 只有植物灯顺序呼吸效果，无音频（初始化环节，不自动跳转）
// 注意：C101的000_0环节不播放音频，只控制植物灯
#define STAGE_000_0_AUDIO_ENABLED   false    // C101的000_0环节不启用音频
#define STAGE_000_0_START           0        // 启动时间(ms)

// 植物灯顺序呼吸效果配置
#define STAGE_000_0_LIGHT_DURATION  1500     // 每个灯的呼吸持续时间(ms)
#define STAGE_000_0_LIGHT_CYCLE     6000     // 完整循环时间(ms) - 4个灯×1500ms
#define STAGE_000_0_LIGHT_COUNT     4        // 植物灯数量

// ========================== 000_0环节引脚状态配置 ==========================
// 入口门系统
#define STAGE_000_0_DOOR_LOCK_STATE         LOW     // 电磁锁状态 (Pin26)
#define STAGE_000_0_DOOR_LIGHT_STATE        LOW     // 指引射灯状态 (Pin25)

// 氛围射灯系统
#define STAGE_000_0_AMBIENT_LIGHT_STATE     LOW     // 氛围射灯状态 (PinA13)

// 嘲讽按键灯光系统 (4个按键灯)
#define STAGE_000_0_TAUNT_BUTTON1_STATE     LOW     // 按键1灯光状态 (Pin30)
#define STAGE_000_0_TAUNT_BUTTON2_STATE     LOW     // 按键2灯光状态 (Pin32)
#define STAGE_000_0_TAUNT_BUTTON3_STATE     LOW     // 按键3灯光状态 (Pin34)
#define STAGE_000_0_TAUNT_BUTTON4_STATE     LOW     // 按键4灯光状态 (Pin36)

// 画灯谜题系统 (8个画射灯)
#define STAGE_000_0_PAINTING_LIGHT1_STATE   LOW     // 画射灯1状态 (Pin38)
#define STAGE_000_0_PAINTING_LIGHT2_STATE   LOW     // 画射灯2状态 (Pin39)
#define STAGE_000_0_PAINTING_LIGHT3_STATE   LOW     // 画射灯3状态 (Pin40)
#define STAGE_000_0_PAINTING_LIGHT4_STATE   LOW     // 画射灯4状态 (Pin41)
#define STAGE_000_0_PAINTING_LIGHT5_STATE   LOW     // 画射灯5状态 (Pin42)
#define STAGE_000_0_PAINTING_LIGHT6_STATE   LOW     // 画射灯6状态 (Pin43)
#define STAGE_000_0_PAINTING_LIGHT7_STATE   LOW     // 画射灯7状态 (Pin44)
#define STAGE_000_0_PAINTING_LIGHT8_STATE   LOW     // 画射灯8状态 (Pin45)

// 提示灯带系统 (2个灯带)
#define STAGE_000_0_HINT_LED1_STATE         LOW     // 提示灯带1状态 (PinA11)
#define STAGE_000_0_HINT_LED2_STATE         LOW     // 提示灯带2状态 (PinA12)

// 蝴蝶灯谜题系统
#define STAGE_000_0_BUTTERFLY_CARD_STATE    LOW     // 门禁读卡器继电器状态 (Pin27)
#define STAGE_000_0_BUTTERFLY_LIGHT_STATE   HIGH    // 蝴蝶灯状态 (PinA15) - 初始化为HIGH
#define STAGE_000_0_AD_FAN_STATE            LOW     // 广告风扇状态 (PinA14)

// 注意：植物灯(Pin2,3,6,5)由呼吸效果动态控制，不在此静态配置中
// 注意：000_0环节作为初始化环节，不设置自动完成时间，等待服务器指令

// 001_1 环节：游戏开始环节（C101专用）- 只检测干簧管，无音频
#define STAGE_001_1_NEXT_STAGE      "001_2"  // 跳转目标环节
#define STAGE_001_1_REED_PIN        22       // 干簧管检测引脚
#define STAGE_001_1_REED_CHECK_INTERVAL 10   // 检测间隔(ms) - 高频检测，提高精度
#define STAGE_001_1_REED_DEBOUNCE_TIME 50    // 防抖时间(ms) - 持续LOW状态50ms才算触发

// ========================== 001_1环节引脚状态配置 ==========================
// 入口门系统
#define STAGE_001_1_DOOR_LOCK_STATE         LOW     // 电磁锁状态 (Pin26)
#define STAGE_001_1_DOOR_LIGHT_STATE        LOW     // 指引射灯状态 (Pin25)

// 氛围射灯系统
#define STAGE_001_1_AMBIENT_LIGHT_STATE     LOW     // 氛围射灯状态 (PinA13)

// 嘲讽按键灯光系统 (4个按键灯)
#define STAGE_001_1_TAUNT_BUTTON1_STATE     LOW     // 按键1灯光状态 (Pin30)
#define STAGE_001_1_TAUNT_BUTTON2_STATE     LOW     // 按键2灯光状态 (Pin32)
#define STAGE_001_1_TAUNT_BUTTON3_STATE     LOW     // 按键3灯光状态 (Pin34)
#define STAGE_001_1_TAUNT_BUTTON4_STATE     LOW     // 按键4灯光状态 (Pin36)

// 画灯谜题系统 (8个画射灯)
#define STAGE_001_1_PAINTING_LIGHT1_STATE   LOW     // 画射灯1状态 (Pin38)
#define STAGE_001_1_PAINTING_LIGHT2_STATE   LOW     // 画射灯2状态 (Pin39)
#define STAGE_001_1_PAINTING_LIGHT3_STATE   LOW     // 画射灯3状态 (Pin40)
#define STAGE_001_1_PAINTING_LIGHT4_STATE   LOW     // 画射灯4状态 (Pin41)
#define STAGE_001_1_PAINTING_LIGHT5_STATE   LOW     // 画射灯5状态 (Pin42)
#define STAGE_001_1_PAINTING_LIGHT6_STATE   LOW     // 画射灯6状态 (Pin43)
#define STAGE_001_1_PAINTING_LIGHT7_STATE   LOW     // 画射灯7状态 (Pin44)
#define STAGE_001_1_PAINTING_LIGHT8_STATE   LOW     // 画射灯8状态 (Pin45)

// 提示灯带系统 (2个灯带)
#define STAGE_001_1_HINT_LED1_STATE         LOW     // 提示灯带1状态 (PinA11)
#define STAGE_001_1_HINT_LED2_STATE         LOW     // 提示灯带2状态 (PinA12)

// 蝴蝶灯谜题系统
#define STAGE_001_1_BUTTERFLY_CARD_STATE    LOW     // 门禁读卡器继电器状态 (Pin27)
#define STAGE_001_1_BUTTERFLY_LIGHT_STATE   HIGH    // 蝴蝶灯状态 (PinA15) - 保持HIGH
#define STAGE_001_1_AD_FAN_STATE            LOW     // 广告风扇状态 (PinA14)

// 001_2 环节：植物灯渐灭效果（C101专用）
#define STAGE_001_2_FADE_DURATION   1500     // 1500ms内完成4个植物灯渐灭
#define STAGE_001_2_FADE_INTERVAL   50       // 50ms检查一次渐灭进度
#define STAGE_001_2_FADE_STEPS      30       // 渐灭分30步完成(1500ms/50ms=30步)

// ========================== 001_2环节引脚状态配置 ==========================
// 入口门系统
#define STAGE_001_2_DOOR_LOCK_STATE         HIGH    // 电磁锁状态 (Pin26) - 上锁
#define STAGE_001_2_DOOR_LIGHT_STATE        LOW     // 指引射灯状态 (Pin25)

// 氛围射灯系统
#define STAGE_001_2_AMBIENT_LIGHT_STATE     LOW     // 氛围射灯状态 (PinA13)

// 嘲讽按键灯光系统 (4个按键灯)
#define STAGE_001_2_TAUNT_BUTTON1_STATE     LOW     // 按键1灯光状态 (Pin30)
#define STAGE_001_2_TAUNT_BUTTON2_STATE     LOW     // 按键2灯光状态 (Pin32)
#define STAGE_001_2_TAUNT_BUTTON3_STATE     LOW     // 按键3灯光状态 (Pin34)
#define STAGE_001_2_TAUNT_BUTTON4_STATE     LOW     // 按键4灯光状态 (Pin36)

// 画灯谜题系统 (8个画射灯)
#define STAGE_001_2_PAINTING_LIGHT1_STATE   LOW     // 画射灯1状态 (Pin38)
#define STAGE_001_2_PAINTING_LIGHT2_STATE   LOW     // 画射灯2状态 (Pin39)
#define STAGE_001_2_PAINTING_LIGHT3_STATE   LOW     // 画射灯3状态 (Pin40)
#define STAGE_001_2_PAINTING_LIGHT4_STATE   LOW     // 画射灯4状态 (Pin41)
#define STAGE_001_2_PAINTING_LIGHT5_STATE   LOW     // 画射灯5状态 (Pin42)
#define STAGE_001_2_PAINTING_LIGHT6_STATE   LOW     // 画射灯6状态 (Pin43)
#define STAGE_001_2_PAINTING_LIGHT7_STATE   LOW     // 画射灯7状态 (Pin44)
#define STAGE_001_2_PAINTING_LIGHT8_STATE   LOW     // 画射灯8状态 (Pin45)

// 提示灯带系统 (2个灯带)
#define STAGE_001_2_HINT_LED1_STATE         LOW     // 提示灯带1状态 (PinA11)
#define STAGE_001_2_HINT_LED2_STATE         LOW     // 提示灯带2状态 (PinA12)

// 蝴蝶灯谜题系统
#define STAGE_001_2_BUTTERFLY_CARD_STATE    LOW     // 门禁读卡器继电器状态 (Pin27)
#define STAGE_001_2_BUTTERFLY_LIGHT_STATE   HIGH    // 蝴蝶灯状态 (PinA15) - 保持HIGH
#define STAGE_001_2_AD_FAN_STATE            LOW     // 广告风扇状态 (PinA14)

// ========================== 002_0环节：画灯谜题复杂效果配置 ==========================
// 音频配置
#define STAGE_002_0_CHANNEL1        1        // 第1播放通道
#define STAGE_002_0_SONG_ID1        2        // 第1播放歌曲ID (002号音频，播放一次)
#define STAGE_002_0_CHANNEL1_START  0        // 第1通道启动时间(ms)
#define STAGE_002_0_CHANNEL2        2        // 第2播放通道
#define STAGE_002_0_SONG_ID2        203      // 第2播放歌曲ID (203号音频，循环播放)
#define STAGE_002_0_CHANNEL2_START  0        // 第2通道启动时间(ms)
#define STAGE_002_0_CHANNEL2_LOOP   true     // 第2通道循环播放

// 环节流程配置
#define STAGE_002_0_MULTI_JUMP_TIME 30000    // 30秒时触发多环节跳转
#define STAGE_002_0_MULTI_JUMP_STAGES "006_0"  // 多环节跳转目标
#define STAGE_002_0_DURATION        60000    // 60秒默认时长
#define STAGE_002_0_NEXT_STAGE      ""       // 跳转目标环节（空字符串表示只报告完成，不跳转）

// ========================== 画灯呼吸效果时间配置 ==========================
// 呼吸效果时间表（从环节开始计时）
#define STAGE_002_0_BREATH_START_1      8118     // 画4长射灯呼吸亮开始时间(ms)
#define STAGE_002_0_BREATH_DURATION_1   1500     // 画4长射灯呼吸亮持续时间(ms)
#define STAGE_002_0_BREATH_END_1        (STAGE_002_0_BREATH_START_1 + STAGE_002_0_BREATH_DURATION_1)  // 9618ms

#define STAGE_002_0_BREATH_START_2      12009    // 画4长射灯呼吸灭开始时间(ms)
#define STAGE_002_0_BREATH_DURATION_2   1500     // 画4长射灯呼吸灭持续时间(ms)
#define STAGE_002_0_BREATH_END_2        (STAGE_002_0_BREATH_START_2 + STAGE_002_0_BREATH_DURATION_2)  // 13509ms

#define STAGE_002_0_BREATH_START_3      17205    // 画8长射灯呼吸亮开始时间(ms)
#define STAGE_002_0_BREATH_DURATION_3   1500     // 画8长射灯呼吸亮持续时间(ms)
#define STAGE_002_0_BREATH_END_3        (STAGE_002_0_BREATH_START_3 + STAGE_002_0_BREATH_DURATION_3)  // 18705ms

#define STAGE_002_0_BREATH_START_4      18705    // 画8长射灯呼吸灭开始时间(ms)
#define STAGE_002_0_BREATH_DURATION_4   1117     // 画8长射灯呼吸灭持续时间(ms) (到19822ms)
#define STAGE_002_0_BREATH_END_4        19822    // 画8长射灯呼吸灭结束时间(ms)

#define STAGE_002_0_BREATH_START_5      24741    // 画2长射灯呼吸亮开始时间(ms)
#define STAGE_002_0_BREATH_DURATION_5   1500     // 画2长射灯呼吸亮持续时间(ms)
#define STAGE_002_0_BREATH_END_5        (STAGE_002_0_BREATH_START_5 + STAGE_002_0_BREATH_DURATION_5)  // 26241ms

#define STAGE_002_0_BREATH_START_6      27495    // 画2长射灯呼吸灭开始时间(ms)
#define STAGE_002_0_BREATH_DURATION_6   1500     // 画2长射灯呼吸灭持续时间(ms)
#define STAGE_002_0_BREATH_END_6        (STAGE_002_0_BREATH_START_6 + STAGE_002_0_BREATH_DURATION_6)  // 28995ms

// 呼吸效果循环周期
#define STAGE_002_0_BREATH_CYCLE_DURATION (STAGE_002_0_BREATH_END_6 - STAGE_002_0_BREATH_START_1)  // 总循环时长

// ========================== 画灯闪烁效果时间配置 ==========================
// 闪烁效果基础参数
#define STAGE_002_0_FLASH_ON_TIME       50       // 闪烁亮持续时间(ms)
#define STAGE_002_0_FLASH_OFF_TIME      50       // 闪烁灭持续时间(ms)
#define STAGE_002_0_FLASH_CYCLES        4        // 每次闪烁循环次数
#define STAGE_002_0_FLASH_TOTAL_TIME    (STAGE_002_0_FLASH_CYCLES * (STAGE_002_0_FLASH_ON_TIME + STAGE_002_0_FLASH_OFF_TIME))  // 400ms

// 闪烁效果时间表（从环节开始计时）
#define STAGE_002_0_FLASH_START_1       22860    // 第1组闪烁开始时间(ms) - 画4长+画8长
#define STAGE_002_0_FLASH_END_1         (STAGE_002_0_FLASH_START_1 + STAGE_002_0_FLASH_TOTAL_TIME)  // 23260ms

#define STAGE_002_0_FLASH_START_2       77204    // 第2组闪烁开始时间(ms) - 画2长+画6长
#define STAGE_002_0_FLASH_END_2         (STAGE_002_0_FLASH_START_2 + STAGE_002_0_FLASH_TOTAL_TIME)  // 77604ms

#define STAGE_002_0_FLASH_START_3       125538   // 第3组闪烁开始时间(ms) - 画4长+画8长
#define STAGE_002_0_FLASH_END_3         (STAGE_002_0_FLASH_START_3 + STAGE_002_0_FLASH_TOTAL_TIME)  // 125938ms

#define STAGE_002_0_FLASH_START_4       173219   // 第4组闪烁开始时间(ms) - 画2长+画6长
#define STAGE_002_0_FLASH_END_4         (STAGE_002_0_FLASH_START_4 + STAGE_002_0_FLASH_TOTAL_TIME)  // 173619ms

// 闪烁效果循环周期（从第一次闪烁开始计算）
#define STAGE_002_0_FLASH_CYCLE_1_2     (STAGE_002_0_FLASH_START_2 - STAGE_002_0_FLASH_START_1)     // 54344ms
#define STAGE_002_0_FLASH_CYCLE_2_3     (STAGE_002_0_FLASH_START_3 - STAGE_002_0_FLASH_START_2)     // 48334ms
#define STAGE_002_0_FLASH_CYCLE_3_4     (STAGE_002_0_FLASH_START_4 - STAGE_002_0_FLASH_START_3)     // 47681ms

// 画灯引脚映射（对应C101_PAINTING_LIGHT_PINS数组索引）
#define STAGE_002_0_PAINTING_LIGHT_2_INDEX    1    // 画2长射灯 (Pin39)
#define STAGE_002_0_PAINTING_LIGHT_4_INDEX    3    // 画4长射灯 (Pin41)
#define STAGE_002_0_PAINTING_LIGHT_6_INDEX    5    // 画6长射灯 (Pin43)
#define STAGE_002_0_PAINTING_LIGHT_8_INDEX    7    // 画8长射灯 (Pin45)

// ========================== 002_0环节循环配置 ==========================
#define STAGE_002_0_FLASH_CYCLE_DURATION    180000   // 闪烁效果循环周期(ms) - 180秒
#define STAGE_002_0_AUDIO_LOOP_START        180000   // 203音频循环开始时间(ms) - 180秒后开始循环播放203音频

// ========================== 002_0环节引脚状态配置 ==========================
// 入口门系统
#define STAGE_002_0_DOOR_LOCK_STATE         HIGH    // 电磁锁状态 (Pin26) - 上锁
#define STAGE_002_0_DOOR_LIGHT_STATE        LOW     // 指引射灯状态 (Pin25)

// 氛围射灯系统
#define STAGE_002_0_AMBIENT_LIGHT_STATE     LOW     // 氛围射灯状态 (PinA13)

// 嘲讽按键灯光系统 (4个按键灯)
#define STAGE_002_0_TAUNT_BUTTON1_STATE     LOW     // 按键1灯光状态 (Pin30)
#define STAGE_002_0_TAUNT_BUTTON2_STATE     LOW     // 按键2灯光状态 (Pin32)
#define STAGE_002_0_TAUNT_BUTTON3_STATE     LOW     // 按键3灯光状态 (Pin34)
#define STAGE_002_0_TAUNT_BUTTON4_STATE     LOW     // 按键4灯光状态 (Pin36)

// 画灯谜题系统 (8个画射灯) - 002_0环节画灯由呼吸和闪烁效果动态控制
#define STAGE_002_0_PAINTING_LIGHT1_STATE   LOW     // 画射灯1状态 (Pin38) - 不参与效果
#define STAGE_002_0_PAINTING_LIGHT2_STATE   LOW     // 画射灯2状态 (Pin39) - 呼吸+闪烁效果
#define STAGE_002_0_PAINTING_LIGHT3_STATE   LOW     // 画射灯3状态 (Pin40) - 不参与效果
#define STAGE_002_0_PAINTING_LIGHT4_STATE   LOW     // 画射灯4状态 (Pin41) - 呼吸+闪烁效果
#define STAGE_002_0_PAINTING_LIGHT5_STATE   LOW     // 画射灯5状态 (Pin42) - 不参与效果
#define STAGE_002_0_PAINTING_LIGHT6_STATE   LOW     // 画射灯6状态 (Pin43) - 闪烁效果
#define STAGE_002_0_PAINTING_LIGHT7_STATE   LOW     // 画射灯7状态 (Pin44) - 不参与效果
#define STAGE_002_0_PAINTING_LIGHT8_STATE   LOW     // 画射灯8状态 (Pin45) - 呼吸+闪烁效果

// 提示灯带系统 (2个灯带)
#define STAGE_002_0_HINT_LED1_STATE         LOW     // 提示灯带1状态 (PinA11)
#define STAGE_002_0_HINT_LED2_STATE         LOW     // 提示灯带2状态 (PinA12)

// 蝴蝶灯谜题系统
#define STAGE_002_0_BUTTERFLY_CARD_STATE    LOW     // 门禁读卡器继电器状态 (Pin27)
#define STAGE_002_0_BUTTERFLY_LIGHT_STATE   HIGH    // 蝴蝶灯状态 (PinA15) - 保持HIGH
#define STAGE_002_0_AD_FAN_STATE            LOW     // 广告风扇状态 (PinA14)

// ========================== 006_0环节：嘲讽按键游戏配置 ==========================
// 游戏核心参数
#define STAGE_006_0_REQUIRED_CORRECT   4        // 需要连续正确次数
#define STAGE_006_0_SUCCESS_JUMP      "010"     // 成功后跳转目标

// 嘲讽按键呼吸效果配置（10秒循环）
#define STAGE_006_0_BREATH_CYCLE      10000     // 呼吸循环周期(ms)
// 按键1呼吸时间配置
#define STAGE_006_0_TAUNT1_BREATH_1_START    0        // 第1次呼吸亮开始
#define STAGE_006_0_TAUNT1_BREATH_1_DUR      1500     // 第1次呼吸亮持续
#define STAGE_006_0_TAUNT1_BREATH_2_START    1500     // 第1次呼吸灭开始
#define STAGE_006_0_TAUNT1_BREATH_2_DUR      1500     // 第1次呼吸灭持续
#define STAGE_006_0_TAUNT1_BREATH_3_START    5000     // 第2次呼吸亮开始
#define STAGE_006_0_TAUNT1_BREATH_3_DUR      1500     // 第2次呼吸亮持续
#define STAGE_006_0_TAUNT1_BREATH_4_START    6500     // 第2次呼吸灭开始
#define STAGE_006_0_TAUNT1_BREATH_4_DUR      1500     // 第2次呼吸灭持续

// 按键2-4使用相同配置（可独立调整）
#define STAGE_006_0_TAUNT2_BREATH_1_START    0
#define STAGE_006_0_TAUNT2_BREATH_1_DUR      1500
#define STAGE_006_0_TAUNT2_BREATH_2_START    1500
#define STAGE_006_0_TAUNT2_BREATH_2_DUR      1500
#define STAGE_006_0_TAUNT2_BREATH_3_START    5000
#define STAGE_006_0_TAUNT2_BREATH_3_DUR      1500
#define STAGE_006_0_TAUNT2_BREATH_4_START    6500
#define STAGE_006_0_TAUNT2_BREATH_4_DUR      1500

#define STAGE_006_0_TAUNT3_BREATH_1_START    0
#define STAGE_006_0_TAUNT3_BREATH_1_DUR      1500
#define STAGE_006_0_TAUNT3_BREATH_2_START    1500
#define STAGE_006_0_TAUNT3_BREATH_2_DUR      1500
#define STAGE_006_0_TAUNT3_BREATH_3_START    5000
#define STAGE_006_0_TAUNT3_BREATH_3_DUR      1500
#define STAGE_006_0_TAUNT3_BREATH_4_START    6500
#define STAGE_006_0_TAUNT3_BREATH_4_DUR      1500

#define STAGE_006_0_TAUNT4_BREATH_1_START    0
#define STAGE_006_0_TAUNT4_BREATH_1_DUR      1500
#define STAGE_006_0_TAUNT4_BREATH_2_START    1500
#define STAGE_006_0_TAUNT4_BREATH_2_DUR      1500
#define STAGE_006_0_TAUNT4_BREATH_3_START    5000
#define STAGE_006_0_TAUNT4_BREATH_3_DUR      1500
#define STAGE_006_0_TAUNT4_BREATH_4_START    6500
#define STAGE_006_0_TAUNT4_BREATH_4_DUR      1500

// 语音IO控制配置
#define STAGE_006_0_VOICE_TRIGGER_LOW_TIME  1000     // LOW电平保持时间(ms)
#define STAGE_006_0_VOICE_PLAY_MODE         0        // 播放模式：0=单次播放，1=循环播放
#define STAGE_006_0_VOICE_LOOP_INTERVAL     5000     // 循环播放间隔(ms)

// 按键防抖配置
#define STAGE_006_0_BUTTON_DEBOUNCE_TIME    50       // 按键防抖时间(ms)
#define STAGE_006_0_BUTTON_CHECK_INTERVAL   10       // 按键检测间隔(ms)

#define STAGE_006_0_VOICE_IO_1              15       // 第1个语音IO引脚 (C01MA05的IO1)
#define STAGE_006_0_VOICE_IO_2              16       // 第2个语音IO引脚 (C01MA06的IO1)
#define STAGE_006_0_VOICE_IO_3              A4       // 第3个语音IO引脚 (C01MA07的IO1)
#define STAGE_006_0_VOICE_IO_4              20       // 第4个语音IO引脚 (C01MA08的IO1)

// 错误处理配置
#define STAGE_006_0_ERROR_WAIT_TIME         3000     // 错误后等待时间(ms)
#define STAGE_006_0_ERROR_PROCESS_TIME      3000     // 错误按键处理时间(ms) - 新增
#define STAGE_006_0_PLANT_OFF_DELAY         375      // 植物灯熄灭间隔(ms)

// 正确处理配置
#define STAGE_006_0_CORRECT_PROCESS_TIME    1000     // 正确按键处理时间(ms) - 新增
#define STAGE_006_0_CORRECT_WAIT_TIME       700     // 正确后等待时间(ms) - 新增
#define STAGE_006_0_PLANT_ON_DELAY          375      // 植物灯点亮间隔(ms)
#define STAGE_006_0_PLANT_BREATH_DURATION   3000     // 植物灯呼吸周期(ms)
#define STAGE_006_0_PLANT_BREATH_ON         1500     // 植物灯呼吸亮时间(ms)
#define STAGE_006_0_PLANT_BREATH_OFF        1500     // 植物灯呼吸灭时间(ms)

// 服务器跳转映射（m%4的结果）
#define STAGE_006_0_JUMP_MOD_0              "211"    // m%4=0时的跳转结果
#define STAGE_006_0_JUMP_MOD_1              "213"    // m%4=1时的跳转结果
#define STAGE_006_0_JUMP_MOD_2              "212"    // m%4=2时的跳转结果
#define STAGE_006_0_JUMP_MOD_3              "214"    // m%4=3时的跳转结果

// 错误跳转映射（基于总数m）
#define STAGE_006_0_ERROR_JUMP_1            "3"      // m=1,2时的错误跳转
#define STAGE_006_0_ERROR_JUMP_2            "4"      // m=3,4时的错误跳转
#define STAGE_006_0_ERROR_JUMP_3            "5"      // m=5,6时的错误跳转

// ========================== 006_0环节引脚状态配置 ==========================
// 入口门系统
#define STAGE_006_0_DOOR_LOCK_STATE         HIGH    // 电磁锁状态 (Pin26) - 保持上锁
#define STAGE_006_0_DOOR_LIGHT_STATE        LOW     // 指引射灯状态 (Pin25)

// 氛围射灯系统
#define STAGE_006_0_AMBIENT_LIGHT_STATE     LOW     // 氛围射灯状态 (PinA13)

// 嘲讽按键灯光系统 - 由呼吸效果动态控制
#define STAGE_006_0_TAUNT_BUTTON1_STATE     LOW     // 按键1灯光状态 (Pin30)
#define STAGE_006_0_TAUNT_BUTTON2_STATE     LOW     // 按键2灯光状态 (Pin32)
#define STAGE_006_0_TAUNT_BUTTON3_STATE     LOW     // 按键3灯光状态 (Pin34)
#define STAGE_006_0_TAUNT_BUTTON4_STATE     LOW     // 按键4灯光状态 (Pin36)

// 画灯谜题系统
#define STAGE_006_0_PAINTING_LIGHT1_STATE   LOW     // 画射灯1状态 (Pin38)
#define STAGE_006_0_PAINTING_LIGHT2_STATE   LOW     // 画射灯2状态 (Pin39)
#define STAGE_006_0_PAINTING_LIGHT3_STATE   LOW     // 画射灯3状态 (Pin40)
#define STAGE_006_0_PAINTING_LIGHT4_STATE   LOW     // 画射灯4状态 (Pin41)
#define STAGE_006_0_PAINTING_LIGHT5_STATE   LOW     // 画射灯5状态 (Pin42)
#define STAGE_006_0_PAINTING_LIGHT6_STATE   LOW     // 画射灯6状态 (Pin43)
#define STAGE_006_0_PAINTING_LIGHT7_STATE   LOW     // 画射灯7状态 (Pin44)
#define STAGE_006_0_PAINTING_LIGHT8_STATE   LOW     // 画射灯8状态 (Pin45)

// 提示灯带系统
#define STAGE_006_0_HINT_LED1_STATE         LOW     // 提示灯带1状态 (PinA11)
#define STAGE_006_0_HINT_LED2_STATE         LOW     // 提示灯带2状态 (PinA12)

// 蝴蝶灯谜题系统
#define STAGE_006_0_BUTTERFLY_CARD_STATE    LOW     // 门禁读卡器继电器状态 (Pin27)
#define STAGE_006_0_BUTTERFLY_LIGHT_STATE   HIGH    // 蝴蝶灯状态 (PinA15) - 保持HIGH
#define STAGE_006_0_AD_FAN_STATE            LOW     // 广告风扇状态 (PinA14)

// ========================== 统一引脚状态管理系统 ==========================

// 语音IO引脚状态结构
struct VoiceIOState {
    int pin;                    // 引脚号
    bool desiredState;          // 期望状态：true=HIGH, false=LOW
    bool currentState;          // 当前实际状态
    unsigned long changeTime;   // 状态改变时间
    unsigned long duration;     // 如果是临时状态，持续时间（0表示永久）
    bool needsUpdate;           // 是否需要更新硬件状态
};

// 最大管理的引脚数量
#define MAX_MANAGED_PINS 35

// 全局引脚状态管理器
class UnifiedPinManager {
private:
    VoiceIOState managedPins[MAX_MANAGED_PINS];
    int managedPinCount;
    
public:
    UnifiedPinManager() : managedPinCount(0) {}
    
    // 注册需要管理的引脚
    void registerPin(int pin, bool initialState = HIGH);
    
    // 设置引脚状态（立即生效）
    void setPinState(int pin, bool state);
    
    // 设置引脚临时状态（指定时间后自动恢复）
    void setPinTemporaryState(int pin, bool tempState, unsigned long duration, bool restoreState);
    
    // 检查引脚是否被PWM控制（避免冲突）
    bool isPinPWMControlled(int pin);
    
    // 统一更新所有引脚状态
    void updateAllPins();
    
    // 获取引脚当前状态
    bool getPinState(int pin);
    
    // 调试：打印所有引脚状态
    void printPinStates();
    
private:
    // 查找引脚索引
    int findPinIndex(int pin);
    
    // 实际更新单个引脚
    void updateSinglePin(int index);
};

// 全局引脚管理器实例
extern UnifiedPinManager pinManager;

class GameFlowManager {
private:
    // 环节状态结构体
    struct StageState {
        String stageId;              // 环节ID
        unsigned long startTime;     // 开始时间
        bool running;                // 是否运行中
        bool jumpRequested;          // 是否已请求跳转
        
        // 环节特定状态（使用union节省内存）
        union {
            struct {  // 000_0环节状态
                bool channelStarted;
                unsigned long lastCheckTime;
                // 植物灯顺序呼吸效果状态
                int currentLightIndex;       // 当前亮起的灯索引(0-3)
                unsigned long lightCycleStartTime;  // 当前循环开始时间
                bool lightEffectStarted;     // 灯光效果是否已启动
            } stage000;
            
            struct {  // 001_1环节状态
                bool channelStarted;
                unsigned long lastCheckTime;
                unsigned long lastReedCheckTime;  // 上次干簧管检测时间
                bool lastReedState;               // 上次干簧管状态
                bool reedTriggered;               // 干簧管是否已触发跳转
                int lastLightIndex;               // 上次亮起的植物灯索引(-1表示未初始化)
                // 防抖相关状态
                unsigned long lowStateStartTime;  // 开始LOW状态的时间(0表示未开始)
                bool debounceComplete;            // 防抖是否完成
            } stage001_1;
            
            struct {  // 001_2环节状态
                bool fadeStarted;             // 渐灭效果是否已开始
                unsigned long lastFadeUpdate; // 上次渐灭更新时间
                int currentFadeStep;          // 当前渐灭步骤(0-30)
                bool fadeComplete;            // 渐灭是否完成
            } stage001_2;
            
            struct {  // 002_0环节状态
                bool channel1Started;
                bool channel2Started;
                bool multiJumpTriggered;  // 多环节跳转是否已触发
                // 呼吸效果状态
                unsigned long breathEffectStartTime;  // 呼吸效果开始时间
                int currentBreathStep;                 // 当前呼吸步骤(0-5)
                bool breathEffectActive;               // 呼吸效果是否激活
                // 闪烁效果状态
                unsigned long flashEffectStartTime;   // 闪烁效果开始时间
                int currentFlashGroup;                 // 当前闪烁组(0-3)
                int currentFlashCycle;                 // 当前闪烁周期内的循环次数
                bool flashEffectActive;                // 闪烁效果是否激活
                bool flashState;                       // 当前闪烁状态(true=亮, false=灭)
                unsigned long lastFlashToggle;        // 上次闪烁切换时间
            } stage002;
            
            struct {  // 006_0环节状态
                // 内部状态机
                enum SubState {
                    SUB_INIT,           // 初始化
                    SUB_WAITING_INPUT,  // 等待输入
                    SUB_CORRECT,        // 正确处理
                    SUB_ERROR,          // 错误处理
                    SUB_ERROR_WAIT,     // 错误等待
                    SUB_SUCCESS         // 成功完成
                } subState;
                
                // 游戏核心状态 (必要)
                int totalCount;                // 总计数器m
                int correctCount;              // 正确计数器
                int currentCorrectButton;      // 当前正确的按键(1-4)
                int pressedButton;             // 玩家按下的按键(1-4)
                bool buttonPressed;            // 是否有按键被按下
                
                // 语音控制状态 (必要)
                bool voiceTriggered;           // 语音是否已触发
                unsigned long voiceTriggerTime; // 语音触发时间
                bool voicePlayedOnce;          // 语音是否已播放过一次
                unsigned long lastVoiceTime;   // 上次语音播放时间
                
                // 按键防抖状态 (必要)
                bool buttonDebouncing;         // 是否正在防抖
                int debouncingButton;          // 正在防抖的按键索引(0-3)
                unsigned long debounceStartTime; // 防抖开始时间
                int lastButtonStates[4];       // 上次按键状态记录
                
                // 时序控制状态 (必要)
                unsigned long errorStartTime;  // 错误处理开始时间
                unsigned long correctStartTime; // 正确处理开始时间
                bool isCorrectWait;             // 是否是正确处理后的等待状态
                
                // 植物灯状态 (必要)
                bool plantLightStates[4];      // 植物灯状态记录
                bool plantBreathActive;        // 植物灯时序呼吸是否激活
                unsigned long plantBreathStartTime; // 植物灯时序呼吸开始时间
                int plantBreathIndex;          // 植物灯时序呼吸索引
            } stage006;
        } state;
        
        // 构造函数
        StageState() : stageId(""), startTime(0), running(false), jumpRequested(false) {
            memset(&state, 0, sizeof(state));
        }
    };
    
    // 并行环节数组
    StageState stages[MAX_PARALLEL_STAGES];
    int activeStageCount;            // 当前活跃环节数
    bool globalStopped;              // 全局停止标志
    
    // 兼容旧接口的当前环节引用
    String currentStageId;           // 当前环节ID（指向第一个活跃环节）
    unsigned long stageStartTime;    // 环节开始时间（指向第一个活跃环节）
    bool stageRunning;               // 环节是否运行中（任意环节运行即为true）
    bool jumpRequested;              // 是否已请求跳转（任意环节请求即为true）
    
    // 紧急开门功能变量
    unsigned long emergencyUnlockStartTime;  // 紧急解锁开始时间
    bool emergencyUnlockActive;              // 紧急解锁状态
    bool lastCardReaderState;                // 上次读卡器状态
    
    // 查找环节索引
    int findStageIndex(const String& stageId);
    int findEmptySlot();
    
    // 环节完成通知
    void notifyStageComplete(const String& currentStep, const String& nextStep, unsigned long duration);
    void notifyStageComplete(const String& currentStep, unsigned long duration);  // 重载版本，不指定下一步
    
    // C101音频环节定义方法（带索引参数）
    void updateStep000(int index);            // 更新000_0环节
    void updateStep001_1(int index);          // 更新001_1环节
    void updateStep001_2(int index);          // 更新001_2环节
    void updateStep002(int index);            // 更新002_0环节
    void updateStep006(int index);            // 更新006_0环节
    
    // 工具方法
    String normalizeStageId(const String& stageId);  // 标准化环节ID格式
    void updateCompatibilityVars();                  // 更新兼容性变量
    
    // 000_0环节引脚配置方法
    void apply000StageConfig();                      // 应用000_0环节引脚配置
    
    // 音量管理方法
    void initializeAllVolumes();                     // 初始化所有通道音量为默认值
    void resetChannelVolume(int channel);            // 重置指定通道音量为默认值
    void resetAllVolumes();                          // 重置所有通道音量为默认值
    
    // 紧急开门功能方法
    void updateEmergencyDoorControl();       // 更新紧急开门控制

public:
    // ========================== 构造和初始化 ==========================
    GameFlowManager();
    // 初始化
    bool begin();
    
    // ========================== 环节控制 ==========================
    bool startStage(const String& stageId);         // 启动指定环节
    bool startMultipleStages(const String& stageIds); // 启动多个环节（逗号分隔）
    void stopStage(const String& stageId);          // 停止指定环节
    void stopCurrentStage();                        // 停止当前环节（兼容旧接口）
    void stopAllStages();                           // 停止所有环节
    
    // ========================== 状态查询 ==========================
    const String& getCurrentStageId() const;        // 获取当前环节ID（兼容旧接口）
    bool isStageRunning() const;                    // 是否有环节在运行
    bool isStageRunning(const String& stageId);     // 指定环节是否在运行
    unsigned long getStageElapsedTime() const;      // 获取环节运行时间（兼容旧接口）
    unsigned long getStageElapsedTime(const String& stageId); // 获取指定环节运行时间
    int getActiveStageCount() const;                // 获取活跃环节数
    void getActiveStages(String stages[], int maxCount); // 获取所有活跃环节ID
    
    // ========================== 环节列表 ==========================
    bool isValidStageId(const String& stageId);     // 检查环节ID是否有效
    void printAvailableStages();                    // 打印所有可用环节
    
    // ========================== 更新和调试功能 ==========================
    void update();                                  // 主更新循环
    void updateStage(int index);                    // 更新单个环节
    void checkEmergencyDoorControl();               // 检查紧急开门功能
    void printStatus();                             // 打印当前状态
    
    // ========================== 环节跳转请求 ==========================
    void requestStageJump(const String& nextStage); // 请求跳转到指定环节（通过服务器）
    void requestMultiStageJump(const String& currentStep, const String& nextSteps); // 请求跳转到多个环节
    
    // ========================== 紧急开门功能 ==========================
    void initEmergencyDoorControl();         // 初始化紧急开门功能
    bool isEmergencyUnlockActive() const;    // 获取紧急解锁状态
    
    // ========================== 门锁和灯光控制 ==========================
    void resetDoorAndLightState();           // 重置门锁和灯光状态（供外部调用）
};

// 全局实例声明
extern GameFlowManager gameFlowManager;

#endif // GAME_FLOW_MANAGER_H 