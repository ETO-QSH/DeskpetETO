#pragma once

#include <functional>
#include <string>
#include <vector>

#include "right_click_menu.h"

// 菜单项数据结构
struct MenuItemData {
    std::string text;
    std::string iconPath;
    std::string value; // 用于回调传参
};

// 回调类型
using SkinCallback = std::function<void(const std::string&)>;
using ModelCallback = std::function<void(const std::string&)>;
using SimpleCallback = std::function<void()>;

// 构建菜单
MenuModel buildMenuModel(
    const std::vector<MenuItemData>& skinList,
    SkinCallback skinCb,
    const std::vector<MenuItemData>& modelList,
    ModelCallback modelCb,
    SimpleCallback onTop,
    SimpleCallback onTopOff,
    SimpleCallback onStick,
    SimpleCallback onStickOff,
    std::function<void(int)> onStatus,
    SimpleCallback onPack,
    SimpleCallback onTrans,
    SimpleCallback onSetting,
    SimpleCallback onPlaceholder,
    SimpleCallback onQuit
);

// 默认菜单模型初始化函数声明
MenuModel getDefaultMenuModel();

std::vector<MenuEntry> getCurrentSkinEntries(SkinCallback skinCb);
std::vector<MenuEntry> getCurrentModelEntries(ModelCallback modelCb);

// 全局回调（供菜单弹出时使用）
extern SkinCallback g_skinCallback;
extern ModelCallback g_modelCallback;
