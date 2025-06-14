#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <atomic>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <windows.h>

#include "keyboard_hook_tool.h"
#include "spine_win_utils.h"

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

static std::atomic<bool> g_hookThreadExit{false};
static std::thread g_hookThread;
static DWORD g_hookThreadId = 0;

// 只保留一个 hookThread 实现，并记录线程ID
void hookThread() {
    g_hookThreadId = GetCurrentThreadId();
    HHOOK hhk = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    MSG msg;
    while (!g_hookThreadExit) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    if (hhk) UnhookWindowsHookEx(hhk);
}

// 检查输入法状态（仅支持Windows，获取当前输入法HKL）
HKL getCurrentInputMethod() {
    return GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(), nullptr));
}

// 全局弹幕窗口指针，供外部访问
HWND g_subtitleHwnd = nullptr;

HWND getSubtitleHwnd() {
    return g_subtitleHwnd;
}

static std::thread g_subtitleThread;
static std::atomic g_threadExit{false};
static std::atomic g_windowVisible{true};
static std::mutex g_windowMutex;
static std::condition_variable g_windowCv;

// 参数缓存
static float g_subtitleDuration = 2.5f;
static size_t g_maxSubtitles = 10;
static int g_modeWidth = 450;

void showSubtitleWindow() {
    g_windowVisible = true;
    g_windowCv.notify_all();
    // 重新定位到任务区右下角（必须异步等待窗口真正创建后再移动）
    std::thread([] {
        for (int i = 0; i < 50; ++i) {
            HWND hwnd = g_subtitleHwnd;
            if (hwnd) {
                RECT workArea;
                SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
                int width = g_modeWidth;
                int height = 600;
                int x = workArea.right - width;
                int y = workArea.bottom - height;
                SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height, SWP_SHOWWINDOW);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }).detach();
}

void hideSubtitleWindow() {
    g_windowVisible = false;
    g_windowCv.notify_all();
}

void subtitleWindowThreadFunc() {
    updateLockStates(); // 启动时获取一次锁定键状态

    g_hookThreadExit = false;
    g_hookThread = std::thread(hookThread); // 启动钩子线程

    sf::RenderWindow* window;

    while (!g_threadExit) {
        // 等待显示请求
        {
            std::unique_lock lk(g_windowMutex);
            g_windowCv.wait(lk, [] { return g_windowVisible || g_threadExit; });
        }
        if (g_threadExit) break;

        // 创建窗口
        window = new sf::RenderWindow(sf::VideoMode(g_modeWidth, 600), "Keyboard Hook", sf::Style::None);
        g_subtitleHwnd = window->getSystemHandle();
        window->setFramerateLimit(30);

        // 设置为无头工具窗口和半透明
        HWND hwnd = g_subtitleHwnd;
        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        exStyle |= WS_EX_LAYERED | WS_EX_TOOLWINDOW;
        exStyle &= ~WS_EX_APPWINDOW;
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

        sf::Font fontZh, fontEn;

        // 加载字体（fontZh为中文字体，fontEn为英文字体）
        if (!fontZh.loadFromFile("./source/font/Lolita.ttf") || !fontEn.loadFromFile("./source/font/MinecraftiaETO.ttf")) {
            delete window;
            g_subtitleHwnd = nullptr;
            return;
        }

        // 字幕队列
        std::deque<SubtitleEntry> subtitles;
        sf::Clock clock;

        std::map<DWORD, sf::Time> keyDownAbsTime;
        sf::Clock globalClock;

        // 拖动相关变量
        bool dragging = false;
        sf::Vector2i dragStartMouse;
        sf::Vector2i dragStartWindow;

        // 保持窗口置顶
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        while (window->isOpen() && !g_threadExit && g_windowVisible) {
            sf::Event event{};
            std::string subtitleText;

            float baseY = static_cast<float>(window->getSize().y) - 80;
            float subtitleMargin = 6.f;
            float subtitleHeight = 32.f;
            float subtitleLeft = 10.f;
            float subtitleWidth = static_cast<float>(window->getSize().x);

            while (window->pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window->close();

                // 拖动当前栏实现窗口移动
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(*window);
                    float currentBarHeight = subtitleHeight * 1.2f;
                    float currentBarY = static_cast<float>(window->getSize().y) - currentBarHeight;
                    sf::FloatRect currentBarRect(0.f, currentBarY, static_cast<float>(window->getSize().x), currentBarHeight);
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
                g_maxSubtitles,
                g_subtitleDuration
            );

            prevPressed = pressedCopy;

            // 更新时间并移除过期字幕
            updateAndCleanSubtitles(subtitles, clock);

            // 可视化显示
            window->clear(sf::Color::Transparent);

            // 绘制到离屏纹理
            static sf::RenderTexture renderTexture;
            if (renderTexture.getSize() != window->getSize())
                renderTexture.create(window->getSize().x, window->getSize().y);
            renderTexture.clear(sf::Color::Transparent);

            // 绘制历史栏内容
            drawHistorySubtitles(renderTexture, subtitles, fontZh, fontEn, baseY,
                g_subtitleDuration, subtitleMargin, subtitleWidth, subtitleLeft);

            // 绘制当前栏内容
            drawCurrentBar(renderTexture, fontZh, fontEn, pressedCopy, subtitleWidth, subtitleHeight, subtitleLeft);

            renderTexture.display();

            // 用 setClickThrough 实现窗口级别的半透明和点击穿透
            setClickThrough(hwnd, renderTexture.getTexture().copyToImage());

            // 主窗口只负责显示
            window->clear(sf::Color::Transparent);
            sf::Sprite spr(renderTexture.getTexture());
            window->draw(spr);
            window->display();
        }
        
        // 隐藏窗口
        ShowWindow(hwnd, SW_HIDE);
        delete window;
        g_subtitleHwnd = nullptr;

        // 等待下次显示
        if (!g_threadExit) {
            std::unique_lock lk(g_windowMutex);
            g_windowCv.wait(lk, [] { return g_windowVisible || g_threadExit; });
        }
    }
    g_subtitleHwnd = nullptr;

    // 退出前关闭钩子线程
    g_hookThreadExit = true;
    if (g_hookThreadId != 0) {
        PostThreadMessage(g_hookThreadId, WM_QUIT, 0, 0);
    }
    if (g_hookThread.joinable()) g_hookThread.join();
}

void initSubtitleWindow(float subtitleDuration, size_t maxSubtitles, int modeWidth) {
    g_subtitleDuration = subtitleDuration;
    g_maxSubtitles = maxSubtitles;
    g_modeWidth = modeWidth;
    g_threadExit = false;
    g_windowVisible = false; // 初始化时不显示
    if (!g_subtitleThread.joinable()) {
        g_subtitleThread = std::thread(subtitleWindowThreadFunc);
    }
}

static std::mutex g_subtitleThreadMutex;

// 立即销毁弹幕窗口和线程（线程安全，强制关闭窗口）
void forceCloseSubtitleWindow() {
    std::lock_guard lock(g_subtitleThreadMutex);
    if (g_subtitleHwnd) {
        PostMessage(g_subtitleHwnd, WM_CLOSE, 0, 0);
        g_subtitleHwnd = nullptr;
    }
    g_threadExit = true;
    g_windowVisible = false;
    g_windowCv.notify_all();
    if (g_subtitleThread.joinable()) {
        g_subtitleThread.join();
    }
    g_threadExit = false;
    // 这里确保钩子线程也被安全 join
    g_hookThreadExit = true;
    if (g_hookThread.joinable()) g_hookThread.join();
}

// 等待弹幕窗口线程安全退出
void waitSubtitleThreadExit() {
    std::lock_guard lock(g_subtitleThreadMutex);
    if (g_subtitleThread.joinable()) {
        g_subtitleThread.join();
    }
}
