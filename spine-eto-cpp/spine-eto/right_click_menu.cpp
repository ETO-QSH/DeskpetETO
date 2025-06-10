#include <cmath>
#include <mutex>
#include <thread>
#include <windows.h>

#include "console_colors.h"
#include "right_click_menu.h"
#include "spine_win_utils.h"
#include "menu_model_utils.h"

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
                if (entry.type == MenuEntryType::Action) {
                    if (entry.callback) {
                        entry.callback();
                    }
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
        float margin = 4.f, margin_d = 12.f;
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
        bg.setFillColor(sf::Color(223, 223, 223, 255));
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
                arrow.setPoint(2, { ax + 10.f, ay });
                arrow.setFillColor(sf::Color(127, 127, 127));
                target.draw(arrow);
            }
            // 新增：切换类型菜单项右侧画圆
            if (entry.type == MenuEntryType::Toggle) {
                sf::CircleShape circle(5.f);
                circle.setOrigin(5.f, 5.f);
                circle.setPosition(m_position.x + width - padding - 6.f, y + itemHeight / 2.f);
                if (entry.toggleState == 0)
                    circle.setFillColor(sf::Color(223, 95, 95)); // 红色
                else
                    circle.setFillColor(sf::Color(95, 223, 95)); // 绿色
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
                    circle.setFillColor(sf::Color::Transparent);
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

    // 新增：获取菜单项引用
    std::vector<MenuEntry>& entries() { return m_entries; }
    const std::vector<MenuEntry>& entries() const { return m_entries; }

    // 新增：刷新指定子菜单内容
    void refreshSubmenu(size_t idx, const std::vector<MenuEntry>& newEntries) {
        if (idx >= m_entries.size()) return;
        if (m_entries[idx].type != MenuEntryType::SubMenu) return;
        m_entries[idx].submenu = newEntries;
        if (m_submenus[idx]) {
            m_submenus[idx] = std::make_unique<MenuWidget>(newEntries, m_font);
            m_submenus[idx]->parent = this;
        }
    }

private:
    std::vector<MenuEntry> m_entries; // 这里去掉const和引用
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

    friend void setFirstTriToggleToZero(MenuWidget* root);
};

// 让MenuWidgetWithHide在main前定义
struct MenuWidgetWithHide : MenuWidget {
    using MenuWidget::MenuWidget;
    std::function<void()> onHideAll;
    void hideAll() {
        MenuWidget::hideAll();
        if (onHideAll) onHideAll();
    }
};

// 内部全局变量（只在本文件使用）
namespace {
    HWND menuHwnd = nullptr;
    MenuWidgetWithHide* menu = nullptr;
    std::thread menuThread;
    std::mutex menuThreadMutex;
}

// 初始化菜单控件和窗口（只初始化一次，后续复用）
MenuWidgetWithHide* initMenu(const MenuModel& model, const sf::Font& font, std::function<void()> onHideAll) {
    if (!menu) { // 只初始化一次
        menu = new MenuWidgetWithHide(model.getEntries(), font);
        menu->onHideAll = std::move(onHideAll);
    }
    return menu;
}

// 修改为线程安全，且避免多线程exit时崩溃
void safeCloseMenuWindow() {
    static std::mutex closeMutex;
    std::lock_guard lock(closeMutex);
    if (menu) menu->hide();
}

// 修改为立即销毁菜单窗口和线程
void forceCloseMenuWindow() {
    static std::mutex closeMutex;
    std::lock_guard lock(closeMutex);
    // 直接关闭菜单窗口（如果存在）
    if (menuHwnd) {
        // 强制关闭窗口
        PostMessage(menuHwnd, WM_CLOSE, 0, 0);
        menuHwnd = nullptr;
    }
    if (menu) menu->hide();
}

// 等待菜单线程安全退出
void waitMenuThreadExit() {
    std::lock_guard lock(menuThreadMutex);
    if (menuThread.joinable()) {
        menuThread.join();
    }
}

