#include <SFML/Graphics.hpp>
#include <spine/spine-sfml.h>

#include <iostream>
#include <windows.h>

#include "console_colors.h"
#include "right_click_menu.h"
#include "spine_animation.h"
#include "spine_win_utils.h"
#include "queue_utils.h"
#include "window_physics.h"

#include "../dependencies/json.hpp"

using namespace spine;

extern nlohmann::json g_modelDatabase;

// 声明全局辉光信号变量
extern bool g_showGlowEffect;
// 声明全局交互半透明信号变量
extern bool g_showHalfAlpha;

static int g_lastWidth = 0, g_lastHeight = 0, g_lastYOffset = 0, g_lastActiveLevel = 0;
static float g_lastMixTime = 0.0f, g_lastScale = 0.0f;
static bool g_hasRecord = false;

HRGN BitmapToRgnAlpha(HBITMAP hBmp, BYTE alphaThreshold) {
    HRGN hRgn = nullptr;
    if (hBmp) {
        BITMAP bm;
        GetObject(hBmp, sizeof(bm), &bm);
        auto* pixels = new DWORD[bm.bmWidth * bm.bmHeight];
        GetBitmapBits(hBmp, bm.bmWidth * bm.bmHeight * 4, pixels);

        #define ALLOC_UNIT 100
        DWORD maxRects = ALLOC_UNIT;
        HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects));
        auto* pData = (RGNDATA*)GlobalLock(hData);
        pData->rdh.dwSize = sizeof(RGNDATAHEADER);
        pData->rdh.iType = RDH_RECTANGLES;
        pData->rdh.nCount = pData->rdh.nRgnSize = 0;
        SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);

        for (int y = 0; y < bm.bmHeight; y++) {
            for (int x = 0; x < bm.bmWidth; x++) {
                int x0 = x;
                while (x < bm.bmWidth) {
                    DWORD color = pixels[(bm.bmHeight - 1 - y) * bm.bmWidth + x];
                    BYTE a = (color >> 24) & 0xFF;
                    if (a < alphaThreshold)
                        break;
                    x++;
                }
                if (x > x0) {
                    if (pData->rdh.nCount >= maxRects) {
                        GlobalUnlock(hData);
                        maxRects += ALLOC_UNIT;
                        hData = GlobalReAlloc(hData, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), GMEM_MOVEABLE);
                        pData = (RGNDATA*)GlobalLock(hData);
                    }
                    RECT* pr = (RECT*)&pData->Buffer;
                    SetRect(&pr[pData->rdh.nCount], x0, y, x, y + 1);
                    if (x0 < pData->rdh.rcBound.left) pData->rdh.rcBound.left = x0;
                    if (y < pData->rdh.rcBound.top) pData->rdh.rcBound.top = y;
                    if (x > pData->rdh.rcBound.right) pData->rdh.rcBound.right = x;
                    if (y + 1 > pData->rdh.rcBound.bottom) pData->rdh.rcBound.bottom = y + 1;
                    pData->rdh.nCount++;
                    if (pData->rdh.nCount == 2000) {
                        HRGN h = ExtCreateRegion(nullptr, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
                        if (hRgn) {
                            CombineRgn(hRgn, hRgn, h, RGN_OR);
                            DeleteObject(h);
                        } else hRgn = h;
                        pData->rdh.nCount = 0;
                        SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
                    }
                }
            }
        }
        HRGN h = ExtCreateRegion(nullptr, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
        if (hRgn) {
            CombineRgn(hRgn, hRgn, h, RGN_OR);
            DeleteObject(h);
        } else hRgn = h;
        GlobalFree(hData);
        delete[] pixels;
    }
    return hRgn;
}

// 替换 setClickThrough，支持半透明（WS_EX_LAYERED + UpdateLayeredWindow 实现）
void setLayeredWindowAlpha(HWND hwnd, const sf::Image& image) {
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    int width = image.getSize().x;
    int height = image.getSize().y;

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hBmp) {
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdcScreen);
        return;
    }

    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    // 修正颜色通道顺序（SFML: RGBA，Windows: BGRA）
    const sf::Uint8* src = image.getPixelsPtr();
    sf::Uint8* dst = static_cast<sf::Uint8*>(bits);
    for (int i = 0; i < width * height; ++i) {
        dst[i * 4 + 0] = src[i * 4 + 2]; // B
        dst[i * 4 + 1] = src[i * 4 + 1]; // G
        dst[i * 4 + 2] = src[i * 4 + 0]; // R
        dst[i * 4 + 3] = src[i * 4 + 3]; // A
    }

    POINT ptSrc = { 0, 0 };
    SIZE sizeWnd = { width, height };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    UpdateLayeredWindow(hwnd, hdcScreen, nullptr, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
}

// 兼容旧接口：setClickThrough 实际调用 setLayeredWindowAlpha
void setClickThrough(HWND hwnd, const sf::Image& image) {
    setLayeredWindowAlpha(hwnd, image);
}

