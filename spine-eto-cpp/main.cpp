#include <spine/spine-sfml.h>
#include <SFML/Graphics.hpp>

#include <chrono>
#include <thread>

#include "spine-eto/menu_model_utils.h"
#include "spine-eto/mouse_events.h"
#include "spine-eto/right_click_menu.h"
#include "spine-eto/spine_win_utils.h"
#include "spine-eto/window_physics.h"

// 声明全局退出标志
extern bool g_appShouldExit;

using namespace spine;

// 声明全局物理状态，供 mouse_events.cpp 使用
WindowPhysicsState g_windowPhysicsState;
// 声明全局菜单指针，供 mouse_events.cpp 使用
MenuWidgetWithHide* g_contextMenu = nullptr;
sf::RenderWindow* g_mainWindow = nullptr;
// 声明全局工作区，供 mouse_events.cpp 使用
WindowWorkArea g_workArea;

// 全局窗口和渲染尺寸参数
constexpr int WINDOW_WIDTH = 360;
constexpr int WINDOW_HEIGHT = 360;
constexpr int Y_OFFSET = 120;

// 全局活跃系数
constexpr int ACTIVE_LEVEL = 2;

int main() {
    system("chcp 65001");

    initWindowAndShader(WINDOW_WIDTH, WINDOW_HEIGHT, Y_OFFSET);
    initSpineModel(WINDOW_WIDTH, WINDOW_HEIGHT, Y_OFFSET);

    MouseEventManager mouseEventManager;

    // 物理相关
    RECT workAreaRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);

    g_workArea.minX = workAreaRect.left;
    g_workArea.minY = workAreaRect.top;
    g_workArea.maxX = static_cast<int>(workAreaRect.right - WINDOW_WIDTH);
    g_workArea.maxY = static_cast<int>(workAreaRect.bottom - WINDOW_HEIGHT) + Y_OFFSET;
    g_workArea.width = WINDOW_WIDTH;
    g_workArea.height = WINDOW_HEIGHT;

    // 初始化右键菜单（只初始化一次）
    MenuModel model = getDefaultMenuModel();
    static sf::Font font;
    font.loadFromFile("./source/font/Lolita.ttf");
    g_contextMenu = initMenu(model, font, []{ /* 可选：菜单关闭回调 */ });
    g_mainWindow = &window;

    sf::Clock deltaClock;
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
            // 主动强制关闭所有菜单线程和窗口
            extern void forceCloseMenuWindow();
            forceCloseMenuWindow();
            extern void waitMenuThreadExit();
            waitMenuThreadExit();
            window.close();
            break;
        }

        float delta = deltaClock.restart().asSeconds();

        // 持续应用物理效果
        updateWindowPhysics(hwnd, g_windowPhysicsState, g_workArea, delta);

        drawable->update(delta);

        renderTexture.clear(sf::Color::Transparent);
        renderTexture.draw(*drawable);
        renderTexture.display();

        setClickThrough(hwnd, renderTexture.getTexture().copyToImage());

        window.clear(sf::Color::Transparent);
        window.draw(*drawable);
        window.display();
    }

    // 程序退出前再次确保所有菜单线程已释放
    extern void forceCloseMenuWindow();
    forceCloseMenuWindow();
    extern void waitMenuThreadExit();
    waitMenuThreadExit();
    return 0;
}
