#include <windows.h>

#include "vk_code_2_string.h"

// 主表实现 (https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)
const VkTable& getMainVkTable() {
    static VkTable vkMap = {

        // 鼠标按键区（0x00 ~ 0x06）
        {VK_LBUTTON, "MouseLeft"}, {VK_RBUTTON, "MouseRight"}, {VK_CANCEL, "MouseBreak"},
        {VK_MBUTTON, "MouseMiddle"}, {VK_XBUTTON1, "MouseX1"}, {VK_XBUTTON2, "MouseX2"},

        // Reserved: 0x07

        // Back键和Tab键（0x08 ~ 0x09）
        {VK_BACK, "Back"}, {VK_TAB, "Tab"},

        // Reserved: 0x0A ~ 0x0B

        // Clear键和Enter键（0x0C ~ 0x0D）
        {VK_CLEAR, "Clear"}, {VK_RETURN, "Enter"},

        // Unassigned: 0x0E ~ 0x0F

        // Shift键和Ctrl键和Alt键（0x10 ~ 0x12）
        {VK_SHIFT, "Shift"}, {VK_CONTROL, "Ctrl"}, {VK_MENU, "Alt"},

        // Pause键和CapsLock键（0x13 ~ 0x14）
        {VK_PAUSE, "Pause"}, {VK_CAPITAL, "CapsLock"},

        // 输入法切换键区（0x15 ~ 0x1A）
        {VK_KANA, "Kana"}, {VK_HANGUL, "Hangul"}, {VK_IME_ON, "IMEOn"},
        {VK_JUNJA, "Junja"}, {VK_FINAL, "Final"},
        {VK_HANJA, "Hanja"}, {VK_KANJI, "Kanji"}, {VK_IME_OFF, "IMEOff"},

        // Escape键（0x1B）
        {VK_ESCAPE, "Escape"},

        // 输入法操作键区（0x1C ~ 0x1F）
        {VK_CONVERT, "Convert"}, {VK_NONCONVERT, "NonConvert"},
        {VK_ACCEPT, "Accept"}, {VK_MODECHANGE, "ModeChange"},

        // 浏览键区（0x20 ~ 0x24）
        {VK_SPACE, "Space"}, {VK_PRIOR, "PageUp"}, {VK_NEXT, "PageDown"},
        {VK_END, "End"}, {VK_HOME, "Home"},

        // 方向键区（0x25 ~ 0x28）
        {VK_LEFT, "Left"}, {VK_UP, "Up"}, {VK_RIGHT, "Right"}, {VK_DOWN, "Down"},

        // 编辑快捷键区（0x29 ~ 0x2F）
        {VK_SELECT, "Select"}, {VK_PRINT, "Print"}, {VK_EXECUTE, "Execute"},
        {VK_SNAPSHOT, "Snapshot"}, {VK_INSERT, "Insert"}, {VK_DELETE, "Delete"},
        {VK_HELP, "Help"},

        // 主键盘数字区（0x30 ~ 0x39）
        {'1', "1!"}, {'2', "2@"}, {'3', "3#"}, {'4', "4$"}, {'5', "5%"},
        {'6', "6^"}, {'7', "7&"}, {'8', "8*"}, {'9', "9("}, {'0', "0)"},

        // Reserved: 0x3A ~ 0x40

        // 主键盘字母区（0x41 ~ 0x5A）
        {'A', "A"}, {'B', "B"}, {'C', "C"}, {'D', "D"}, {'E', "E"}, {'F', "F"},
        {'G', "G"}, {'H', "H"}, {'I', "I"}, {'J', "J"}, {'K', "K"}, {'L', "L"},
        {'M', "M"}, {'N', "N"}, {'O', "O"}, {'P', "P"}, {'Q', "Q"}, {'R', "R"},
        {'S', "S"}, {'T', "T"}, {'U', "U"}, {'V', "V"}, {'W', "W"}, {'X', "X"},
        {'Y', "Y"}, {'Z', "Z"},

        // 重要系统键区（0x5B ~ 0x5D）
        {VK_LWIN, "LWin"}, {VK_RWIN, "RWin"}, {VK_APPS, "Apps"},

        // Reserved: 0x5E

        // 休眠键（0x5F）
        {VK_SLEEP, "Sleep"},

        // 数字键盘区（0x60 ~ 0x69）
        {VK_NUMPAD0, "Num0"}, {VK_NUMPAD1, "Num1"}, {VK_NUMPAD2, "Num2"}, {VK_NUMPAD3, "Num3"},
        {VK_NUMPAD4, "Num4"}, {VK_NUMPAD5, "Num5"}, {VK_NUMPAD6, "Num6"}, {VK_NUMPAD7, "Num7"},
        {VK_NUMPAD8, "Num8"}, {VK_NUMPAD9, "Num9"},

        // 数字键盘操作符区（0x6A ~ 0x6F）
        {VK_MULTIPLY, "Num*"}, {VK_ADD, "Num+"}, {VK_SEPARATOR, "Separator"},
        {VK_SUBTRACT, "Num-"}, {VK_DECIMAL, "Num."}, {VK_DIVIDE, "Num/"},

        // 常用Function键区（0x70 ~ 0x7B）
        {VK_F1, "F1"}, {VK_F2, "F2"}, {VK_F3, "F3"}, {VK_F4, "F4"},
        {VK_F5, "F5"}, {VK_F6, "F6"}, {VK_F7, "F7"}, {VK_F8, "F8"}, 
        {VK_F9, "F9"}, {VK_F10, "F10"}, {VK_F11, "F11"}, {VK_F12, "F12"},

        // 少用Function键区（0x7C ~ 0x87）
        {VK_F13, "F13"}, {VK_F14, "F14"}, {VK_F15, "F15"}, {VK_F16, "F16"},
        {VK_F17, "F17"}, {VK_F18, "F18"}, {VK_F19, "F19"}, {VK_F20, "F20"},
        {VK_F21, "F21"}, {VK_F22, "F22"}, {VK_F23, "F23"}, {VK_F24, "F24"},
        
        // Reserved: 0x88 ~ 0x8F

        // 数字键盘锁和滚动操作锁（0x90 ~ 0x91）
        {VK_NUMLOCK, "NumLock"}, {VK_SCROLL, "ScrollLock"},

        // OEM specific: 0x92 ~ 0x96

        // Unassigned: 0x97 ~ 0x9F

        // 重要功能键区（0xA0 ~ 0xA5）
        {VK_LSHIFT, "LShift"}, {VK_RSHIFT, "RShift"},
        {VK_LCONTROL, "LCtrl"}, {VK_RCONTROL, "RCtrl"},
        {VK_LMENU, "LAlt"}, {VK_RMENU, "RAlt"},

        // 浏览器控制键区（0xA6 ~ 0xAC）
        {VK_BROWSER_BACK, "BrowserBack"}, {VK_BROWSER_FORWARD, "BrowserForward"},
        {VK_BROWSER_REFRESH, "BrowserRefresh"}, {VK_BROWSER_STOP, "BrowserStop"},
        {VK_BROWSER_SEARCH, "BrowserSearch"}, {VK_BROWSER_FAVORITES, "BrowserFavorites"},
        {VK_BROWSER_HOME, "BrowserHome"},

        // 多媒体控制键区（0xAD ~ 0xB7）
        {VK_VOLUME_MUTE, "VolumeMute"}, {VK_VOLUME_DOWN, "VolumeDown"}, {VK_VOLUME_UP, "VolumeUp"},
        {VK_MEDIA_NEXT_TRACK, "MediaNext"}, {VK_MEDIA_PREV_TRACK, "MediaPrev"},
        {VK_MEDIA_STOP, "MediaStop"}, {VK_MEDIA_PLAY_PAUSE, "MediaPlayPause"},
        {VK_LAUNCH_MAIL, "LaunchMail"}, {VK_LAUNCH_MEDIA_SELECT, "LaunchMedia"},
        {VK_LAUNCH_APP1, "LaunchApp1"}, {VK_LAUNCH_APP2, "LaunchApp2"},

        // Reserved: 0xB8 ~ 0xB9

        // 常用符号键区（0xBA ~ 0xC0）
        {VK_OEM_1, ";:"}, {VK_OEM_PLUS, "+="}, {VK_OEM_COMMA, ",<"},
        {VK_OEM_MINUS, "-_"}, {VK_OEM_PERIOD, ".>"}, {VK_OEM_2, "/?"}, {VK_OEM_3, "`~"},
        
        // Reserved: 0xC1 ~ 0xDA

        // 括号和引号键区（0xDB ~ 0xDF）
        {VK_OEM_4, "[{"}, {VK_OEM_5, "\\|"}, {VK_OEM_6, "]}"},
        {VK_OEM_7, "'\""}, {VK_OEM_8, "MiscChar"},

        // Reserved: 0xE0

        // OEM specific: 0xE1
        
        // RT102键键盘上的尖括号键或反斜杠键: 0xE2
        {VK_OEM_102, "RT102"},

        // OEM specific: 0xE3 ~ 0xE4

        // 查看输入法是否对当前按键做处理: 0xE5
        {VK_PROCESSKEY, "ProcessKey"},

        // OEM specific: 0xE6

        // Packet键，用于传递Unicode字符: 0xE7
        {VK_PACKET, "Packet"},

        // Unassigned: 0xE8

        // OEM specific: 0xE9 ~ 0xF5

        // 其余虚拟键键区（0xF6 ~ 0xFE）
        {VK_ATTN, "Attn"}, {VK_CRSEL, "CrSel"}, {VK_EXSEL, "ExSel"},
        {VK_EREOF, "EraseEOF"}, {VK_PLAY, "Play"}, {VK_ZOOM, "Zoom"},
        {VK_NONAME, "NoName"}, {VK_PA1, "PA1"}, {VK_OEM_CLEAR, "OEMClear"}
        
    };
    
    return vkMap;
}