// 手势光标设置
void setHandCursor(bool pressed) {
    static HCURSOR hHand = LoadCursor(nullptr, IDC_HAND);
    static HCURSOR hArrow = LoadCursor(nullptr, IDC_ARROW);
    static HCURSOR hClosed = LoadCursorFromFileW(L"C:\\Windows\\Cursors\\handwriting.ani"); // 可自定义抓取样式
    if (pressed && hClosed)
        SetCursor(hClosed);
    else if (hHand)
        SetCursor(hHand);
    else
        SetCursor(hArrow);
}

// 鼠标消息处理
void handleWindowCursor(HWND hwnd, bool isPressed) {
    POINT pt;
    GetCursorPos(&pt);
    RECT rc;
    GetWindowRect(hwnd, &rc);
    if (PtInRect(&rc, pt)) {
        setHandCursor(isPressed);
    } else {
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
    }
}

// 显示主窗口
void showMainWindow() {
    ShowWindow(hwnd, SW_SHOW);
}

// 全局变量定义
HWND hwnd;
sf::RenderWindow window;
sf::RenderTexture renderTexture;
SkeletonDrawable* drawable = nullptr;
SpineAnimation* animSystem = nullptr;

// 新增：安全释放 SpineAnimation
void freeSpineModel() {
    // 先置空全局 drawable，防止外部访问已释放对象
    drawable = nullptr;
    if (animSystem) {
        delete animSystem;
        animSystem = nullptr;
    }
}

void initWindowAndShader(int width, int height, int offset) {
    window.create(sf::VideoMode(width, height), "DeskpetETO", sf::Style::None);
    window.setFramerateLimit(30); // 由60改为30
    hwnd = window.getSystemHandle();

    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_LAYERED | WS_EX_TOOLWINDOW; // 增加 TOOLWINDOW
    exStyle &= ~WS_EX_APPWINDOW; // 移除 APPWINDOW
    exStyle &= ~WS_EX_TRANSPARENT;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    renderTexture.create(width, height);

    // 获取屏幕工作区（排除任务栏），并打印
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int minX = workArea.left;
    int minY = workArea.top;
    int maxX = workArea.right - width;
    int maxY = workArea.bottom - height + offset;

    // 检查任务栏位置
    APPBARDATA abd = { sizeof(APPBARDATA) };
    abd.hWnd = FindWindowA("Shell_TrayWnd", nullptr);
    int taskbarPos = -1;
    if (abd.hWnd && SHAppBarMessage(ABM_GETTASKBARPOS, &abd)) {
        if (abd.rc.top == 0 && abd.rc.left == 0 && abd.rc.right == screenW) {
            taskbarPos = 1; // top
        } else if (abd.rc.left == 0 && abd.rc.top == screenH - (abd.rc.bottom - abd.rc.top)) {
            taskbarPos = 0; // bottom
        } else if (abd.rc.left == 0 && abd.rc.top == 0 && abd.rc.bottom == screenH) {
            taskbarPos = 2; // left
        } else if (abd.rc.right == screenW && abd.rc.top == 0 && abd.rc.bottom == screenH) {
            taskbarPos = 3; // right
        }
    }

    std::cout << CONSOLE_BRIGHT_WHITE
        << "Work Area: "
        << "minX:" << minX << " "
        << "maxX:" << maxX << " | "
        << "minY:" << minY << " "
        << "maxY:" << maxY
        << " | TaskbarPos: ";
    switch (taskbarPos) {
        case 0: std::cout << "Bottom"; break;
        case 1: std::cout << "Top"; break;
        case 2: std::cout << "Left"; break;
        case 3: std::cout << "Right"; break;
        default: std::cout << "Unknown";
    }
    std::cout << CONSOLE_RESET << std::endl;
}

