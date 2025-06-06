#include <SFML/Graphics.hpp>
#include <utility>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <functional>
#include <map>

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

// 菜单项类型
enum class MenuEntryType { Action, Separator, SubMenu };

// 菜单项结构体
struct MenuEntry {
    MenuEntryType type = MenuEntryType::Action; // 默认初始化
    std::string text;
    std::string iconPath;
    std::function<void()> callback;
    std::vector<MenuEntry> submenu;

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

// 菜单显示控件，递归支持子菜单和点击事件
class MenuWidget {
public:
    MenuWidget(const std::vector<MenuEntry>& entries, const sf::Font& font)
        : m_entries(entries), m_font(font), visible(false), hoverIndex(-1), parent(nullptr) {
        // 递归生成子菜单控件
        for (const auto& entry : m_entries) {
            if (entry.type == MenuEntryType::SubMenu) {
                auto sub = std::make_unique<MenuWidget>(entry.submenu, font);
                sub->parent = this;
                m_submenus.push_back(std::move(sub));
            } else {
                m_submenus.push_back(nullptr);
            }
        }
    }

    void setPosition(const sf::Vector2f& pos) { m_position = pos; }
    void show(const sf::Vector2f& pos) {
        setPosition(pos);
        visible = true;
        hoverIndex = -1;
        // 隐藏所有子菜单
        for (auto& sub : m_submenus) if (sub) sub->hide();
    }
    void hide() {
        visible = false;
        hoverIndex = -1;
        for (auto& sub : m_submenus) if (sub) sub->hide();
    }
    [[nodiscard]] bool isVisible() const { return visible; }