// 鼠标按键分表
const VkTable& mouseVkTables() {
    static VkTable table = {
        {VK_LBUTTON, "MouseLeft"}, {VK_RBUTTON, "MouseRight"}, {VK_CANCEL, "MouseBreak"},
        {VK_MBUTTON, "MouseMiddle"}, {VK_XBUTTON1, "MouseX1"}, {VK_XBUTTON2, "MouseX2"}
    };
    return table;
}

// 输入法相关分表
const VkTable& imeVkTables() {
    static VkTable table = {
        {VK_KANA, "Kana"}, {VK_HANGUL, "Hangul"}, {VK_IME_ON, "IMEOn"},
        {VK_JUNJA, "Junja"}, {VK_FINAL, "Final"},
        {VK_HANJA, "Hanja"}, {VK_KANJI, "Kanji"}, {VK_IME_OFF, "IMEOff"},
        {VK_CONVERT, "Convert"}, {VK_NONCONVERT, "NonConvert"},
        {VK_ACCEPT, "Accept"}, {VK_MODECHANGE, "ModeChange"}
    };
    return table;
}

// 方向键分表
const VkTable& directionVkTables() {
    static VkTable table = {
        {VK_LEFT, "Left"}, {VK_UP, "Up"}, {VK_RIGHT, "Right"}, {VK_DOWN, "Down"}
    };
    return table;
}

