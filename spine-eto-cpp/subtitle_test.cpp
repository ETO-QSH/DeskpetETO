#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <cmath>
#include <iostream>
#include <thread>
#include <windows.h>

#include "spine-eto/keyboard_hook_tool.h"
#include "spine-eto/spine_win_utils.h"

// 全局变量
std::set<DWORD> g_pressedKeys;
std::mutex g_keyMutex;

// Caps/NumLock/ScrollLock状态
bool g_capsOn = false;
bool g_numOn = false;
bool g_scrollOn = false;
std::mutex g_lockStateMutex;

// 获取锁定键状态
void updateLockStates(bool b = true) {
    std::lock_guard lock(g_lockStateMutex);
    g_capsOn = (GetKeyState(VK_CAPITAL) & 1) == 0 ^ b;
    g_numOn = (GetKeyState(VK_NUMLOCK) & 1) == 0 ^ b;
    g_scrollOn = (GetKeyState(VK_SCROLL) & 1) == 0 ^ b;
}

// 钩子回调
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        auto* p = static_cast<KBDLLHOOKSTRUCT*>(reinterpret_cast<void*>(lParam));
        std::lock_guard lock(g_keyMutex);
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            g_pressedKeys.insert(p->vkCode);
            // 检查锁定键
            if (p->vkCode == VK_CAPITAL || p->vkCode == VK_NUMLOCK || p->vkCode == VK_SCROLL) {
                updateLockStates(false);
            }
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            g_pressedKeys.erase(p->vkCode);
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

// 启动钩子的线程
void hookThread() {
    HHOOK hhk = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) { }
    UnhookWindowsHookEx(hhk);
}

// 检查输入法状态（仅支持Windows，获取当前输入法HKL）
HKL getCurrentInputMethod() {
    return GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(), nullptr));
}

int main() {
    system("chcp 65001");

    updateLockStates(); // 启动时获取一次锁定键状态

    std::thread th(hookThread); // 启动钩子线程

    sf::RenderWindow window(sf::VideoMode(450, 600), "Keyboard Hook Example (Global)", sf::Style::None);
    window.setFramerateLimit(60);

    // 设置为无头工具窗口和半透明
    HWND hwnd = window.getSystemHandle();
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_LAYERED | WS_EX_TOOLWINDOW;
    exStyle &= ~WS_EX_APPWINDOW;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    sf::Font fontZh, fontEn;

    // 加载字体（fontZh为中文字体，fontEn为英文字体）
    if (!fontZh.loadFromFile("./source/font/Lolita.ttf")) {
        std::cerr << "中文字体加载失败" << std::endl;
        return 1;
    }
    if (!fontEn.loadFromFile("./source/font/MinecraftiaETO.ttf")) {
        std::cerr << "英文字体加载失败" << std::endl;
        return 1;
    }

    // 字幕相关
    std::deque<SubtitleEntry> subtitles;
    const float SUBTITLE_DURATION = 2.5f;
    const size_t MAX_SUBTITLES = 10;
    sf::Clock clock;

    std::map<DWORD, sf::Time> keyDownAbsTime;
    sf::Clock globalClock;

    // 拖动相关变量
    bool dragging = false;
    sf::Vector2i dragStartMouse;
    sf::Vector2i dragStartWindow;

    // 保持窗口置顶
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    while (window.isOpen()) {
        sf::Event event{};
        std::string subtitleText;

        float baseY = static_cast<float>(window.getSize().y) - 80;
        float subtitleMargin = 6.f;
        float subtitleHeight = 32.f;
        float subtitleLeft = 10.f;
        float subtitleWidth = static_cast<float>(window.getSize().x);

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            // 拖动当前栏实现窗口移动
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                float currentBarHeight = subtitleHeight * 1.2f;
                float currentBarY = static_cast<float>(window.getSize().y) - currentBarHeight;
                sf::FloatRect currentBarRect(0.f, currentBarY, static_cast<float>(window.getSize().x), currentBarHeight);
                if (currentBarRect.contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                    dragging = true;
                    dragStartMouse = sf::Mouse::getPosition();
                    RECT winRect;
                    GetWindowRect(hwnd, &winRect);
                    dragStartWindow = sf::Vector2i(winRect.left, winRect.top);
                }
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                dragging = false;
            }
            if (event.type == sf::Event::MouseMoved && dragging) {
                sf::Vector2i mouseNow = sf::Mouse::getPosition();
                sf::Vector2i delta = mouseNow - dragStartMouse;
                SetWindowPos(hwnd, HWND_TOPMOST, dragStartWindow.x + delta.x, dragStartWindow.y + delta.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
        }

        // 获取全局按键状态
        std::set<DWORD> pressedCopy;
        {
            std::lock_guard lock(g_keyMutex);
            pressedCopy = g_pressedKeys;
        }

        // 记录每个键的绝对按下时间
        updateKeyDownAbsTime(pressedCopy, keyDownAbsTime, globalClock);

        // 清理已松开的键
        cleanReleasedKeys(pressedCopy, keyDownAbsTime);

        // 检测新按下的键
        static std::set<DWORD> prevPressed;
        std::set<DWORD> newlyPressed;
        std::set_difference(
            pressedCopy.begin(), pressedCopy.end(),
            prevPressed.begin(), prevPressed.end(),
            std::inserter(newlyPressed, newlyPressed.end())
        );

        // 切换输入法状态
        setSymbolMap();

        // 使用独立函数处理弹出逻辑
        handleNewlyPressed(
            newlyPressed,
            pressedCopy,
            subtitles,
            MAX_SUBTITLES,
            SUBTITLE_DURATION
        );

        prevPressed = pressedCopy;

        // 更新时间并移除过期字幕
        updateAndCleanSubtitles(subtitles, clock);

        // 可视化显示
        window.clear(sf::Color::Transparent);

        // 绘制到离屏纹理
        static sf::RenderTexture renderTexture;
        if (renderTexture.getSize() != window.getSize())
            renderTexture.create(window.getSize().x, window.getSize().y);
        renderTexture.clear(sf::Color::Transparent);

        // 绘制历史栏内容
        drawHistorySubtitles(renderTexture, subtitles, fontZh, fontEn, baseY,
            SUBTITLE_DURATION, subtitleMargin, subtitleWidth, subtitleLeft);

        // 绘制当前栏内容
        drawCurrentBar(renderTexture, fontZh, fontEn, pressedCopy, subtitleWidth, subtitleHeight, subtitleLeft);

        renderTexture.display();

        // 用 setClickThrough 实现窗口级别的半透明和点击穿透
        setClickThrough(hwnd, renderTexture.getTexture().copyToImage());

        // 主窗口只负责显示
        window.clear(sf::Color::Transparent);
        sf::Sprite spr(renderTexture.getTexture());
        window.draw(spr);
        window.display();
    }
    return 0;
}
