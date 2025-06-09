#include <chrono>
#include <cstdio>
#include <Windows.h>

#include "console_colors.h"
#include "menu_model_utils.h"
#include "spine_animation.h"

// 声明全局退出标志
bool g_appShouldExit = false;

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

    // 切换皮肤子菜单
    std::vector<MenuEntry> skinEntries;
    for (const auto& item : skinList) {
        skinEntries.push_back(MenuEntry::Action(item.text, item.iconPath, [skinCb, v = item.value]() {
            skinCb(v);
        }));
    }
    model.addSubMenu("切换皮肤", "./source/icon/skin.png", skinEntries);

    // 切换模型子菜单
    std::vector<MenuEntry> modelEntries;
    for (const auto& item : modelList) {
        modelEntries.push_back(MenuEntry::Action(item.text, item.iconPath, [modelCb, v = item.value]() {
            modelCb(v);
        }));
    }
    model.addSubMenu("切换模型", "./source/icon/armor.png", modelEntries);

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

// 新增：默认菜单模型初始化函数，main.cpp 只需调用此函数即可
MenuModel getDefaultMenuModel() {
    std::vector<MenuItemData> skinList = {
        { "弃土花开", "./source/image/弃土花开.png", "弃土花开" },
        { "寰宇独奏", "./source/image/寰宇独奏.png", "寰宇独奏" },
        { "至高判决", "./source/image/至高判决.png", "至高判决" }
    };
    std::vector<MenuItemData> modelList = {
        { "寄自奥格尼斯科", "./source/image/寄自奥格尼斯科.png", "寄自奥格尼斯科" },
        { "夏卉 FA210", "./source/image/夏卉 FA210.png", "夏卉 FA210" },
        { "远行前的野餐", "./source/image/远行前的野餐.png", "远行前的野餐" }
    };
    return buildMenuModel(
        skinList,
        [](const std::string& v) {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 切换皮肤: %s" CONSOLE_RESET "\n", v.c_str());
        },
        modelList,
        [](const std::string& v) {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 切换模型: %s" CONSOLE_RESET "\n", v.c_str());
        },
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
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 位置锁定: 关" CONSOLE_RESET "\n");
            extern void setWalkEnabled(bool);
            setWalkEnabled(true);
            extern void setGravityEnabled(bool);
            setGravityEnabled(true);
        },
        [](int state){
            extern SpineAnimation* animSystem;
            switch (state) {
                case 1:
                    printf(CONSOLE_BRIGHT_GREEN "[MENU] 目前状态: 坐" CONSOLE_RESET "\n");
                    if (animSystem) {
                        animSystem->playTemp("Sit", true);
                    }
                    break;
                case 2:
                    printf(CONSOLE_BRIGHT_GREEN "[MENU] 目前状态: 卧" CONSOLE_RESET "\n");
                    if (animSystem) {
                        animSystem->playTemp("Sleep", true);
                    }
                    break;
                default:
                    printf(CONSOLE_BRIGHT_GREEN "[MENU] 目前状态: 行" CONSOLE_RESET "\n");
                    if (animSystem) {
                        animSystem->playTemp("Move");
                    }
                    break;
            }
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 桌宠收纳" CONSOLE_RESET "\n");
            extern HWND hwnd;
            ShowWindow(hwnd, SW_HIDE);
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 交互透明" CONSOLE_RESET "\n");
            extern HWND hwnd;
            LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
            exStyle |= WS_EX_TRANSPARENT;
            SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
            // 设置窗口整体50%透明
            SetLayeredWindowAttributes(hwnd, 0, static_cast<BYTE>(255 * 0.5), LWA_ALPHA);
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 应用设置" CONSOLE_RESET "\n");
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 占位符喵" CONSOLE_RESET "\n");
        },
        [] {
            printf(CONSOLE_BRIGHT_GREEN "[MENU] 销毁退出" CONSOLE_RESET "\n");
            g_appShouldExit = true;
        }
    );
}