    // 鼠标事件处理
    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (!visible) return false;
        bool consumed = false;
        if (event.type == sf::Event::MouseMoved) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseMove.x, event.mouseMove.y });
            hoverIndex = getItemIndexAt(mousePos);

            // 判断鼠标是否在某个子菜单区域内
            int submenuActive = -1;
            for (size_t i = 0; i < m_submenus.size(); ++i) {
                if (m_submenus[i] && m_submenus[i]->isVisible() && m_submenus[i]->isPointInMenu(mousePos)) {
                    submenuActive = static_cast<int>(i);
                    break;
                }
            }

            // 先清除所有子菜单
            for (auto& sub : m_submenus) {
                if (sub) sub->hide();
            }
            // 只显示一个子菜单，其余全部隐藏
            for (size_t i = 0; i < m_entries.size(); ++i) {
                if (m_entries[i].type == MenuEntryType::SubMenu && m_submenus[i]) {
                    // 如果当前hover在主菜单项上，或鼠标在子菜单区域内，保持子菜单激活
                    if (static_cast<int>(i) == hoverIndex || static_cast<int>(i) == submenuActive) {
                        sf::Vector2f subPos = getSubmenuPosition(i);
                        m_submenus[i]->show(subPos);
                    }
                }
            }
        }
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });
            int idx = getItemIndexAt(mousePos);
            bool inAnySubMenu = false;
            // 检查鼠标是否在任何子菜单内
            for (auto& sub : m_submenus) {
                if (sub && sub->isVisible()) {
                    if (sub->isPointInMenu(mousePos)) {
                        inAnySubMenu = true;
                        break;
                    }
                }
            }
            if (idx >= 0 && idx < static_cast<int>(m_entries.size())) {
                const auto& entry = m_entries[idx];
                if (entry.type == MenuEntryType::Action && entry.callback) {
                    entry.callback();
                    hideAll();
                    consumed = true;
                }
                if (entry.type == MenuEntryType::SubMenu && m_submenus[idx]) {
                    consumed = true;
                }
            } else if (!inAnySubMenu) {
                hideAll();
            }
        }
        // 传递事件给可见子菜单
        for (auto& sub : m_submenus) {
            if (sub && sub->isVisible()) {
                if (sub->handleEvent(event, window)) consumed = true;
            }
        }
        return consumed;
    }

    // 判断点是否在当前菜单区域内
    [[nodiscard]] bool isPointInMenu(const sf::Vector2f& pt) const {
        float width = 200.f, height = 0.f, itemHeight = 32.f;
        for (const auto& entry : m_entries)
            height += (entry.type == MenuEntryType::Separator) ? 8.f : itemHeight;
        sf::FloatRect rect(m_position.x, m_position.y, width, height);
        if (rect.contains(pt)) return true;
        // 检查子菜单
        for (const auto& sub : m_submenus) {
            if (sub && sub->isVisible() && sub->isPointInMenu(pt)) return true;
        }
        return false;
    }

    void draw(sf::RenderTarget& target) {
        if (!visible) return;
        // 菜单整体边距
        float margin = 4.f, margin_d = 10.f;
        float width = 150.f, height = 0.f, itemHeight = 32.f, padding = 8.f;

        // 计算总高度
        for (const auto& entry : m_entries)
            height += (entry.type == MenuEntryType::Separator) ? padding : itemHeight;
        height += margin + margin_d;

        // 背景（含圆角，含边距）
        RoundedRectangleShape bg({ width + margin * 2, height }, 12.f, 12);
        bg.setFillColor(sf::Color(40, 40, 40, 230));
        bg.setPosition(m_position.x - margin, m_position.y - margin);
        target.draw(bg);

        // 绘制每个菜单项
        float y = m_position.y + margin;
        for (size_t i = 0; i < m_entries.size(); ++i) {
            const auto& entry = m_entries[i];
            float iconSize = 24.f;

            if (entry.type == MenuEntryType::Separator) {
                sf::RectangleShape line({ width - 2 * padding, 1.f });
                line.setFillColor(sf::Color(100, 100, 100, 180));
                line.setPosition(m_position.x + padding, y + 4.f);
                target.draw(line);
                y += 8.f;
                continue;
            }
            // 圆角高亮
            if (static_cast<int>(i) == hoverIndex) {
                float highlightRadius = 10.f;
                RoundedRectangleShape hi({ width, itemHeight }, highlightRadius, 8);
                hi.setFillColor(sf::Color(60, 60, 60, 220));
                hi.setPosition(m_position.x, y);
                target.draw(hi);
            }
            // 图标（路径加载&缓存）
            if (!entry.iconPath.empty()) {
                sf::Texture& icon = getIcon(entry.iconPath);
                sf::Sprite iconSprite(icon);
                iconSprite.setScale(iconSize / icon.getSize().x, iconSize / icon.getSize().y);
                iconSprite.setPosition(m_position.x + padding, y + (itemHeight - iconSize) / 2.f);
                target.draw(iconSprite);
            }
            // 中文文本
            if (entry.type == MenuEntryType::Action || entry.type == MenuEntryType::SubMenu) {
                sf::Text text;
                text.setFont(m_font);
                text.setCharacterSize(18);
                text.setFillColor(sf::Color::White);
                text.setString(sf::String::fromUtf8(entry.text.begin(), entry.text.end())); // 支持中文
                text.setPosition(m_position.x + padding + (!entry.iconPath.empty() ? iconSize + 8.f : 0.f), y + (itemHeight - static_cast<float>(text.getCharacterSize())) / 2.f - 2.f);
                target.draw(text);
            }
            // 子菜单箭头
            if (entry.type == MenuEntryType::SubMenu) {
                sf::ConvexShape arrow;
                arrow.setPointCount(3);
                float ax = m_position.x + width - padding - 10.f;
                float ay = y + itemHeight / 2.f;
                arrow.setPoint(0, { ax, ay - 6.f });
                arrow.setPoint(1, { ax, ay + 6.f });
                arrow.setPoint(2, { ax + 8.f, ay });
                arrow.setFillColor(sf::Color(200, 200, 200));
                target.draw(arrow);
            }
            y += itemHeight;
        }
        // 绘制子菜单
        for (auto& sub : m_submenus) {
            if (sub && sub->isVisible()) {
                sub->draw(target);
            }
        }
    }

    void hideAll() {
        hide();
        if (parent) parent->hideAll();
    }

