#include <spine/spine-sfml.h>
#include <SFML/Graphics.hpp>

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>

#include "spine-eto/console_colors.h"
#include "spine-eto/menu_model_utils.h"
#include "spine-eto/mouse_events.h"
#include "spine-eto/right_click_menu.h"
#include "spine-eto/spine_win_utils.h"
#include "spine-eto/subtitle_window.h"
#include "spine-eto/window_physics.h"
#include "spine-eto/vk_code_2_string.h"

#include "json.hpp"

using namespace spine;

// 声明全局物理状态，供 mouse_events.cpp 使用
WindowPhysicsState g_windowPhysicsState;
// 声明全局菜单指针，供 mouse_events.cpp 使用
MenuWidgetWithHide* g_contextMenu = nullptr;
// 声明全局主窗口，供 mouse_events.cpp 使用
sf::RenderWindow* g_mainWindow = nullptr;
// 声明全局工作区，供 mouse_events.cpp 使用
WindowWorkArea g_workArea;

// 声明全局退出标志
extern bool g_appShouldExit;
// 声明全局辉光信号变量
extern bool g_showGlowEffect;
// 声明全局交互半透明信号变量
extern bool g_showHalfAlpha;

// 声明全局字幕特殊处理变量
bool g_enableSpecialAlpha = false;

// 全局数据库
nlohmann::json g_initDatabase;
nlohmann::json g_modelDatabase;

// 解析 #RRGGBB 或 #RRGGBBAA 字符串为 sf::Color
sf::Color parseHexColor(const std::string& hex) {
    unsigned int r = 0, g = 0, b = 0, a = 255;
    if (hex.size() == 7 && hex[0] == '#') {
        std::istringstream(hex.substr(1, 2)) >> std::hex >> r;
        std::istringstream(hex.substr(3, 2)) >> std::hex >> g;
        std::istringstream(hex.substr(5, 2)) >> std::hex >> b;
    } else if (hex.size() == 9 && hex[0] == '#') {
        std::istringstream(hex.substr(1, 2)) >> std::hex >> r;
        std::istringstream(hex.substr(3, 2)) >> std::hex >> g;
        std::istringstream(hex.substr(5, 2)) >> std::hex >> b;
        std::istringstream(hex.substr(7, 2)) >> std::hex >> a;
    }
    return sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g), static_cast<sf::Uint8>(b), static_cast<sf::Uint8>(a));
}

// 工具函数：优先从 json 读取，否则用默认值
template<typename T>
T getOrDefault(const nlohmann::json& j, const char* key, const T& def) {
    if (j.contains(key) && !j[key].is_null()) {
        try {
            return j[key].get<T>();
        } catch (...) {}
    }
    return def;
}