// 主键盘数字键分表
const VkTable& mainNumVkTables() {
    static VkTable table = {
        {'1', "1!"}, {'2', "2@"}, {'3', "3#"}, {'4', "4$"}, {'5', "5%"},
        {'6', "6^"}, {'7', "7&"}, {'8', "8*"}, {'9', "9("}, {'0', "0)"}
    };
    return table;
}

// 主键盘数字键分表
const VkTable& alphabetVkTables() {
    static VkTable table = {
        {'A', "A"}, {'B', "B"}, {'C', "C"}, {'D', "D"}, {'E', "E"}, {'F', "F"},
        {'G', "G"}, {'H', "H"}, {'I', "I"}, {'J', "J"}, {'K', "K"}, {'L', "L"},
        {'M', "M"}, {'N', "N"}, {'O', "O"}, {'P', "P"}, {'Q', "Q"}, {'R', "R"},
        {'S', "S"}, {'T', "T"}, {'U', "U"}, {'V', "V"}, {'W', "W"}, {'X', "X"},
        {'Y', "Y"}, {'Z', "Z"}
    };
    return table;
}

// 小键盘数字键分表
const VkTable& liteNumVkTables() {
    static VkTable table = {
        {VK_NUMPAD0, "Num0"}, {VK_NUMPAD1, "Num1"}, {VK_NUMPAD2, "Num2"}, {VK_NUMPAD3, "Num3"},
        {VK_NUMPAD4, "Num4"}, {VK_NUMPAD5, "Num5"}, {VK_NUMPAD6, "Num6"}, {VK_NUMPAD7, "Num7"},
        {VK_NUMPAD8, "Num8"}, {VK_NUMPAD9, "Num9"}
    };
    return table;
}

