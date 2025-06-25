#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <cmath>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <windows.h>

#include "keyboard_hook_tool.h"
#include "vk_code_2_string.h"

extern std::mutex g_lockStateMutex;
extern bool g_capsOn;
extern bool g_numOn;
extern bool g_scrollOn;

// 全局符号映射表
static std::map<char, char> g_symbolMap = { };

// 符号映射表-EN
static std::map<char, char> g_symbolMapEN = { };

// 符号映射表-CN
static std::map<char, char> g_symbolMapCN = {
    {'$', '￥'}, {'`', '·'},
};

// 判断当前输入法是否为中文输入模式
bool IsChineseInput() {
    HWND hIME = ImmGetDefaultIMEWnd(GetForegroundWindow());
    LRESULT status = SendMessage(hIME, WM_IME_CONTROL, 0x0005, 0);
    return status ? true : false;
}

// 根据输入法HKL自动切换符号映射表
void setSymbolMap() {
    if (IsChineseInput()) {
        // 中文输入法
        g_symbolMap = g_symbolMapCN;
    } else {
        // 英文或其它
        g_symbolMap = g_symbolMapEN;
    }
}

// 圆角矩形辅助函数
class RoundedRectangleShape : public sf::Shape {
public:
    explicit RoundedRectangleShape(const sf::Vector2f& size, float radius = 10.f, std::size_t cornerPointCount = 8)
        : m_size(size), m_radius(radius), m_cornerPointCount(cornerPointCount) {
        update();
    }

    void setSize(const sf::Vector2f& size) { m_size = size; update(); }
    void setCornersRadius(float radius) { m_radius = radius; update(); }
    void setCornerPointCount(std::size_t count) { m_cornerPointCount = count; update(); }

    std::size_t getPointCount() const override {
        return m_cornerPointCount * 4;
    }

    sf::Vector2f getPoint(std::size_t index) const override {
        std::size_t corner = index / m_cornerPointCount;
        float theta = static_cast<float>(index % m_cornerPointCount) / static_cast<float>(m_cornerPointCount) * 3.1415926f / 2.f;
        float x, y;
        switch (corner) {
            case 0: // top-left
                x = m_radius * (1 - std::cos(theta));
            y = m_radius * (1 - std::sin(theta));
            break;
            case 1: // top-right
                x = m_size.x - m_radius + m_radius * std::sin(theta);
            y = m_radius * (1 - std::cos(theta));
            break;
            case 2: // bottom-right
                x = m_size.x - m_radius + m_radius * std::cos(theta);
            y = m_size.y - m_radius + m_radius * std::sin(theta);
            break;
            case 3: // bottom-left
                x = m_radius * (1 - std::sin(theta));
            y = m_size.y - m_radius + m_radius * std::cos(theta);
            break;
            default: // 防御性编程，返回左上角
                x = 0;
            y = 0;
            break;
        }
        return { x, y };
    }

private:
    sf::Vector2f m_size;
    float m_radius;
    std::size_t m_cornerPointCount;
};

// 工具函数：历史栏符号处理（适用于interVkTables和mainNumVkTables）
sf::String symbolForHistory(
    const std::string& n,
    const std::set<std::string>& modKeys,
    bool forHistory,
    bool isMiscChar
) {
    // g_enableSpecialAlpha=false时，历史栏也输出双符号（和当前栏一致）
    if (!g_enableSpecialAlpha) {
        return sf::String::fromUtf8(n.begin(), n.end());
    }
    // 历史栏且只和shift组合/无修饰时输出单符号，否则输出双符号
    if (isMiscChar || n.size() != 2) {
        return sf::String::fromUtf8(n.begin(), n.end());
    }
    if (forHistory) {
        char ch = 0;
        if (modKeys.size() == 1 && modKeys.count("Shift") == 1) {
            ch = n[1];
        } else if (modKeys.empty()) {
            ch = n[0];
        }
        if (ch) {
            // 查映射表，若有则替换
            auto it = g_symbolMap.find(ch);
            if (it != g_symbolMap.end()) {
                // 输出映射后的符号
                return sf::String::fromUtf8(&it->second, &it->second + 1);
            }
            // 没有映射则原样输出
            return sf::String::fromUtf8(&ch, &ch + 1);
        }
    }
    // 当前栏或其它修饰，输出双符号
    return sf::String::fromUtf8(n.begin(), n.end());
}