int main() {
    system("chcp 65001");

    // 读取 init.json 到全局变量
    {
        std::ifstream dbFile("init.json");
        if (!dbFile) {
            std::cout << CONSOLE_BRIGHT_RED << "无法打开 init.json" << CONSOLE_RESET << std::endl;
            return 1;
        }
        dbFile >> g_initDatabase;
    }

    // 读取 package.json 到全局变量
    {
        std::ifstream dbFile("package.json");
        if (!dbFile) {
            std::cout << CONSOLE_BRIGHT_RED << "无法打开 package.json" << CONSOLE_RESET << std::endl;
            return 1;
        }
        dbFile >> g_modelDatabase;
    }

    // 全局窗口和渲染尺寸参数
    int WORK_OFFSET = getOrDefault(g_initDatabase, "WORK_OFFSET", 0);
    int WINDOW_CROP = getOrDefault(g_initDatabase, "WINDOW_CROP", 0);

    // 全局动画参数
    int ACTIVE_LEVEL = getOrDefault(g_initDatabase, "ACTIVE_LEVEL", 2);
    float MIX_TIME = getOrDefault(g_initDatabase, "MIX_TIME", 0.25f);
    float G_SCALE = getOrDefault(g_initDatabase, "G_SCALE", 0.5f);

    int WALK_SPEED = getOrDefault(g_initDatabase, "WALK_SPEED", 100);
    float GRAVITY_TIME = getOrDefault(g_initDatabase, "GRAVITY_TIME", 1.2f);;

    // 字幕相关参数
    float RESIDENCE_TIME = getOrDefault(g_initDatabase, "RESIDENCE_TIME", 2.5f);
    int MAX_SUBTITLES = getOrDefault(g_initDatabase, "MAX_SUBTITLES", 10);
    int SUBTITLE_WIDTH = getOrDefault(g_initDatabase, "SUBTITLE_WIDTH", 0);

    // 初始化主vk表（如有VK_TABLES配置）
    initVkMainTableFromJson(g_initDatabase);
    g_enableSpecialAlpha = getOrDefault(g_initDatabase, "SPECIAL_KEYS", false);

    // 辉光字符串
    std::string GLOW_COLOR = getOrDefault(g_initDatabase, "GLOW_COLOR", std::string("#ffff00"));

    int window_width = (420 * 2 + WINDOW_CROP * 30) * G_SCALE;
    int window_height = (420 * 2 + WINDOW_CROP * 30) * G_SCALE;
    int y_offset = (140 * 2 + WINDOW_CROP * 10) * G_SCALE;

    initWindowAndShader(window_width, window_height, y_offset);
    initSpineModel(window_width, window_height, y_offset, ACTIVE_LEVEL, MIX_TIME, G_SCALE);

    MouseEventManager mouseEventManager;

    // 物理相关
    RECT workAreaRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);

    int work_offset = WORK_OFFSET * 10;

    g_workArea.minX = workAreaRect.left;
    g_workArea.minY = workAreaRect.top;
    g_workArea.maxX = static_cast<int>(workAreaRect.right - window_width);
    g_workArea.maxY = static_cast<int>(workAreaRect.bottom - window_height) + y_offset - work_offset;
    g_workArea.width = window_width;
    g_workArea.height = window_height;

    // 初始化右键菜单（只初始化一次）
    MenuModel model = getDefaultMenuModel();
    static sf::Font font;
    font.loadFromFile("./source/font/Lolita.ttf");
    g_contextMenu = initMenu(model, font, []{ /* 可选：菜单关闭回调 */ });
    g_mainWindow = &window;

    // 启动弹幕窗口线程
    float subtitle_width = 450 + SUBTITLE_WIDTH * 30;
    initSubtitleWindow(RESIDENCE_TIME, MAX_SUBTITLES, subtitle_width);

    float speed = WALK_SPEED * G_SCALE;
    int gravity = (g_workArea.maxY - g_workArea.minY) * 2 / (GRAVITY_TIME * GRAVITY_TIME);

    sf::Clock deltaClock;
    float minFrameTime = 1.0f / 30.0f; // 30 FPS
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            mouseEventManager.handleEvent(event, window);
        }

        // 检查菜单请求退出
        if (g_appShouldExit) {

            // 程序退出前销毁杂七杂八的窗口线程
            forceCloseSubtitleWindow();
            waitSubtitleThreadExit();

            forceCloseMenuWindow();
            waitMenuThreadExit();

            window.close();
            break;
        }

        float delta = deltaClock.restart().asSeconds();

        // 持续应用物理效果
        updateWindowPhysics(hwnd, g_windowPhysicsState, g_workArea, speed, gravity, delta);

        // 防止 drawable 为 nullptr 时崩溃
        if (drawable) {
            drawable->update(delta);
        }

        renderTexture.clear(sf::Color::Transparent);
        if (drawable) {
            renderTexture.draw(*drawable);
        }
        renderTexture.display();

        // 判断是否显示辉光或半透明，可以叠加
        if (g_showGlowEffect || g_showHalfAlpha) {
            sf::Image img = renderTexture.getTexture().copyToImage();
            // 先处理半透明
            if (g_showHalfAlpha) {
                for (unsigned x = 0; x < img.getSize().x; ++x) {
                    for (unsigned y = 0; y < img.getSize().y; ++y) {
                        sf::Color c = img.getPixel(x, y);
                        c.r = static_cast<sf::Uint8>(c.r * 0.5f);
                        c.g = static_cast<sf::Uint8>(c.g * 0.5f);
                        c.b = static_cast<sf::Uint8>(c.b * 0.5f);
                        c.a = static_cast<sf::Uint8>(c.a * 0.5f);
                        img.setPixel(x, y, c);
                    }
                }
            }
            // 再叠加辉光
            if (g_showGlowEffect) {
                img = addGlowToAlphaEdge(img, parseHexColor(GLOW_COLOR), 4);
            }
            setClickThrough(hwnd, img);
        } else {
            setClickThrough(hwnd, renderTexture.getTexture().copyToImage());
        }

        window.clear(sf::Color::Transparent);
        if (drawable) {
            window.draw(*drawable);
        }
        window.display();

        // 限制帧率到30FPS
        float elapsed = deltaClock.getElapsedTime().asSeconds();
        if (elapsed < minFrameTime) {
            sleep(sf::seconds(minFrameTime - elapsed));
        }
    }

    // 程序退出前再次确保所有子线程已释放
    forceCloseSubtitleWindow();
    waitSubtitleThreadExit();

    forceCloseMenuWindow();
    waitMenuThreadExit();

    return 0;
}
