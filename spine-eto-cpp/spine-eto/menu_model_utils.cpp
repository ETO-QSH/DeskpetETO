#include <chrono>
#include <cstdio>
#include <functional>
#include <windows.h>
#include <vector>
#include <string>

#include "console_colors.h"
#include "menu_model_utils.h"
#include "spine_animation.h"
#include "spine_win_utils.h"
#include "window_physics.h"

// 引入json头文件
#include "../dependencies/json.hpp"

extern nlohmann::json g_modelDatabase;

// 全局辉光信号变量
bool g_showGlowEffect = false;
// 全局交互半透明信号变量
bool g_showHalfAlpha = false;

// 声明全局菜单数据
std::vector<MenuItemData> g_skinList;
std::vector<MenuItemData> g_modelList;

// 刷新菜单数据
void updateMenuLists() {
    g_skinList.clear();
    g_modelList.clear();

    // 获取default皮肤和模型
    if (!g_modelDatabase.contains("default") || !g_modelDatabase.contains("library")) return;
    const auto& def = g_modelDatabase["default"];
    if (!def.is_array() || def.size() < 2) return;
    std::string currentSkin = def[0];
    std::string currentModel = def[1];

    const auto& lib = g_modelDatabase["library"];

    // 皮肤列表（除当前皮肤）
    for (auto it = lib.begin(); it != lib.end(); ++it) {
        std::string skinName = it.key();
        if (skinName == currentSkin) continue;
        std::string headPath;
        if (it.value().contains("head")) {
            headPath = it.value()["head"].get<std::string>();
        }
        g_skinList.push_back({ skinName, headPath, skinName });
    }

    // 模型列表（当前皮肤下所有模型，除head、非对象、当前模型）
    if (lib.contains(currentSkin)) {
        const auto& skinObj = lib[currentSkin];
        for (auto it = skinObj.begin(); it != skinObj.end(); ++it) {
            std::string modelName = it.key();
            if (modelName == "head" || modelName == currentModel) continue;
            if (!it.value().is_object()) continue;
            std::string pngPath;
            if (it.value().contains("png")) {
                pngPath = it.value()["png"].get<std::string>();
            }
            g_modelList.push_back({ modelName, pngPath, modelName });
        }
    }
}

// 声明全局退出标志
bool g_appShouldExit = false;

// 动态生成皮肤子菜单项
std::vector<MenuEntry> getCurrentSkinEntries(SkinCallback skinCb) {
    updateMenuLists();
    std::vector<MenuEntry> skinEntries;
    for (const auto& item : g_skinList) {
        // 只生成 Action 类型，callback 必须有效
        skinEntries.push_back(MenuEntry::Action(item.text, item.iconPath, [skinCb, v = item.value]() {
            if (skinCb) skinCb(v);
        }));
    }
    return skinEntries;
}

// 动态生成模型子菜单项
std::vector<MenuEntry> getCurrentModelEntries(ModelCallback modelCb) {
    updateMenuLists();
    std::vector<MenuEntry> modelEntries;
    for (const auto& item : g_modelList) {
        // 只生成 Action 类型，callback 必须有效
        modelEntries.push_back(MenuEntry::Action(item.text, item.iconPath, [modelCb, v = item.value]() {
            if (modelCb) modelCb(v);
        }));
    }
    return modelEntries;
}

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
) {
    MenuModel model;

    // 注意：这里的skinList/modelList参数仅用于初始化，实际弹出菜单时应动态刷新
    model.addSubMenu("切换皮肤", "./source/icon/skin.png", getCurrentSkinEntries(skinCb));
    model.addSubMenu("切换模型", "./source/icon/armor.png", getCurrentModelEntries(modelCb));

    model.addSeparator();

    // 窗口置顶
    model.getEntries().push_back(MenuEntry::Toggle("窗口置顶", "./source/icon/top.png",
        1, [onTop, onTopOff](const int& state) {
            if (state) onTop();
            else onTopOff();
        }));

    // 位置锁定
    model.getEntries().push_back(MenuEntry::Toggle("位置锁定", "./source/icon/stick.png",
        0, [onStick, onStickOff](const int& state) {
            if (state) onStick();
            else onStickOff();
        }));

    // 状态切换
    model.getEntries().push_back(MenuEntry::ToggleTri("状态切换", "./source/icon/break.png",
        0, onStatus));

    model.addSeparator();
    model.addAction("桌宠收纳", "./source/icon/pack.png", onPack);
    model.addAction("交互透明", "./source/icon/trans.png", onTrans);
    model.addAction("应用设置", "./source/icon/control.png", onSetting);
    model.addAction("占位符喵", "./source/icon/setting.png", onPlaceholder);

    model.addSeparator();
    model.addAction("销毁退出", "./source/icon/quit.png", onQuit);

    return model;
}