private:
    const std::vector<MenuEntry>& m_entries;
    const sf::Font& m_font;
    sf::Vector2f m_position;
    bool visible;
    int hoverIndex;
    std::vector<std::unique_ptr<MenuWidget>> m_submenus;
    MenuWidget* parent;
    std::map<std::string, sf::Texture> iconCache;

    [[nodiscard]] int getItemIndexAt(const sf::Vector2f& mouse) const {
        float y = m_position.y;
        for (size_t i = 0; i < m_entries.size(); ++i) {
            if (m_entries[i].type == MenuEntryType::Separator) {
                y += 8.f;
                continue;
            }
            sf::FloatRect rect(m_position.x, y, 150.f, 32.f);
            if (rect.contains(mouse))
                return static_cast<int>(i);
            y += 32.f;
        }
        return -1;
    }

    [[nodiscard]] sf::Vector2f getSubmenuPosition(size_t index) const {
        float y = m_position.y;
        for (size_t i = 0; i < index; ++i) {
            if (m_entries[i].type == MenuEntryType::Separator)
                y += 8.f;
            else
                y += 32.f;
        }
        return { m_position.x + 150.f - 4.f, y };
    }

    sf::Texture& getIcon(const std::string& path) {
        auto it = iconCache.find(path);
        if (it != iconCache.end()) return it->second;
        sf::Texture tex;
        sf::Image img;
        // 仅支持 Windows，直接用 _wfopen 方式
        std::wstring wpath = sf::String::fromUtf8(path.begin(), path.end()).toWideString();
        if (FILE* fp = _wfopen(wpath.c_str(), L"rb")) {
            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            std::vector<char> buffer(size);
            fread(buffer.data(), 1, size, fp);
            fclose(fp);
            if (img.loadFromMemory(buffer.data(), buffer.size())) {
                tex.loadFromImage(img);
            } else {
                tex.create(24, 24);
                img.create(24, 24, sf::Color::Red);
                tex.update(img);
            }
        } else {
            tex.create(24, 24);
            img.create(24, 24, sf::Color::Red);
            tex.update(img);
        }
        iconCache[path] = tex;
        return iconCache[path];
    }
};

int main() {
    system("chcp 65001");

    sf::RenderWindow window(sf::VideoMode(480, 320), "SFML ContextMenu Demo", sf::Style::Close);
    window.setFramerateLimit(60);

    // 加载字体
    sf::Font font;
    if (!font.loadFromFile("./source/font/Lolita.ttf")) {
        return 1;
    }

    // 菜单结构示例（传递图片路径，支持中文）
    MenuModel model;
    model.addAction("新建", "./source/icon/top.png", [](){ std::puts("新建被点击"); });
    model.addAction("打开", "./source/icon/web.png", [](){ std::puts("打开被点击"); });
    model.addSeparator();
    model.addSubMenu("导出", "./source/icon/skin.png", {
        MenuEntry::Action("导出为PNG", "./source/image/弃土花开.png", [](){ std::puts("导出为PNG"); }),
        MenuEntry::Action("导出为ICO", "./source/image/寄自奥格尼斯科.png", [](){ std::puts("导出为ICO"); }),
        MenuEntry::Separator(),
        MenuEntry::Action("高级导出", "./source/icon/pack.png", [](){ std::puts("高级导出"); })
    });
    model.addSeparator();
    model.addSubMenu("导出", "./source/icon/skin.png", {
        MenuEntry::Action("导出为PNG", "./source/image/寰宇独奏.png", [](){ std::puts("导出为PNG"); }),
        MenuEntry::Action("导出为ICO", "./source/image/远行前的野餐.png", [](){ std::puts("导出为ICO"); }),
        MenuEntry::Separator(),
        MenuEntry::Action("高级导出", "./source/icon/view.png", [](){ std::puts("高级导出"); })
    });
    model.addSeparator();
    model.addAction("退出", "./source/icon/quit.png", [](){ std::puts("退出被点击"); });

    // 创建菜单控件
    MenuWidget menu(model.getEntries(), font);

    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
                menu.show({ static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y) });
            }
            menu.handleEvent(event, window);
        }

        window.clear(sf::Color(30, 30, 30));
        menu.draw(window);
        window.display();
    }
    return 0;
}