void initSpineModel(int width, int height, int yOffset, int activeLevel, float mixTime, float Scale) {
    // 记录上次参数的静态变量
    g_lastWidth = width;
    g_lastHeight = height;
    g_lastYOffset = yOffset;
    g_lastActiveLevel = activeLevel;
    g_lastMixTime = mixTime;
    g_lastScale = Scale;
    g_hasRecord = true;

    // 先释放旧对象并置空全局指针
    freeSpineModel();
    // 新建 SpineAnimation
    animSystem = new SpineAnimation(width, height);

    // 动态获取当前皮肤和模型的路径
    extern nlohmann::json g_modelDatabase;
    std::string atlasPath, skelPath;
    do {
        if (!g_modelDatabase.contains("default") || !g_modelDatabase.contains("library")) break;
        const auto& def = g_modelDatabase["default"];
        if (!def.is_array() || def.size() < 2) break;
        const auto& lib = g_modelDatabase["library"];
        if (!lib.contains(def[0])) break;
        const auto& skinObj = lib[def[0]];
        if (!skinObj.contains(def[1])) break;
        const auto& modelObj = skinObj[def[1]];
        if (!modelObj.contains("atlas") || !modelObj.contains("skel")) break;
        atlasPath = modelObj["atlas"].get<std::string>();
        skelPath = modelObj["skel"].get<std::string>();
    } while (false);

    if (atlasPath.empty() || skelPath.empty()) {
        std::cout << CONSOLE_BRIGHT_RED << "模型路径未找到，请检查 package.json" << CONSOLE_RESET << std::endl;
        return;
    }

    // 加载资源
    auto info = SpineAnimation::loadFromBinary(
        atlasPath,
        skelPath
    );

    // 获取运动方向决定朝向
    int walkDir = getWalkDirection();

    if (info.valid) {
        animSystem->clearQueue();
        animSystem->apply(info, activeLevel);
        animSystem->setGlobalMixTime(mixTime);
        animSystem->setDefaultAnimation("Move");
        animSystem->setScale(Scale);
        animSystem->setFlip(walkDir == -1, false);
        animSystem->setPosition(width / 2.0f, yOffset);
        animSystem->playTemp("Interact");

        // 清空显示状态
        g_showGlowEffect = false;
        g_showHalfAlpha = false;

        extern MenuWidgetWithHide* g_contextMenu;
        if (g_contextMenu) {
            setFirstTriToggleToZero(reinterpret_cast<MenuWidget*>(g_contextMenu));
        }

        // 自动队列生成
        ActiveParams params = getActiveParams(activeLevel);
        std::vector<std::string> animQueue = generateRandomAnimQueue(
            params.relaxToMoveRatio, params.specialRatio, 128, info.animationsWithDuration);

        // 取中间33-96（64个）插入队列（包含Turn）
        int start = 33, end = 97;
        if (animQueue.size() < static_cast<size_t>(end)) end = static_cast<int>(animQueue.size());
        std::vector initialQueue(animQueue.begin() + start, animQueue.begin() + end);
        for (const auto& anim : initialQueue) {
            animSystem->enqueueAnimation(anim);
        }

        // 继承Turn计数
        int turnMiss = 0;
        for (int i = end - 1; i >= start; --i) {
            if (animQueue[i] == "Turn") break;
            ++turnMiss;
        }
        setTurnMissCount(turnMiss);
    }

    // 重新赋值全局 drawable 指针
    drawable = animSystem ? animSystem->getDrawable() : nullptr;
}

// 无参数重载，自动用上次参数
void reinitSpineModel() {
    if (!g_hasRecord) return;
    freeSpineModel();
    initSpineModel(g_lastWidth, g_lastHeight, g_lastYOffset, g_lastActiveLevel, g_lastMixTime, g_lastScale);
}

// 更高效的辉光实现：只遍历一次像素，利用距离变换思想
sf::Image addGlowToAlphaEdge(const sf::Image& src, sf::Color glowColor, int glowWidth) {
    sf::Vector2u size = src.getSize();
    sf::Image result;
    result.create(size.x, size.y, sf::Color::Transparent);

    // 先拷贝原图
    result.copy(src, 0, 0);

    // 记录每个像素到最近不透明像素的距离（初始化为大于glowWidth）
    std::vector dist(size.x, std::vector<int>(size.y, glowWidth + 1));

    // 先标记所有不透明像素为0
    for (unsigned x = 0; x < size.x; ++x)
        for (unsigned y = 0; y < size.y; ++y)
            if (src.getPixel(x, y).a > 0)
                dist[x][y] = 0;

    // 两次扫描（近似距离变换，4邻域曼哈顿距离）
    // 正向
    for (unsigned y = 0; y < size.y; ++y) {
        for (unsigned x = 0; x < size.x; ++x) {
            if (x > 0)
                dist[x][y] = std::min(dist[x][y], dist[x - 1][y] + 1);
            if (y > 0)
                dist[x][y] = std::min(dist[x][y], dist[x][y - 1] + 1);
        }
    }
    // 反向
    for (int y = static_cast<int>(size.y) - 1; y >= 0; --y) {
        for (int x = static_cast<int>(size.x) - 1; x >= 0; --x) {
            if (x + 1 < static_cast<int>(size.x))
                dist[x][y] = std::min(dist[x][y], dist[x + 1][y] + 1);
            if (y + 1 < static_cast<int>(size.y))
                dist[x][y] = std::min(dist[x][y], dist[x][y + 1] + 1);
        }
    }

    // 绘制辉光
    for (unsigned x = 0; x < size.x; ++x) {
        for (unsigned y = 0; y < size.y; ++y) {
            int d = dist[x][y];
            if (src.getPixel(x, y).a == 0 && d > 0 && d <= glowWidth) {
                sf::Color c = glowColor;
                c.a = glowColor.a * (glowWidth - d + 1) / (glowWidth + 1); // 边缘渐变
                result.setPixel(x, y, c);
            }
        }
    }
    return result;
}
