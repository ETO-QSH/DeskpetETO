#pragma once

#include <map>
#include <string>
#include <vector>

#include "json.hpp"

using VkTable = std::map<int, std::string>;

// 获取主映射表
const VkTable& getMainVkTable();

// 获取自定义分表
const VkTable& mouseVkTables();
const VkTable& imeVkTables();
const VkTable& directionVkTables();
const VkTable& mainNumVkTables();
const VkTable& alphabetVkTables();
const VkTable& liteNumVkTables();
const VkTable& liteNumOpVkTables();
const VkTable& funcNumVkTables();
const VkTable& highFuncVkTables();
const VkTable& midFuncVkTables();
const VkTable& modifyVkTables();
const VkTable& browserVkTables();
const VkTable& appVkTables();
const VkTable& interVkTables();
const VkTable& otherVkTables();

// 组合分表
VkTable combineVkTables(const std::vector<const VkTable*>& tables);

// 主接口：根据vkCode和映射表获取字符串
std::string vkCodeToString(int vkCode, const VkTable& table = getMainVkTable());

// 初始化主映射表（从json配置），main.cpp只需调用一次
void initVkMainTableFromJson(const nlohmann::json& config);
