#include <cstdio>
#include <Windows.h>

#include "console_colors.h"
#include "mouse_events.h"
#include "right_click_menu.h"
#include "spine_animation.h"
#include "window_physics.h"

// 声明全局物理状态
extern WindowPhysicsState g_windowPhysicsState;
// 声明全局菜单和主窗口指针
extern MenuWidgetWithHide* g_contextMenu;
extern sf::RenderWindow* g_mainWindow;
// 声明全局工作区
extern WindowWorkArea g_workArea;

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
    static constexpr int doubleClickPixel = 10;

    extern SpineAnimation* animSystem; // 声明外部变量
    extern MenuWidgetWithHide* g_contextMenu; // 声明全局菜单指针

    // 用全局物理状态
    WindowPhysicsState& physicsState = g_windowPhysicsState;
    // 用全局工作区
    const WindowWorkArea& workArea = g_workArea;

    // 用于计算最后minInterval秒的平均速度
    struct WindowMoveSample {
        double time;
        int x, y;
    };
    static std::deque<WindowMoveSample> moveHistory;

    static sf::Clock windowMoveClock;

    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            // 检查双击
            sf::Vector2i pos = sf::Mouse::getPosition(window);
            float elapsed = doubleClickClock.getElapsedTime().asSeconds();
            if (lastClickButton == sf::Mouse::Left &&
                elapsed < doubleClickThreshold &&
                std::abs(pos.x - lastClickPos.x) <= doubleClickPixel &&
                std::abs(pos.y - lastClickPos.y) <= doubleClickPixel) {
                printf(CONSOLE_BRIGHT_GREEN "[INTERACT] Double Click @ (%d, %d)" CONSOLE_RESET "\n", pos.x, pos.y);
                if (animSystem) {
                    animSystem->playTemp("Interact");
                }
                // 新增：双击时将第一个三态按钮状态设为0
                if (g_contextMenu) setFirstTriToggleToZero(reinterpret_cast<MenuWidget*>(g_contextMenu));
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

            // 拖动开始时，物理状态同步
            physicsState.isDragging = true;
            // 拖动时速度清零，位置同步
            RECT rc;
            GetWindowRect(hwnd, &rc);
            physicsState.lastX = static_cast<float>(rc.left);
            physicsState.lastY = static_cast<float>(rc.top);
            physicsState.vx = 0.0f;
            physicsState.vy = 0.0f;

            // 拖动速度采样历史
            moveHistory.clear();
            // 显式类型转换，避免narrowing conversion
            moveHistory.push_back(WindowMoveSample{0.0, static_cast<int>(rc.left), static_cast<int>(rc.top)});
            windowMoveClock.restart();
        } else if (event.mouseButton.button == sf::Mouse::Right) {
            sf::Vector2i pos = sf::Mouse::getPosition(window);
            printf(CONSOLE_BRIGHT_CYAN "[INTERACT] Right Pressed @ (%d, %d)" CONSOLE_RESET "\n", pos.x, pos.y);
            // 弹出右键菜单
            if (g_contextMenu && g_mainWindow) {
                popupMenu(g_mainWindow, sf::Vector2f(static_cast<float>(pos.x), static_cast<float>(pos.y)), g_contextMenu);
            }
        }
    }
    if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Left && dragState.dragging) {
            dragState.dragging = false;
            sf::Vector2i pos = sf::Mouse::getPosition(window);
            printf(CONSOLE_BRIGHT_CYAN "[INTERACT] Left Released @ (%d, %d)" CONSOLE_RESET "\n", pos.x, pos.y);

            HWND hwnd = window.getSystemHandle();
            RECT rc;
            GetWindowRect(hwnd, &rc);
            double now = windowMoveClock.getElapsedTime().asSeconds();

            // 维护历史，插入当前
            moveHistory.push_back(WindowMoveSample{now, static_cast<int>(rc.left), static_cast<int>(rc.top)});
            // 移除minInterval前的点
            while (!moveHistory.empty() && now - moveHistory.front().time > minInterval) {
                moveHistory.pop_front();
            }

            float velocityX = 0.0f, velocityY = 0.0f;
            if (moveHistory.size() >= 2) {
                auto& first = moveHistory.front();
                auto& last = moveHistory.back();
                double dt = last.time - first.time;
                if (dt > 0.01) {
                    velocityX = static_cast<float>(last.x - first.x) / static_cast<float>(dt);
                    velocityY = static_cast<float>(last.y - first.y) / static_cast<float>(dt);
                }
            }
            printf(CONSOLE_BRIGHT_BLUE "[TOTAL] Move To (%d, %d)" CONSOLE_RESET "\n", pos.x, pos.y);
            printf(CONSOLE_BRIGHT_YELLOW "[TOTAL] Velocity (%.2f, %.2f) px/s" CONSOLE_RESET "\n", velocityX, velocityY);

            physicsState.vx = velocityX;
            physicsState.vy = velocityY;
            physicsState.isDragging = false;

            setHandCursorWin(false); // 松开时恢复手型
            accumulatedDelta = sf::Vector2f(0.f, 0.f);
            accumulatedTime = 0.f;
            moveHistory.clear();
        } else if (event.mouseButton.button == sf::Mouse::Right) {
            sf::Vector2i pos = sf::Mouse::getPosition(window);
            printf(CONSOLE_BRIGHT_CYAN "[INTERACT] Right Released @ (%d, %d)" CONSOLE_RESET "\n", pos.x, pos.y);
        }
    }
    if (event.type == sf::Event::MouseMoved) {
        HWND hwnd = window.getSystemHandle();
        POINT pt;
        GetCursorPos(&pt);
        RECT rc;
        GetWindowRect(hwnd, &rc);

        // 拖动限制：鼠标超出工作区时不移动窗口
        if (dragState.dragging) {
            int newLeft = pt.x - (dragState.startPos.x);
            int newTop = pt.y - (dragState.startPos.y);
            // 限制鼠标拖动范围
            newLeft = std::min(std::max(newLeft, workArea.minX), workArea.maxX);
            newTop = std::min(std::max(newTop, workArea.minY), workArea.maxY);
            SetWindowPos(hwnd, nullptr, newLeft, newTop, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            // 拖动时物理状态同步
            physicsState.lastX = static_cast<float>(newLeft);
            physicsState.lastY = static_cast<float>(newTop);

            // 记录窗口移动用于速度计算
            double now = windowMoveClock.getElapsedTime().asSeconds();
            moveHistory.push_back(WindowMoveSample{now, newLeft, newTop});
            // 保留minInterval内的历史
            while (!moveHistory.empty() && now - moveHistory.front().time > minInterval) {
                moveHistory.pop_front();
            }
        }

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
