#pragma once

#include <SFML/Graphics.hpp>
#include <deque>
#include <map>
#include <set>
#include <string>

// 字幕结构体
struct SubtitleEntry {
    std::string text;
    float timer;
};

// 支持直接用HKL切换符号映射表
void setSymbolMap();

// 工具函数声明
sf::String symbolForHistory(
    const std::string& n,
    const std::set<std::string>& modKeys,
    bool forHistory,
    bool isMiscChar = false
);

sf::String comboToString(const std::set<DWORD>& keys, bool forHistory = false);

void extractModKeysAndOthers(
    const std::set<DWORD>& combo,
    std::set<std::string>& modKeys,
    std::vector<std::string>& others
);

void handleNewlyPressed(
    const std::set<DWORD>& newlyPressed,
    const std::set<DWORD>& pressedCopy,
    std::deque<SubtitleEntry>& subtitles,
    size_t MAX_SUBTITLES,
    float SUBTITLE_DURATION
);

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
);

void drawCurrentBar(
    sf::RenderTarget& window,
    const sf::Font& fontZh,
    const sf::Font& fontEn,
    const std::set<DWORD>& pressedCopy,
    float subtitleWidth,
    float subtitleHeight,
    float subtitleLeft
);

void updateAndCleanSubtitles(std::deque<SubtitleEntry>& subtitles, sf::Clock& clock);

void updateKeyDownAbsTime(
    const std::set<DWORD>& pressedCopy,
    std::map<DWORD, sf::Time>& keyDownAbsTime,
    sf::Clock& globalClock
);

void cleanReleasedKeys(
    const std::set<DWORD>& pressedCopy,
    std::map<DWORD, sf::Time>& keyDownAbsTime
);

// 判断当前输入法是否为中文输入模式
bool IsChineseInput();

// 控制是否开启特殊处理的全局参数
extern bool g_enableSpecialAlpha;
