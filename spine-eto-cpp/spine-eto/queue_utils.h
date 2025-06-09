#pragma once

#include <map>
#include <string>
#include <vector>

// 结构体：活跃参数
struct ActiveParams {
    float specialRatio;
    float relaxToMoveRatio;
};

// 获取活跃参数
ActiveParams getActiveParams(int activeLevel);

// 生成动画队列（带Turn机制，Turn计数全局持久化）
std::vector<std::string> generateRandomAnimQueue(
    float relaxToMoveRatio,
    float specialRatio,
    int totalCount,
    const std::map<std::string, float>& animDurations,
    const std::string& specialName = "Special"
);

// 以prefix为头（继承Turn计数），生成count个动画（带Turn机制）
std::vector<std::string> generateRandomAnimQueueWithPrefix(
    const std::vector<std::string>& prefix,
    float relaxToMoveRatio,
    float specialRatio,
    int count,
    const std::map<std::string, float>& animDurations,
    const std::string& specialName = "Special"
);

// Turn计数管理
void setTurnMissCount(int count);
int getTurnMissCount();
