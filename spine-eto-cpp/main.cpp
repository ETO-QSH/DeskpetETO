#include <spine/spine-sfml.h>
#include <SFML/Graphics.hpp>

#include "spine-eto/spine_win_utils.h"
#include "spine-eto/window_physics.h"
#include "spine-eto/mouse_events.h"

using namespace spine;

// 声明全局物理状态，供 mouse_events.cpp 使用
WindowPhysicsState g_windowPhysicsState;

int main() {
    system("chcp 65001");

    initWindowAndShader();
    initSpineModel();

    MouseEventManager mouseEventManager;

    // 物理相关
    RECT workAreaRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0);
    WindowWorkArea workArea{};
    workArea.minX = workAreaRect.left;
    workArea.minY = workAreaRect.top;
    workArea.maxX = static_cast<int>(workAreaRect.right - window.getSize().x);
    workArea.maxY = static_cast<int>(workAreaRect.bottom - window.getSize().y);
    workArea.width = static_cast<int>(window.getSize().x);
    workArea.height = static_cast<int>(window.getSize().y);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            mouseEventManager.handleEvent(event, window);
        }

        float delta = deltaClock.restart().asSeconds();

        // 持续应用物理效果
        updateWindowPhysics(hwnd, g_windowPhysicsState, workArea, delta);

        drawable->update(delta);

        renderTexture.clear(sf::Color::Transparent);
        renderTexture.draw(*drawable);
        renderTexture.display();

        setClickThrough(hwnd, renderTexture.getTexture().copyToImage());

        window.clear(sf::Color::Transparent);
        window.draw(*drawable);
        window.display();
    }

    return 0;
}
