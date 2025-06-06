#include <cstdio>
#include <Windows.h>

#include "mouse_events.h"
#include "console_colors.h"
#include "spine_animation.h"

MouseEventManager::MouseEventManager() = default;

void setHandCursorWin(bool pressed) {
    static HCURSOR hHand = LoadCursor(nullptr, IDC_HAND);
    static HCURSOR hArrow = LoadCursor(nullptr, IDC_ARROW);
    static HCURSOR hClosed = LoadCursor(nullptr, IDC_SIZEALL);
    // static HCURSOR hClosed = LoadCursorFromFileW(L"C:\\Windows\\Cursors\\handwriting.ani"); // 可自定义抓手样式
    if (pressed && hClosed)
        SetCursor(hClosed);
    else if (hHand)
        SetCursor(hHand);
    else
        SetCursor(hArrow);
}

void MouseEventManager::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    static constexpr float minInterval = 0.2f; // 0.2秒输出一次

    // 拖动累积
    static sf::Vector2f accumulatedDelta(0.f, 0.f);
    static float accumulatedTime = 0.f;
    static sf::Vector2i dragOffset; // 鼠标按下时窗口左上角到鼠标的偏移

    // 双击检测
    static sf::Clock doubleClickClock;
    static int lastClickButton = -1;
    static sf::Vector2i lastClickPos;
    static constexpr float doubleClickThreshold = 0.5f; // 秒
    static constexpr int doubleClickPixel = 4;

    extern SpineAnimation* animSystem; // 声明外部变量

    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            // 检查双击
            sf::Vector2i pos = sf::Mouse::getPosition(window);
            float elapsed = doubleClickClock.getElapsedTime().asSeconds();
            if (lastClickButton == sf::Mouse::Left &&
                elapsed < doubleClickThreshold &&
                std::abs(pos.x - lastClickPos.x) <= doubleClickPixel &&
                std::abs(pos.y - lastClickPos.y) <= doubleClickPixel) {
                printf(CONSOLE_BRIGHT_GREEN "[INTERACT] Left Double Click @ (%d, %d)" CONSOLE_RESET "\n", pos.x, pos.y);
                if (animSystem) {
                    animSystem->playTemp("Interact");
                }
            }
            lastClickButton = sf::Mouse::Left;
            lastClickPos = pos;
            doubleClickClock.restart();

            dragState.dragging = true;
            dragState.startPos = pos;
            dragState.lastPos = dragState.startPos;
            dragState.currentPos = dragState.startPos;
            dragState.dragClock.restart();
            velocityClock.restart();
            accumulatedDelta = sf::Vector2f(0.f, 0.f);
            accumulatedTime = 0.f;

            // 记录窗口左上角到鼠标的偏移
            RECT winRect;
            HWND hwnd = window.getSystemHandle();
            GetWindowRect(hwnd, &winRect);
            dragOffset.x = pos.x + winRect.left;
            dragOffset.y = pos.y + winRect.top;

            printf(CONSOLE_BRIGHT_CYAN "[INTERACT] Left Pressed @ (%d, %d)" CONSOLE_RESET "\n", pos.x, pos.y);
            setHandCursorWin(true); // 按下时抓手
        } else if (event.mouseButton.button == sf::Mouse::Right) {
            sf::Vector2i pos = sf::Mouse::getPosition(window);
            printf(CONSOLE_BRIGHT_CYAN "[INTERACT] Right Pressed @ (%d, %d)" CONSOLE_RESET "\n", pos.x, pos.y);
            if (animSystem) {
                animSystem->setFlip(true, false);
            }
        }
    }
    if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Left && dragState.dragging) {
            dragState.dragging = false;
            sf::Vector2i pos = sf::Mouse::getPosition(window);
            printf(CONSOLE_BRIGHT_CYAN "[INTERACT] Left Released @ (%d, %d)" CONSOLE_RESET "\n", pos.x, pos.y);
            // 输出末速度
            float totalTime = accumulatedTime + velocityClock.getElapsedTime().asSeconds();
            sf::Vector2f totalDelta = accumulatedDelta;
            sf::Vector2i lastPos = dragState.currentPos;
            sf::Vector2i nowPos = sf::Mouse::getPosition(window);
            totalDelta.x += static_cast<float>(nowPos.x - lastPos.x);
            totalDelta.y += static_cast<float>(nowPos.y - lastPos.y);
            if (totalTime > 0.0001f) {
                sf::Vector2f finalVelocity = totalDelta / totalTime;
                printf(CONSOLE_BRIGHT_BLUE "[STATE] Move To (%d, %d)" CONSOLE_RESET "\n", nowPos.x, nowPos.y);
                printf(CONSOLE_BRIGHT_YELLOW "[STATE] Velocity (%.2f, %.2f) px/s" CONSOLE_RESET "\n", finalVelocity.x, finalVelocity.y);
            }
            setHandCursorWin(false); // 松开时恢复手型
            accumulatedDelta = sf::Vector2f(0.f, 0.f);
            accumulatedTime = 0.f;
        } else if (event.mouseButton.button == sf::Mouse::Right) {
            sf::Vector2i pos = sf::Mouse::getPosition(window);
            printf(CONSOLE_BRIGHT_CYAN "[INTERACT] Right Released @ (%d, %d)" CONSOLE_RESET "\n", pos.x, pos.y);
        }
    }
    if (event.type == sf::Event::MouseMoved) {
        // 判断鼠标是否在窗口区域
        HWND hwnd = window.getSystemHandle();
        POINT pt;
        GetCursorPos(&pt);
        RECT rc;
        GetWindowRect(hwnd, &rc);
        if (PtInRect(&rc, pt)) {
            if (dragState.dragging)
                setHandCursorWin(true);
            else
                setHandCursorWin(false);
        } else {
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
        }
    }

    if (event.type == sf::Event::MouseMoved && dragState.dragging) {
        sf::Vector2i newPos = sf::Mouse::getPosition(window);
        float dt = velocityClock.getElapsedTime().asSeconds();
        int dx = newPos.x - dragState.lastPos.x;
        int dy = newPos.y - dragState.lastPos.y;
        if (dt > 0 && (dx != 0 || dy != 0)) {
            accumulatedDelta.x += static_cast<float>(dx);
            accumulatedDelta.y += static_cast<float>(dy);
            accumulatedTime += dt;
            dragState.lastPos = dragState.currentPos;
            dragState.currentPos = newPos;
            if (accumulatedTime >= minInterval) {
                sf::Vector2f velocity = accumulatedDelta / accumulatedTime;
                printf(CONSOLE_BRIGHT_BLUE "[STATE] Move To (%d, %d)" CONSOLE_RESET "\n", newPos.x, newPos.y);
                printf(CONSOLE_BRIGHT_YELLOW "[STATE] Velocity (%.2f, %.2f) px/s" CONSOLE_RESET "\n", velocity.x, velocity.y);
                dragState.velocity = velocity;
                accumulatedDelta = sf::Vector2f(0.f, 0.f);
                accumulatedTime = 0.f;
            }
            velocityClock.restart();
        }
        // 拖动窗口
        HWND hwnd = window.getSystemHandle();
        POINT pt;
        GetCursorPos(&pt);
        int newLeft = pt.x - (dragState.startPos.x);
        int newTop = pt.y - (dragState.startPos.y);
        SetWindowPos(hwnd, nullptr, newLeft, newTop, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

bool MouseEventManager::isDragging() const {
    return dragState.dragging;
}

sf::Vector2i MouseEventManager::getDragStart() const {
    return dragState.startPos;
}

sf::Vector2i MouseEventManager::getDragCurrent() const {
    return dragState.currentPos;
}

sf::Vector2f MouseEventManager::getDragVelocity() const {
    return dragState.velocity;
}