// 修改comboToString，增加forHistory参数
sf::String comboToString(const std::set<DWORD>& keys, bool forHistory) {
    static const std::vector<std::string> priority = {"Win", "Ctrl", "Alt", "Shift"};
    std::vector<std::string> ordered;
    std::vector<std::string> others;

    bool capsOn, numOn, scrollOn;
    {
        std::lock_guard lock(g_lockStateMutex);
        capsOn = g_capsOn;
        numOn = g_numOn;
        scrollOn = g_scrollOn;
    }

    if (keys.size() == 1) {
        DWORD vk = *keys.begin();
        if (vk == VK_CAPITAL) return capsOn ? L"CapsOn" : L"CapsOff";
        if (vk == VK_NUMLOCK) return numOn ? L"NumLockOn" : L"NumLockOff";
        if (vk == VK_SCROLL) return scrollOn ? L"ScrollLockOn" : L"ScrollLockOff";
    }

    std::set<std::string> modKeys;
    for (const auto& pri : priority) {
        for (DWORD vk : keys) {
            std::string name = vkCodeToString(static_cast<int>(vk));
            if (name.size() > 1 && (name[0] == 'L' || name[0] == 'R')) {
                std::string base = name.substr(1);
                if (base == pri) {
                    modKeys.insert(pri);
                    ordered.push_back(pri);
                    continue;
                }
            }
            if (name == pri) {
                modKeys.insert(pri);
                ordered.push_back(pri);
            }
        }
    }

    for (DWORD vk : keys) {
        std::string name = vkCodeToString(static_cast<int>(vk));
        bool isMod = false;
        for (const auto& pri : priority) {
            if (name == pri) { isMod = true; break; }
            if (name.size() > 1 && (name[0] == 'L' || name[0] == 'R') && name.substr(1) == pri) {
                isMod = true; break;
            }
        }
        if (!isMod) others.push_back(name);
    }

    bool onlyAlpha = false;
    bool onlyShiftAlpha = false;
    if (others.size() == 1 && others[0].size() == 1 && std::isalpha(others[0][0])) {
        onlyAlpha = (keys.size() == 1);
        onlyShiftAlpha = (keys.size() == 2 && modKeys.count("Shift") == 1);
    }

    // interVkTables符号处理
    const VkTable& interTable = interVkTables();
    const VkTable& mainNumTable = mainNumVkTables();

    sf::String s;
    bool first = true;
    for (const auto& n : ordered) {
        // interVkTables/mainNumVkTables符号处理：历史栏且只和shift组合时，不输出shift+，只输出符号
        if (g_enableSpecialAlpha && forHistory && modKeys.size() == 1 && modKeys.count("Shift") == 1) {
            if (others.size() == 1 && n == "Shift") {
                // interVkTables
                if (interTable.end() != std::find_if(interTable.begin(), interTable.end(),
                    [&](const std::pair<int, std::string>& p) { return p.second == others[0]; }) && others[0].size() == 2) {
                    continue; // 跳过shift
                }
                // mainNumVkTables
                if (mainNumTable.end() != std::find_if(mainNumTable.begin(), mainNumTable.end(),
                    [&](const std::pair<int, std::string>& p) { return p.second == others[0]; }) && others[0].size() == 2) {
                    continue; // 跳过shift
                }
            }
        }
        if (!first) s += L" + ";
        s += sf::String::fromUtf8(n.begin(), n.end());
        first = false;
    }
    for (const auto& n : others) {
        if (!first) s += L" + ";
        // 字母键大小写处理
        if (n.size() == 1 && std::isalpha(n[0])) {
            char ch = n[0];
            if (g_enableSpecialAlpha && (onlyAlpha || onlyShiftAlpha)) {
                if (capsOn)
                    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                else
                    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            } else {
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
            s += sf::String::fromUtf8(&ch, &ch + 1);
        }
        // interVkTables符号处理
        else if (interTable.end() != std::find_if(interTable.begin(), interTable.end(),
            [&](const std::pair<int, std::string>& p) { return p.second == n; })) {
            bool isMiscChar = (n == "MiscChar");
            // 总是输出双符号，只有历史栏且只和shift/无修饰时输出单符号
            s += symbolForHistory(n, modKeys, forHistory, isMiscChar);
        }
        // mainNumVkTables符号处理
        else if (mainNumTable.end() != std::find_if(mainNumTable.begin(), mainNumTable.end(),
            [&](const std::pair<int, std::string>& p) { return p.second == n; })) {
            // 总是输出双符号，只有历史栏且只和shift/无修饰时输出单符号
            s += symbolForHistory(n, modKeys, forHistory, false);
        }
        // 其它
        else if (n == "CapsLock") {
            s += capsOn ? L"CapsOn" : L"CapsOff";
        } else if (n == "NumLock") {
            s += numOn ? L"NumLockOn" : L"NumLockOff";
        } else if (n == "ScrollLock") {
            s += scrollOn ? L"ScrollLockOn" : L"ScrollLockOff";
        } else {
            s += sf::String::fromUtf8(n.begin(), n.end());
        }
        first = false;
    }
    return s;
}

// 从组合中提取modKeys和others
void extractModKeysAndOthers(
    const std::set<DWORD>& combo,
    std::set<std::string>& modKeys,
    std::vector<std::string>& others
) {
    static const std::vector<std::string> priority = {"Win", "Ctrl", "Alt", "Shift"};
    for (const auto& pri : priority) {
        for (DWORD vk2 : combo) {
            std::string name = vkCodeToString(static_cast<int>(vk2));
            if (name.size() > 1 && (name[0] == 'L' || name[0] == 'R')) {
                std::string base = name.substr(1);
                if (base == pri) {
                    modKeys.insert(pri);
                    continue;
                }
            }
            if (name == pri) {
                modKeys.insert(pri);
            }
        }
    }
    for (DWORD vk2 : combo) {
        std::string name = vkCodeToString(static_cast<int>(vk2));
        bool isMod = false;
        for (const auto& pri : priority) {
            if (name == pri) { isMod = true; break; }
            if (name.size() > 1 && (name[0] == 'L' || name[0] == 'R') && name.substr(1) == pri) {
                isMod = true; break;
            }
        }
        if (!isMod) others.push_back(name);
    }
}

// 处理新按下的键并弹出历史字幕
void handleNewlyPressed(
    const std::set<DWORD>& newlyPressed,
    const std::set<DWORD>& pressedCopy,
    std::deque<SubtitleEntry>& subtitles,
    const size_t MAX_SUBTITLES,
    float SUBTITLE_DURATION
) {
    static const std::vector<std::string> priority = {"Win", "Ctrl", "Alt", "Shift"};
    const VkTable& modifyTable = modifyVkTables();

    auto isModifyKey = [&](DWORD vk) {
        return modifyTable.find(static_cast<int>(vk)) != modifyTable.end();
    };

    bool hasModify = false;
    std::set<DWORD> currentModifyKeys;
    for (DWORD vk : pressedCopy) {
        if (isModifyKey(vk)) {
            hasModify = true;
            currentModifyKeys.insert(vk);
        }
    }

    for (DWORD vk : newlyPressed) {
        std::set single{vk};
        if ((vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL) && pressedCopy.size() == 1) {
            std::string histText = comboToString(single, true).toAnsiString();
            subtitles.push_back({histText, SUBTITLE_DURATION});
            if (subtitles.size() > MAX_SUBTITLES)
                subtitles.pop_front();
        }
        else if (!hasModify) {
            std::string histText = comboToString(single, true).toAnsiString();
            subtitles.push_back({histText, SUBTITLE_DURATION});
            if (subtitles.size() > MAX_SUBTITLES)
                subtitles.pop_front();
        } else {
            const std::set<DWORD>& combo = pressedCopy;
            std::set<std::string> modKeys;
            std::vector<std::string> others;
            extractModKeysAndOthers(combo, modKeys, others);

            bool onlyShiftAlpha = false;
            if (others.size() == 1 && others[0].size() == 1 && std::isalpha(others[0][0])) {
                onlyShiftAlpha = (combo.size() == 2 && modKeys.count("Shift") == 1);
            }

            if (onlyShiftAlpha) {
                char ch = others[0][0];
                bool capsOn;
                {
                    std::lock_guard lock(g_lockStateMutex);
                    capsOn = g_capsOn;
                }
                if (capsOn)
                    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                else
                    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                std::string histText(1, ch);
                subtitles.push_back({histText, SUBTITLE_DURATION});
                if (subtitles.size() > MAX_SUBTITLES)
                    subtitles.pop_front();
            } else {
                std::string histText = comboToString(combo, true).toAnsiString();
                subtitles.push_back({histText, SUBTITLE_DURATION});
                if (subtitles.size() > MAX_SUBTITLES)
                    subtitles.pop_front();
            }
        }
    }
}

// 绘制历史字幕
void drawHistorySubtitles(
    sf::RenderTarget& window,
    const std::deque<SubtitleEntry>& subtitles,
    const sf::Font& fontZh,
    const sf::Font& fontEn,
    float baseY,
    float SUBTITLE_DURATION,
    float subtitleMargin,
    float subtitleWidth,
    float subtitleLeft
) {
    int idx = 0;
    // 让上面的字幕更透明且先消失
    for (auto it = subtitles.rbegin(); it != subtitles.rend(); ++it, ++idx) {
        float alpha = std::clamp(it->timer / SUBTITLE_DURATION, 0.f, 1.f);

        // 计算实际文本高度
        sf::Text textZh;
        textZh.setFont(fontZh);
        textZh.setCharacterSize(20);
        textZh.setString(L"历史： ");

        sf::Text textEn;
        textEn.setFont(fontEn);
        textEn.setCharacterSize(20);
        textEn.setString(sf::String::fromUtf8(it->text.begin(), it->text.end()));

        float textHeight = textZh.getLocalBounds().height + 6;
        float bgHeight = textHeight + 9.f; // 适当加padding
        float yPos = baseY - idx * (bgHeight + subtitleMargin);

        // 使用圆角矩形
        RoundedRectangleShape bg({subtitleWidth, bgHeight}, 8.f, 16);
        bg.setPosition(0.f, yPos);
        bg.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(180 * alpha)));
        window.draw(bg);
    }

    idx = 0;
    for (auto it = subtitles.rbegin(); it != subtitles.rend(); ++it, ++idx) {
        sf::Text textZh;
        textZh.setFont(fontZh);
        textZh.setCharacterSize(20);
        textZh.setString(L"历史： ");

        sf::Text textEn;
        textEn.setFont(fontEn);
        textEn.setCharacterSize(20);
        textEn.setString(sf::String::fromUtf8(it->text.begin(), it->text.end()));
        float textHeight = textZh.getLocalBounds().height + 6;
        float bgHeight = textHeight + 9.f;
        float yPos = baseY - idx * (bgHeight + subtitleMargin);

        float ratio = std::max(0.f, it->timer / SUBTITLE_DURATION);
        float alpha = 255.f * std::log1p(5 * ratio) / std::log1p(5.f);
        auto a = static_cast<sf::Uint8>(alpha);

        textZh.setFillColor(sf::Color(255, 255, 255, a));
        textZh.setOutlineColor(sf::Color(0, 0, 0, a));
        textZh.setOutlineThickness(3);
        textZh.setPosition(subtitleLeft, yPos + 4);

        window.draw(textZh);

        if (!it->text.empty()) {
            textEn.setFillColor(sf::Color(255, 255, 255, a));
            textEn.setOutlineColor(sf::Color(0, 0, 0, a));
            textEn.setOutlineThickness(3);
            float offsetX2 = textZh.getPosition().x + (textZh.getLocalBounds().width + textZh.getLocalBounds().left);
            textEn.setPosition(offsetX2, yPos + 9);
            window.draw(textEn);
        }
    }
}

