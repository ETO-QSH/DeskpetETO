#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <windows.h>

#include "vkCodeToString.h"

// 全局变量
std::set<int> g_pressedKeys;
std::mutex g_keyMutex;

// 钩子回调
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        auto* p = (KBDLLHOOKSTRUCT*)lParam;
        std::lock_guard lock(g_keyMutex);
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            g_pressedKeys.insert(p->vkCode);
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

// 组合键转字符串（返回 sf::String 以支持中文和英文）
sf::String comboToString(const std::set<int>& keys) {
    sf::String s;
    for (auto it = keys.begin(); it != keys.end(); ++it) {
        if (it != keys.begin()) s += L" + ";
        s += vkCodeToString(*it); // 直接拼接
    }
    return s;
}

int main() {
    system("chcp 65001");

    std::thread th(hookThread); // 启动钩子线程

    sf::RenderWindow window(sf::VideoMode(600, 200), "Keyboard Hook Example (Global)");
    sf::Font fontZh, fontEn;

    // 加载字体（fontZh为中文字体，fontEn为英文字体）
    if (!fontZh.loadFromFile("D:/Desktop/Desktop/keyboard-hook/source/font/Lolita.ttf")) {
        std::cerr << "中文字体加载失败" << std::endl;
        return 1;
    }
    if (!fontEn.loadFromFile("D:/Desktop/Desktop/keyboard-hook/source/font/Minecraftia.ttf")) {
        std::cerr << "英文字体加载失败" << std::endl;
        return 1;
    }

    std::set<int> lastComboKeys;
    sf::String lastComboStr;

    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // 获取全局按键状态
        std::set<int> pressedCopy;
        {
            std::lock_guard lock(g_keyMutex);
            pressedCopy = g_pressedKeys;
        }

        // 检测是否有按键松开（组合变化）
        static std::set<int> prevPressed;
        if (pressedCopy.size() < prevPressed.size()) {
            if (!prevPressed.empty()) {
                lastComboKeys = prevPressed;
                lastComboStr = comboToString(lastComboKeys);
                std::wcout << L"组合键: " << lastComboStr.toWideString() << std::endl;
            }
        }
        prevPressed = pressedCopy;

        // 可视化显示
        window.clear(sf::Color::Black);

        // 当前按下
        sf::Text textZh;
        textZh.setFont(fontZh);
        textZh.setCharacterSize(28);
        textZh.setFillColor(sf::Color::White);
        textZh.setPosition(20, 120);
        textZh.setString(L"当前按下:  ");

        sf::Text textEn;
        textEn.setFont(fontEn);
        textEn.setCharacterSize(28);
        textEn.setFillColor(sf::Color::White);
        // 拼接在中文后面
        float offsetX = textZh.getPosition().x + textZh.getLocalBounds().width;
        textEn.setPosition(offsetX, 120);
        textEn.setString(comboToString(pressedCopy));

        window.draw(textZh);
        window.draw(textEn);

        // 上次组合
        sf::Text lastTextZh;
        lastTextZh.setFont(fontZh);
        lastTextZh.setCharacterSize(22);
        lastTextZh.setFillColor(sf::Color(200, 200, 200));
        lastTextZh.setPosition(20, 60);
        lastTextZh.setString(L"上次组合:  ");

        sf::Text lastTextEn;
        lastTextEn.setFont(fontEn);
        lastTextEn.setCharacterSize(22);
        lastTextEn.setFillColor(sf::Color(200, 200, 200));
        float offsetX2 = lastTextZh.getPosition().x + lastTextZh.getLocalBounds().width;
        lastTextEn.setPosition(offsetX2, 60);
        lastTextEn.setString(lastComboStr);

        window.draw(lastTextZh);
        window.draw(lastTextEn);

        window.display();
    }
    return 0;
}