// 小键盘数字键分表
const VkTable& liteNumOpVkTables() {
    static VkTable table = {
        {VK_MULTIPLY, "Num*"}, {VK_ADD, "Num+"}, {VK_SEPARATOR, "Separator"},
        {VK_SUBTRACT, "Num-"}, {VK_DECIMAL, "Num."}, {VK_DIVIDE, "Num/"}
    };
    return table;
}

// Function区分表
const VkTable& funcNumVkTables() {
    static VkTable table = {
        {VK_F1, "F1"}, {VK_F2, "F2"}, {VK_F3, "F3"}, {VK_F4, "F4"},
        {VK_F5, "F5"}, {VK_F6, "F6"}, {VK_F7, "F7"}, {VK_F8, "F8"},
        {VK_F9, "F9"}, {VK_F10, "F10"}, {VK_F11, "F11"}, {VK_F12, "F12"},
        {VK_F13, "F13"}, {VK_F14, "F14"}, {VK_F15, "F15"}, {VK_F16, "F16"},
        {VK_F17, "F17"}, {VK_F18, "F18"}, {VK_F19, "F19"}, {VK_F20, "F20"},
        {VK_F21, "F21"}, {VK_F22, "F22"}, {VK_F23, "F23"}, {VK_F24, "F24"}
    };
    return table;
}

// 支持度高的基础按键分表
const VkTable& highFuncVkTable1() {
    static VkTable table = {
        {VK_BACK, "Back"}, {VK_TAB, "Tab"}, {VK_RETURN, "Enter"}, {VK_CAPITAL, "CapsLock"},
        {VK_ESCAPE, "Escape"}, {VK_SPACE, "Space"}, {VK_PRIOR, "PageUp"}, {VK_NEXT, "PageDown"},
        {VK_END, "End"}, {VK_HOME, "Home"}, {VK_INSERT, "Insert"}, {VK_DELETE, "Delete"}
    };
    return table;
}

// 支持度中的基础按键分表
const VkTable& midFuncVkTable1() {
    static VkTable table = {
        {VK_CLEAR, "Clear"}, {VK_PAUSE, "Pause"}, {VK_SELECT, "Select"}, {VK_PRINT, "Print"},
        {VK_EXECUTE, "Execute"}, {VK_SNAPSHOT, "Snapshot"}, {VK_HELP, "Help"},
        {VK_NUMLOCK, "NumLock"}, {VK_SCROLL, "ScrollLock"}
    };
    return table;
}

