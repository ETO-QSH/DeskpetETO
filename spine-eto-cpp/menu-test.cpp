#include <SFML/Graphics.hpp>
#include <functional>
#include <utility>
#include <vector>
#include <string>
#include <memory>
#include <cmath>
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
enum class MenuEntryType { Action, Separator, SubMenu, Toggle, ToggleTri };

// 菜单项结构体
struct MenuEntry {
    MenuEntryType type = MenuEntryType::Action; // 默认初始化
    std::string text;
    std::string iconPath;
    std::function<void()> callback;
    std::vector<MenuEntry> submenu;
    int toggleState = 0; // 0/1(二态), 0/1/2(三态)
    std::function<void(const int&)> toggleCallback;
    // 三态颜色
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
    // 新增：切换类型
    static MenuEntry Toggle(const std::string& text, const std::string& iconPath, int initialState, std::function<void(const int&)> cb) {
        MenuEntry e;
        e.type = MenuEntryType::Toggle;
        e.text = text;
        e.iconPath = iconPath;
        e.toggleState = initialState;
        e.toggleCallback = std::move(cb);
        return e;
    }
    // 新增：三态切换
    static MenuEntry ToggleTri(const std::string& text, const std::string& iconPath, int initialState, std::function<void(const int&)> cb) {
        MenuEntry e;
        e.type = MenuEntryType::ToggleTri;
        e.text = text;
        e.iconPath = iconPath;
        e.toggleState = initialState;
        e.toggleCallback = std::move(cb);
        // 蓝、紫、粉
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
            int prevHover = hoverIndex;
            hoverIndex = getItemIndexAt(mousePos);

            // 判断鼠标是否在某个子菜单区域内
            int submenuActive = -1;
            for (size_t i = 0; i < m_submenus.size(); ++i) {
                if (m_submenus[i] && m_submenus[i]->isVisible() && m_submenus[i]->isPointInMenu(mousePos)) {
                    submenuActive = static_cast<int>(i);
                    break;
                }
            }

            // 进入一个新的主菜单项时，主动隐藏所有子菜单
            if (hoverIndex != prevHover && hoverIndex != -1) {
                for (auto& sub : m_submenus) {
                    if (sub) sub->hide();
                }
            }

            // 只显示一个子菜单，其余全部隐藏
            for (size_t i = 0; i < m_entries.size(); ++i) {
                if (m_entries[i].type == MenuEntryType::SubMenu && m_submenus[i]) {
                    if (static_cast<int>(i) == hoverIndex || static_cast<int>(i) == submenuActive) {
                        // 打开子菜单前，先隐藏所有子菜单，保证只显示一个
                        for (auto& sub : m_submenus) {
                            if (sub) sub->hide();
                        }
                        sf::Vector2f subPos = getSubmenuPosition(i);
                        m_submenus[i]->show(subPos);
                        break; // 只显示一个
                    }
                }
            }
            // 鼠标离开主菜单和子菜单区域时，不做隐藏
        }
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });
            int idx = getItemIndexAt(mousePos);
            bool inAnySubMenu = false;
            for (auto& sub : m_submenus) {
                if (sub && sub->isVisible()) {
                    if (sub->isPointInMenu(mousePos)) {
                        inAnySubMenu = true;
                        break;
                    }
                }
            }
            if (idx >= 0 && idx < static_cast<int>(m_entries.size())) {
                auto& entry = const_cast<MenuEntry&>(m_entries[idx]);
                if (entry.type == MenuEntryType::Action && entry.callback) {
                    entry.callback();
                    hideAll();
                    consumed = true;
                }
                if (entry.type == MenuEntryType::SubMenu && m_submenus[idx]) {
                    consumed = true;
                }
                // 切换类型菜单点击：切换状态、打印、收起菜单
                if (entry.type == MenuEntryType::Toggle) {
                    entry.toggleState = 1 - entry.toggleState;
                    if (entry.toggleCallback) entry.toggleCallback(entry.toggleState);
                    hideAll();
                    consumed = true;
                }
                // 三态切换
                if (entry.type == MenuEntryType::ToggleTri) {
                    entry.toggleState = (entry.toggleState + 1) % 3;
                    if (entry.toggleCallback) entry.toggleCallback(entry.toggleState);
                    hideAll();
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
        float margin = 4.f, margin_d = 10.f;
        float itemHeight = 32.f, padding = 8.f;

        // 判断是否是子菜单（有parent即为子菜单）
        bool isSubMenu = parent != nullptr;

        // 计算宽度
        float width;
        if (isSubMenu) {
            float maxTextWidth = 0.f;
            for (const auto& entry : m_entries) {
                if (entry.type == MenuEntryType::Separator) continue;
                sf::Text text;
                text.setFont(m_font);
                text.setCharacterSize(16);
                text.setString(sf::String::fromUtf8(entry.text.begin(), entry.text.end()));
                float textWidth = text.getLocalBounds().width;
                float iconWidth = entry.iconPath.empty() ? 0.f : (32.f);
                float toggleCircle = (entry.type == MenuEntryType::Toggle || entry.type == MenuEntryType::ToggleTri) ? 20.f : 0.f;
                float totalWidth = padding + iconWidth + textWidth + toggleCircle + padding;
                if (entry.type == MenuEntryType::SubMenu) totalWidth += 18.f;
                if (totalWidth > maxTextWidth) maxTextWidth = totalWidth;
            }
            width = maxTextWidth + 4.f;
        } else {
            width = 135.f;
        }

        float height = 0.f;
        for (const auto& entry : m_entries)
            height += (entry.type == MenuEntryType::Separator) ? padding : itemHeight;
        height += margin + margin_d;

        RoundedRectangleShape bg({ width + margin * 2, height }, 10.f, 12);
        bg.setFillColor(sf::Color(223, 223, 223, 223));
        bg.setPosition(m_position.x - margin, m_position.y - margin);
        target.draw(bg);

        float y = m_position.y + margin;
        for (size_t i = 0; i < m_entries.size(); ++i) {
            const auto& entry = m_entries[i];
            float iconSize = 24.f;

            if (entry.type == MenuEntryType::Separator) {
                sf::RectangleShape line({ width - 2 * padding, 2.f });
                line.setFillColor(sf::Color(159, 159, 159, 191));
                line.setPosition(m_position.x + padding, y + 3);
                target.draw(line);
                y += 8.f;
                continue;
            }
            // 圆角高亮
            if (static_cast<int>(i) == hoverIndex) {
                float highlightRadius = 10.f;
                RoundedRectangleShape hi({ width - 8, itemHeight - 2 }, highlightRadius, 8);
                hi.setFillColor(sf::Color(191, 191, 191, 223));
                hi.setPosition(m_position.x + 4, y + 1);
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
            if (entry.type != MenuEntryType::Separator) {
                sf::Text text;
                text.setFont(m_font);
                text.setCharacterSize(16);
                text.setFillColor(sf::Color::Black);
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
                arrow.setFillColor(sf::Color(127, 127, 127));
                target.draw(arrow);
            }
            // 新增：切换类型菜单项右侧画圆
            if (entry.type == MenuEntryType::Toggle) {
                sf::CircleShape circle(5.f);
                circle.setOrigin(5.f, 5.f);
                circle.setPosition(m_position.x + width - padding - 6.f, y + itemHeight / 2.f);
                if (entry.toggleState == 0)
                    circle.setFillColor(sf::Color(223, 63, 63)); // 红色
                else
                    circle.setFillColor(sf::Color(63, 223, 63)); // 绿色
                target.draw(circle);
            }
            // 三态切换类型菜单项右侧画圆
            if (entry.type == MenuEntryType::ToggleTri) {
                sf::CircleShape circle(5.f);
                circle.setOrigin(5.f, 5.f);
                circle.setPosition(m_position.x + width - padding - 6.f, y + itemHeight / 2.f);
                if (!entry.triColors.empty())
                    circle.setFillColor(entry.triColors[entry.toggleState % entry.triColors.size()]);
                else
                    circle.setFillColor(sf::Color::Yellow);
                target.draw(circle);
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
        bool isSubMenu = parent != nullptr;
        float width;
        if (isSubMenu) {
            float maxTextWidth = 0.f;
            for (const auto& entry : m_entries) {
                if (entry.type == MenuEntryType::Separator) continue;
                sf::Text text;
                text.setFont(m_font);
                text.setCharacterSize(16);
                text.setString(sf::String::fromUtf8(entry.text.begin(), entry.text.end()));
                float textWidth = text.getLocalBounds().width;
                float iconWidth = entry.iconPath.empty() ? 0.f : (32.f);
                float toggleCircle = (entry.type == MenuEntryType::Toggle || entry.type == MenuEntryType::ToggleTri) ? 20.f : 0.f;
                float totalWidth = iconWidth + textWidth + toggleCircle + 16.f;
                if (entry.type == MenuEntryType::SubMenu) totalWidth += 18.f;
                if (totalWidth > maxTextWidth) maxTextWidth = totalWidth;
            }
            width = maxTextWidth + 4.f;
        } else {
            width = 135.f;
        }

        for (size_t i = 0; i < m_entries.size(); ++i) {
            if (m_entries[i].type == MenuEntryType::Separator) {
                y += 8.f;
                continue;
            }
            sf::FloatRect rect(m_position.x, y, width, 32.f);
            if (rect.contains(mouse))
                return static_cast<int>(i);
            y += 32.f;
        }
        return -1;
    }

    [[nodiscard]] sf::Vector2f getSubmenuPosition(size_t index) const {
        float y = m_position.y;
        // 子菜单宽度动态，主菜单固定
        bool isSubMenu = parent != nullptr;
        float width;
        if (isSubMenu) {
            float maxTextWidth = 0.f;
            for (const auto& entry : m_entries) {
                if (entry.type == MenuEntryType::Separator) continue;
                sf::Text text;
                text.setFont(m_font);
                text.setCharacterSize(16);
                text.setString(sf::String::fromUtf8(entry.text.begin(), entry.text.end()));
                float textWidth = text.getLocalBounds().width;
                float iconWidth = entry.iconPath.empty() ? 0.f : (32.f);
                float toggleCircle = (entry.type == MenuEntryType::Toggle || entry.type == MenuEntryType::ToggleTri) ? 20.f : 0.f;
                float totalWidth = iconWidth + textWidth + toggleCircle + 16.f;
                if (entry.type == MenuEntryType::SubMenu) totalWidth += 18.f;
                if (totalWidth > maxTextWidth) maxTextWidth = totalWidth;
            }
            width = maxTextWidth + 4.f;
        } else {
            width = 135.f;
        }

        for (size_t i = 0; i < index; ++i) {
            if (m_entries[i].type == MenuEntryType::Separator)
                y += 8.f;
            else
                y += 32.f;
        }
        return { m_position.x + width - 4.f, y };
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

    sf::RenderWindow window(sf::VideoMode(480, 400), "SFML ContextMenu Demo", sf::Style::Close);
    window.setFramerateLimit(60);

    // 加载字体
    sf::Font font;
    if (!font.loadFromFile("./source/font/Lolita.ttf")) {
        return 1;
    }

    // 菜单结构示例
    MenuModel model;
    model.addSubMenu("切换皮肤", "./source/icon/skin.png", {
        MenuEntry::Action("弃土花开", "./source/image/弃土花开.png", [](){ std::puts("弃土花开"); }),
        MenuEntry::Action("寰宇独奏", "./source/image/寰宇独奏.png", [](){ std::puts("寰宇独奏"); }),
        MenuEntry::Action("至高判决", "./source/image/至高判决.png", [](){ std::puts("至高判决"); })
    });
    model.addSubMenu("切换模型", "./source/icon/armor.png", {
        MenuEntry::Action("寄自奥格尼斯科", "./source/image/寄自奥格尼斯科.png", [](){ std::puts("寄自奥格尼斯科"); }),
        MenuEntry::Action("夏卉 FA210", "./source/image/夏卉 FA210.png", [](){ std::puts("夏卉 FA210"); }),
        MenuEntry::Action("远行前的野餐", "./source/image/远行前的野餐.png", [](){ std::puts("远行前的野餐"); })
    });

    model.addSeparator();
    model.getEntries().push_back(MenuEntry::Toggle("窗口置顶", "./source/icon/top.png",
        0, [](const int& state){ std::puts(state ? "置顶:开" : "置顶:关"); }));
    model.getEntries().push_back(MenuEntry::Toggle("位置锁定", "./source/icon/stick.png",
        0, [](const int& state){ std::puts(state ? "锁定:开" : "锁定:关"); }));
    model.getEntries().push_back(MenuEntry::ToggleTri("状态切换", "./source/icon/break.png",
        0, [](const int& state) {
            switch (state) { case 1: std::puts("状态:坐"); break; case 2: std::puts("状态:卧"); break; default: std::puts("状态:站"); }
        }));

    model.addSeparator();
    model.addAction("桌宠收纳", "./source/icon/pack.png", [](){ std::puts("桌宠收纳"); });
    model.addAction("交互透明", "./source/icon/trans.png", [](){ std::puts("交互透明"); });
    model.addAction("应用设置", "./source/icon/control.png", [](){ std::puts("应用设置"); });
    model.addAction("占位符喵", "./source/icon/setting.png", [](){ std::puts("占位符喵"); });

    model.addSeparator();
    model.addAction("销毁退出", "./source/icon/quit.png", [](){ std::puts("销毁退出"); });

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

        window.clear(sf::Color(255, 255, 255));
        menu.draw(window);
        window.display();
    }
    return 0;
}