// 绘制当前栏
void drawCurrentBar(
    sf::RenderTarget& window,
    const sf::Font& fontZh,
    const sf::Font& fontEn,
    const std::set<DWORD>& pressedCopy,
    float subtitleWidth,
    float subtitleHeight,
    float subtitleLeft
) {
    // 当前栏高度提升为1.2倍
    float currentBarHeight = subtitleHeight * 1.2f;
    float currentBarY = static_cast<float>(window.getSize().y) - currentBarHeight;

    // 使用圆角矩形
    RoundedRectangleShape currentBarBg({subtitleWidth, currentBarHeight}, 8.f, 16);
    currentBarBg.setPosition(0.f, currentBarY);
    currentBarBg.setFillColor(sf::Color(0, 0, 0, 191));
    window.draw(currentBarBg);

    sf::Text currentZh;
    currentZh.setFont(fontZh);
    currentZh.setCharacterSize(24);
    currentZh.setFillColor(sf::Color::White);

    // 垂直居中
    float zhOffsetY = currentBarY + (currentBarHeight - currentZh.getLocalBounds().height) / 2.f - 15.f;
    currentZh.setPosition(subtitleLeft, zhOffsetY);
    currentZh.setString(L"当前： ");
    currentZh.setOutlineColor(sf::Color(0, 0, 0, 191));
    currentZh.setOutlineThickness(3);

    sf::Text currentEn;
    currentEn.setFont(fontEn);
    currentEn.setCharacterSize(24);
    currentEn.setFillColor(sf::Color::White);

    float offsetX = currentZh.getPosition().x + currentZh.getLocalBounds().width;
    currentEn.setPosition(offsetX, zhOffsetY + 5.f);
    currentEn.setString(pressedCopy.empty() ? "" : comboToString(pressedCopy, false));
    currentEn.setOutlineColor(sf::Color(0, 0, 0, 191));
    currentEn.setOutlineThickness(3);

    window.draw(currentZh);
    window.draw(currentEn);
}

