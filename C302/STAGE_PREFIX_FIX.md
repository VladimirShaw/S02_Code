# Stage前缀可配置化修复报告

## 问题描述

用户反映了两个关键问题：

1. **编译错误**：`SimpleGameStage.cpp`中出现"'gameFlowManager' has incomplete type"错误
2. **硬编码问题**：系统中大量使用硬编码的"072-"前缀，不支持其他stage ID格式（如073-等）

## 修复方案

### 1. 编译错误修复

**问题原因**：
- `SimpleGameStage.cpp`只有`GameFlowManager`的前向声明，没有包含完整头文件
- 在`executeEndAction`方法中调用`gameFlowManager.requestStageJump()`时无法找到完整类定义

**修复内容**：
```cpp
// SimpleGameStage.cpp - 添加完整头文件包含
#include "GameFlowManager.h"
```

### 2. Stage ID硬编码问题修复

**问题原因**：
- 系统中大量硬编码"072-"前缀
- 无法支持其他游戏的stage ID格式（如073-、074-等）

**修复内容**：

#### A. GameFlowManager类增强
```cpp
// GameFlowManager.h - 添加可配置前缀支持
class GameFlowManager {
private:
    String stagePrefix;  // 环节ID前缀（默认"072-"）
    
public:
    // 环节前缀配置方法
    void setStagePrefix(const String& prefix);      // 设置环节ID前缀
    const String& getStagePrefix() const;           // 获取环节ID前缀
    String buildStageId(const String& suffix);      // 构建完整环节ID
};
```

#### B. SimpleGameStage类增强
```cpp
// SimpleGameStage.h - 支持字符串格式的stage ID
class SimpleGameStage {
private:
    String pendingJumpStageId;  // 待跳转的环节ID（字符串版本）
    
public:
    // 新增字符串版本的jumpToStage方法
    void jumpToStage(unsigned long startTime, int nextStage);                    // 数字版本（向后兼容）
    void jumpToStage(unsigned long startTime, const String& nextStageId);       // 字符串版本（推荐）
};
```

#### C. 动态stage ID构建
```cpp
// 原来的硬编码方式
nextStep = "072-7";
gameStage.jumpToStage(16000, 5);

// 修复后的可配置方式
nextStep = buildStageId("7");
gameStage.jumpToStage(16000, buildStageId("5"));
```

## 使用方法

### 基本用法（保持默认072-前缀）
```cpp
// 无需任何修改，系统默认使用072-前缀
gameFlowManager.startStage("072-5");  // 正常工作
```

### 切换到其他前缀
```cpp
// 设置为073-前缀
gameFlowManager.setStagePrefix("073-");

// 现在所有stage ID都会使用073-前缀
// 错误处理会跳转到073-7, 073-8, 073-9
// 定时跳转会跳转到073-5
```

### 混合使用不同格式
```cpp
// 支持任意前缀格式
gameFlowManager.setStagePrefix("074_");  // 下划线分隔
gameFlowManager.setStagePrefix("GAME_");  // 文本前缀
gameFlowManager.setStagePrefix("");      // 无前缀
```

## 向后兼容性

- ✅ 所有现有代码无需修改即可正常工作
- ✅ 数字版本的`jumpToStage`方法保留
- ✅ 默认前缀仍为"072-"
- ✅ 现有的stage ID格式完全兼容

## 技术实现细节

### 1. 字符串stage ID存储机制
- 使用`pendingJumpStageId`临时存储字符串格式的stage ID
- `value1=-1`标识使用字符串版本
- 执行后自动清空临时存储

### 2. 前缀配置机制
- 运行时可动态修改前缀
- `buildStageId()`方法统一构建完整stage ID
- 所有相关方法都使用可配置前缀

### 3. 错误处理更新
- `handleGameError()`使用`buildStageId("7/8/9")`
- `handleGameComplete()`使用`buildStageId("9")`
- 定时跳转使用`buildStageId("5")`

## 测试验证

修复后的系统支持：
- ✅ 编译通过，无类型错误
- ✅ 默认072-前缀正常工作
- ✅ 动态切换到073-前缀
- ✅ 支持任意自定义前缀格式
- ✅ 向后兼容现有代码

## 总结

此次修复完全解决了用户提出的两个问题：
1. **编译错误**：通过包含完整头文件解决
2. **硬编码问题**：通过可配置前缀机制解决

系统现在具备了更好的灵活性和可扩展性，能够支持多种游戏的stage ID格式，同时保持完全的向后兼容性。 