// 工具函数：切换皮肤并自动选择模型
void switchSkin(const std::string& skinName) {
    if (!g_modelDatabase.contains("library")) return;
    const auto& lib = g_modelDatabase["library"];
    if (!lib.contains(skinName)) return;
    const auto& skinObj = lib[skinName];

    // 1. 同名
    if (skinObj.contains(skinName)) {
        g_modelDatabase["default"][0] = skinName;
        g_modelDatabase["default"][1] = skinName;
        return;
    }
    // 2. 默认
    if (skinObj.contains("默认")) {
        g_modelDatabase["default"][0] = skinName;
        g_modelDatabase["default"][1] = "默认";
        return;
    }
    // 3. 基建
    if (skinObj.contains("基建")) {
        g_modelDatabase["default"][0] = skinName;
        g_modelDatabase["default"][1] = "基建";
        return;
    }
    // 4. 正面
    if (skinObj.contains("正面")) {
        g_modelDatabase["default"][0] = skinName;
        g_modelDatabase["default"][1] = "正面";
        return;
    }
    // 5. 取第一个模型（排除head）
    for (auto it = skinObj.begin(); it != skinObj.end(); ++it) {
        if (it.key() == "head") continue;
        g_modelDatabase["default"][0] = skinName;
        g_modelDatabase["default"][1] = it.key();
        return;
    }
}

// 工具函数：切换模型
void switchModel(const std::string& modelName) {
    g_modelDatabase["default"][1] = modelName;
}

// 默认菜单模型初始化函数，main.cpp 只需调用此函数即可
MenuModel getDefaultMenuModel() {
    // 初始化时刷新一次
    updateMenuLists();

    // 赋值全局回调
    g_skinCallback = [](const std::string& v) {
        printf(CONSOLE_BRIGHT_GREEN "[MENU] 切换皮肤: %s" CONSOLE_RESET "\n", v.c_str());
        switchSkin(v);
        updateMenuLists();
        reinitSpineModel();
    };
    g_modelCallback = [](const std::string& v) {
        printf(CONSOLE_BRIGHT_GREEN "[MENU] 切换模型: %s" CONSOLE_RESET "\n", v.c_str());
        switchModel(v);
        updateMenuLists();
        reinitSpineModel();
    };

    return buildMenuModel(
        g_skinList,
        g_skinCallback,
        g_modelList,
        g_modelCallback,
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 窗口置顶: 开" CONSOLE_RESET "\n");
            extern HWND hwnd;
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 窗口置顶: 关" CONSOLE_RESET "\n");
            extern HWND hwnd;
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 位置锁定: 开" CONSOLE_RESET "\n");
            extern void setWalkEnabled(bool);
            setWalkEnabled(false);
            extern void setGravityEnabled(bool);
            setGravityEnabled(false);
            extern WindowPhysicsState g_windowPhysicsState;
            g_windowPhysicsState.locked = true;
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 位置锁定: 关" CONSOLE_RESET "\n");
            extern void setWalkEnabled(bool);
            setWalkEnabled(true);
            extern void setGravityEnabled(bool);
            setGravityEnabled(true);
            extern WindowPhysicsState g_windowPhysicsState;
            g_windowPhysicsState.locked = false;
            g_windowPhysicsState.vx = 0.0f;
            g_windowPhysicsState.vy = 0.0f;
        },
        [](int state){
            extern SpineAnimation* animSystem;
            switch (state) {
                case 1:
                    printf(CONSOLE_BRIGHT_GREEN "[MENU] 目前状态: 坐" CONSOLE_RESET "\n");
                    if (animSystem) {
                        animSystem->playTemp("Sit", true);
                    } break;
                case 2:
                    printf(CONSOLE_BRIGHT_GREEN "[MENU] 目前状态: 卧" CONSOLE_RESET "\n");
                    if (animSystem) {
                        animSystem->playTemp("Sleep", true);
                    } break;
                default:
                    printf(CONSOLE_BRIGHT_GREEN "[MENU] 目前状态: 行" CONSOLE_RESET "\n");
                    if (animSystem) {
                        if (animSystem->isPlayingTemp()) {
                            animSystem->playTemp("Interact");
                        }
                    }
            }
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 桌宠收纳" CONSOLE_RESET "\n");
            extern HWND hwnd;
            ShowWindow(hwnd, SW_HIDE);
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 交互透明" CONSOLE_RESET "\n");
            // 设置窗口为交互穿透（全窗口点击穿透）
            extern HWND hwnd;
            LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
            exStyle |= WS_EX_TRANSPARENT;
            SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
            // 设置半透明信号，主循环渲染时处理
            g_showHalfAlpha = true;
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 应用设置" CONSOLE_RESET "\n");
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 占位符喵" CONSOLE_RESET "\n");
            extern SpineAnimation* animSystem;
            if (animSystem) {
                animSystem->playTemp("Interact");
            }
            g_showGlowEffect = true;
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 销毁退出" CONSOLE_RESET "\n");
            g_appShouldExit = true;
        }
    );
}

// 全局回调（供菜单弹出时使用）
SkinCallback g_skinCallback;
ModelCallback g_modelCallback;