// 更新时间并移除过期字幕
void updateAndCleanSubtitles(std::deque<SubtitleEntry>& subtitles, sf::Clock& clock) {
    float dt = clock.restart().asSeconds();
    for (auto& entry : subtitles) {
        entry.timer -= dt;
    }
    while (!subtitles.empty() && subtitles.front().timer <= 0)
        subtitles.pop_front();
}

// 记录每个键的绝对按下时间
void updateKeyDownAbsTime(
    const std::set<DWORD>& pressedCopy,
    std::map<DWORD, sf::Time>& keyDownAbsTime,
    sf::Clock& globalClock
) {
    for (DWORD vk : pressedCopy) {
        if (keyDownAbsTime.find(vk) == keyDownAbsTime.end()) {
            keyDownAbsTime[vk] = globalClock.getElapsedTime();
        }
    }
}

// 清理已松开的键
void cleanReleasedKeys(
    const std::set<DWORD>& pressedCopy,
    std::map<DWORD, sf::Time>& keyDownAbsTime
) {
    for (auto it = keyDownAbsTime.begin(); it != keyDownAbsTime.end(); ) {
        if (pressedCopy.find(it->first) == pressedCopy.end()) {
            it = keyDownAbsTime.erase(it);
        } else {
            ++it;
        }
    }
}
