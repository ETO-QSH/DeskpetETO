#pragma once

#include <SFML/Graphics.hpp>

#include <functional>
#include <string>
#include <vector>

// 菜单项类型
enum class MenuEntryType { Action, Separator, SubMenu, Toggle, ToggleTri };

// 菜单项结构体
struct MenuEntry {
    MenuEntryType type = MenuEntryType::Action;
    std::string text;
    std::string iconPath;
    std::function<void()> callback;
    std::vector<MenuEntry> submenu;
    int toggleState = 0;
    std::function<void(const int&)> toggleCallback;
    std::vector<sf::Color> triColors;

    static MenuEntry Action(const std::string& text, const std::string& iconPath, std::function<void()> cb) {
        MenuEntry e;
        e.type = MenuEntryType::Action;
        e.text = text;
        e.iconPath = iconPath;
        e.callback = std::move(cb);
        return e;
    }
    static MenuEntry Separator() {
        MenuEntry e;
        e.type = MenuEntryType::Separator;
        return e;
    }
    static MenuEntry SubMenu(const std::string& text, const std::string& iconPath, std::vector<MenuEntry> submenu) {
        MenuEntry e;
        e.type = MenuEntryType::SubMenu;
        e.text = text;
        e.iconPath = iconPath;
        e.submenu = std::move(submenu);
        return e;
    }
    static MenuEntry Toggle(const std::string& text, const std::string& iconPath, int initialState, std::function<void(const int&)> cb) {
        MenuEntry e;
        e.type = MenuEntryType::Toggle;
        e.text = text;
        e.iconPath = iconPath;
        e.toggleState = initialState;
        e.toggleCallback = std::move(cb);
        return e;
    }
    static MenuEntry ToggleTri(const std::string& text, const std::string& iconPath, int initialState, std::function<void(const int&)> cb) {
        MenuEntry e;
        e.type = MenuEntryType::ToggleTri;
        e.text = text;
        e.iconPath = iconPath;
        e.toggleState = initialState;
        e.toggleCallback = std::move(cb);
        e.triColors = { sf::Color(63, 191, 255), sf::Color(127, 127, 255), sf::Color(255, 159, 159) };
        return e;
    }
};

// 菜单管理类
class MenuModel {
public:
    void addAction(const std::string& text, const std::string& iconPath, std::function<void()> cb) {
        entries.push_back(MenuEntry::Action(text, iconPath, std::move(cb)));
    }
    void addSeparator() {
        entries.push_back(MenuEntry::Separator());
    }
    void addSubMenu(const std::string& text, const std::string& iconPath, const std::vector<MenuEntry>& submenu) {
        entries.push_back(MenuEntry::SubMenu(text, iconPath, submenu));
    }
    [[nodiscard]] const std::vector<MenuEntry>& getEntries() const { return entries; }
    std::vector<MenuEntry>& getEntries() { return entries; }
private:
    std::vector<MenuEntry> entries;
};

// 菜单显示控件声明
class MenuWidget;
struct MenuWidgetWithHide;

// 初始化菜单控件
MenuWidgetWithHide* initMenu(const MenuModel& model, const sf::Font& font, std::function<void()> onHideAll);

// 弹出菜单（每次弹出直接创建新窗口，主线程不阻塞，无需线程管理）
void popupMenu(sf::RenderWindow* parentWindow, const sf::Vector2f& pos, MenuWidgetWithHide* menu);

// 绘制菜单到RenderTexture
void drawMenu(MenuWidget* menu, sf::RenderTexture& target);

// 设置第一个三态按钮为0（只设置第一个找到的ToggleTri）
void setFirstTriToggleToZero(MenuWidget* root);

// 主动强制关闭所有菜单线程和窗口
void forceCloseMenuWindow();
void waitMenuThreadExit();