// 绘制菜单并处理事件（窗口和RenderTexture全部局部）
void drawMenu(MenuWidget* menuPtr, sf::RenderWindow* parentWindow, const sf::Vector2f& popupPos) {
    sf::RenderWindow menuWindow;
    sf::RenderTexture menuTexture;

    int menuW = 365, menuH = 365;

    menuTexture.create(menuW, menuH);
    menuWindow.create(sf::VideoMode(menuW, menuH), L"Menu", sf::Style::None);
    menuWindow.setFramerateLimit(30); // 由60改为30
    menuHwnd = menuWindow.getSystemHandle();
    LONG exStyle = GetWindowLong(menuHwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED;
    SetWindowLong(menuHwnd, GWL_EXSTYLE, exStyle);

    // 获取工作区
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    // popupPos为窗口坐标，需转换为屏幕坐标
    POINT desktopPos = { static_cast<LONG>(popupPos.x), static_cast<LONG>(popupPos.y) };
    ClientToScreen(parentWindow->getSystemHandle(), &desktopPos);

    // 计算四个方位的可用性
    std::string showCase;
    POINT pt = { 0, 0 };

    // 右上
    if (desktopPos.x + menuW <= workArea.right && desktopPos.y - menuH >= workArea.top) {
        showCase = "right-upon";
        pt.x = desktopPos.x;
        pt.y = desktopPos.y - menuH;
    }
    // 右下
    else if (desktopPos.x + menuW <= workArea.right && desktopPos.y + menuH <= workArea.bottom) {
        showCase = "right-down";
        pt.x = desktopPos.x;
        pt.y = desktopPos.y;
    }
    // 左上
    else if (desktopPos.x - menuW >= workArea.left && desktopPos.y - menuH >= workArea.top) {
        showCase = "left-upon";
        pt.x = desktopPos.x - 145;
        pt.y = desktopPos.y - menuH;
    }
    // 左下
    else if (desktopPos.x - menuW >= workArea.left && desktopPos.y + menuH <= workArea.bottom) {
        showCase = "left-down";
        pt.x = desktopPos.x - 145;
        pt.y = desktopPos.y;
    }

    // 直接用桌面坐标
    SetWindowPos(menuHwnd, HWND_TOPMOST, pt.x, pt.y, menuW, menuH, SWP_SHOWWINDOW);
    menuPtr->show({ 6, 6 });
    printf(CONSOLE_BRIGHT_GREEN "[MENU] %s" CONSOLE_RESET "\n", showCase.c_str());

    while (menuWindow.isOpen()) {
        sf::Event menuEvent{};
        while (menuWindow.pollEvent(menuEvent)) {
            if (menuEvent.type == sf::Event::Closed) {
                safeCloseMenuWindow();
                menuWindow.close();
                return;
            }
            menuPtr->handleEvent(menuEvent, menuWindow);
        }

        menuTexture.clear(sf::Color::Transparent);
        menuPtr->draw(menuTexture);
        menuTexture.display();

        sf::Image img = menuTexture.getTexture().copyToImage();
        setClickThrough(menuHwnd, img);

        menuWindow.clear(sf::Color::Transparent);
        sf::Sprite spr(menuTexture.getTexture());
        menuWindow.draw(spr);
        menuWindow.display();

        if (GetForegroundWindow() != menuHwnd) {
            safeCloseMenuWindow();
            menuWindow.close();
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(0));
    }
}

// 弹出菜单接口
void popupMenu(sf::RenderWindow* parentWindow, const sf::Vector2f& pos, MenuWidgetWithHide* menuPtr) {
    std::lock_guard lock(menuThreadMutex);

    if (!menuPtr) return;

    // 动态刷新两个子菜单内容
    extern SkinCallback g_skinCallback;
    extern ModelCallback g_modelCallback;
    auto skinEntries = getCurrentSkinEntries(g_skinCallback);
    auto modelEntries = getCurrentModelEntries(g_modelCallback);

    for (size_t i = 0; i < menuPtr->entries().size(); ++i) {
        if (menuPtr->entries()[i].text == "切换皮肤" && menuPtr->entries()[i].type == MenuEntryType::SubMenu) {
            menuPtr->refreshSubmenu(i, skinEntries);
        }
        if (menuPtr->entries()[i].text == "切换模型" && menuPtr->entries()[i].type == MenuEntryType::SubMenu) {
            menuPtr->refreshSubmenu(i, modelEntries);
        }
    }

    // 等待上一个菜单线程完全退出
    if (menuThread.joinable()) {
        safeCloseMenuWindow();
        menuThread.join();
    }
    // 启动新线程弹出菜单
    menuThread = std::thread([=]() {
        drawMenu(menu, parentWindow, pos);
    });
    // 不detach，始终join保证线程安全
}

// 设置第一个三态按钮为0（只设置第一个找到的ToggleTri）
void setFirstTriToggleToZero(MenuWidget* root) {
    if (!root) return;
    std::vector<MenuWidget*> stack;
    stack.push_back(root);
    while (!stack.empty()) {
        MenuWidget* widget = stack.back();
        stack.pop_back();
        auto& entries = const_cast<std::vector<MenuEntry>&>(widget->m_entries);
        auto& submenus = widget->m_submenus;
        for (size_t i = 0; i < entries.size(); ++i) {
            auto& entry = entries[i];
            if (entry.type == MenuEntryType::ToggleTri) {
                if (entry.toggleState != 0) {
                    entry.toggleState = 0;
                    if (entry.toggleCallback) entry.toggleCallback(0);
                }
                return;
            }
            if (entry.type == MenuEntryType::SubMenu && submenus[i]) {
                stack.push_back(submenus[i].get());
            }
        }
    }
}