// 非常重要的修饰键分表
const VkTable& modifyVkTable1() {
    static VkTable table = {
        {VK_LWIN, "LWin"}, {VK_RWIN, "RWin"}, {VK_APPS, "Apps"},
        {VK_MENU, "Alt"},{VK_LMENU, "LAlt"}, {VK_RMENU, "RAlt"},
        {VK_SHIFT, "Shift"}, {VK_LSHIFT, "LShift"}, {VK_RSHIFT, "RShift"},
        {VK_CONTROL, "Ctrl"}, {VK_LCONTROL, "LCtrl"}, {VK_RCONTROL, "RCtrl"}
    };
    return table;
}

// 浏览器控制键分表
const VkTable& browserVkTables() {
    static VkTable table = {
        {VK_BROWSER_BACK, "BrowserBack"}, {VK_BROWSER_FORWARD, "BrowserForward"},
        {VK_BROWSER_REFRESH, "BrowserRefresh"}, {VK_BROWSER_STOP, "BrowserStop"},
        {VK_BROWSER_SEARCH, "BrowserSearch"}, {VK_BROWSER_FAVORITES, "BrowserFavorites"},
        {VK_BROWSER_HOME, "BrowserHome"}
    };
    return table;
}

// 多媒体控制键分表
const VkTable& appVkTables() {
    static VkTable table = {
        {VK_VOLUME_MUTE, "VolumeMute"}, {VK_VOLUME_DOWN, "VolumeDown"}, {VK_VOLUME_UP, "VolumeUp"},
        {VK_MEDIA_NEXT_TRACK, "MediaNext"}, {VK_MEDIA_PREV_TRACK, "MediaPrev"},
        {VK_MEDIA_STOP, "MediaStop"}, {VK_MEDIA_PLAY_PAUSE, "MediaPlayPause"},
        {VK_LAUNCH_MAIL, "LaunchMail"}, {VK_LAUNCH_MEDIA_SELECT, "LaunchMedia"},
        {VK_LAUNCH_APP1, "LaunchApp1"}, {VK_LAUNCH_APP2, "LaunchApp2"}
    };
    return table;
}

// 常用标点符号键分表
const VkTable& interVkTables() {
    static VkTable table = {
        {VK_OEM_1, ";:"}, {VK_OEM_PLUS, "+="}, {VK_OEM_COMMA, ",<"},
        {VK_OEM_MINUS, "-_"}, {VK_OEM_PERIOD, ".>"}, {VK_OEM_2, "/?"}, {VK_OEM_3, "`~"},
        {VK_OEM_4, "[{"}, {VK_OEM_5, "\\|"}, {VK_OEM_6, "]}"},
        {VK_OEM_7, "'\""}, {VK_OEM_8, "MiscChar"}
    };
    return table;
}

// 其余杂项虚拟键分表
const VkTable& otherVkTables() {
    static VkTable table = {
        {VK_SLEEP, "Sleep"}, {VK_OEM_102, "RT102"},
        {VK_PROCESSKEY, "ProcessKey"}, {VK_PACKET, "Packet"},
        {VK_ATTN, "Attn"}, {VK_CRSEL, "CrSel"}, {VK_EXSEL, "ExSel"},
        {VK_EREOF, "EraseEOF"}, {VK_PLAY, "Play"}, {VK_ZOOM, "Zoom"},
        {VK_NONAME, "NoName"}, {VK_PA1, "PA1"}, {VK_OEM_CLEAR, "OEMClear"}
    };
    return table;
}

// 分表组合
VkTable combineVkTables(const std::vector<const VkTable*>& tables) {
    VkTable result;
    for (auto t : tables) {
        result.insert(t->begin(), t->end());
    }
    return result;
}

// 主接口
std::string vkCodeToString(int vkCode, const VkTable& table) {
    auto it = table.find(vkCode);
    if (it != table.end()) return it->second;
    return "VK(" + std::to_string(vkCode) + ")";
}
