#include <spine/spine-sfml.h>
#include <windows.h>
#include <iostream>

#include "spine_win_utils.h"
#include "spine_animation.h"
#include "console_colors.h"

using namespace spine;

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

void setClickThrough(HWND hwnd, const sf::Image& image) {
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = static_cast<LONG>(image.getSize().x);
    bmi.bmiHeader.biHeight = -static_cast<LONG>(image.getSize().y);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hBmp) return;

    const sf::Uint8* pixels = image.getPixelsPtr();
    auto width = image.getSize().x;
    auto height = image.getSize().y;
    auto* dst = static_cast<sf::Uint8*>(bits);

    for (unsigned y = 0; y < height; ++y) {
        unsigned yDst = height - 1 - y;
        for (unsigned x = 0; x < width; ++x) {
            unsigned idxSrc = (y * width + x) * 4;
            unsigned idxDst = (yDst * width + x) * 4;
            dst[idxDst + 0] = pixels[idxSrc + 2];
            dst[idxDst + 1] = pixels[idxSrc + 1];
            dst[idxDst + 2] = pixels[idxSrc + 0];
            dst[idxDst + 3] = pixels[idxSrc + 3];
        }
    }

    HRGN hRgn = BitmapToRgnAlpha(hBmp, 32);
    SetWindowRgn(hwnd, hRgn, TRUE);

    DeleteObject(hBmp);
    DeleteObject(hRgn);
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

// 鼠标消息处理（需在主消息循环中调用）
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

// 全局变量定义
HWND hwnd;
sf::RenderWindow window;
sf::RenderTexture renderTexture;
SkeletonDrawable* drawable = nullptr;
SpineAnimation* animSystem = nullptr;

void initWindowAndShader() {
    window.create(sf::VideoMode(800, 600), "Spine SFML", sf::Style::None);
    window.setFramerateLimit(60);
    hwnd = window.getSystemHandle();

    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_LAYERED;
    exStyle &= ~WS_EX_TRANSPARENT;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    renderTexture.create(800, 600);

    // 获取屏幕工作区（排除任务栏），并打印
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 800, winH = 600;
    int minX = workArea.left;
    int minY = workArea.top;
    int maxX = workArea.right - winW;
    int maxY = workArea.bottom - winH;

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

void initSpineModel() {
    // 创建动画系统
    static SpineAnimation staticAnimSystem(800, 600);
    animSystem = &staticAnimSystem;

    // 加载资源
    auto info = SpineAnimation::loadFromBinary(
        "./models/lisa/build_char_358_lisa_lxh#1.atlas",
        "./models/lisa/build_char_358_lisa_lxh#1.skel"
    );

    // animSystem 操作区
    if (info.valid) {
        animSystem->apply(info);
        animSystem->setGlobalMixTime(0.2f);
        animSystem->setDefaultAnimation("Relax");
        animSystem->setScale(1.0f);
        animSystem->setFlip(false, false);
        animSystem->setPosition(300.0f, 0.0f);
        animSystem->playTemp("Move");

        animSystem->enqueueAnimation("Interact");
        animSystem->enqueueAnimation("Special");
        animSystem->enqueueAnimation("Move");
        animSystem->enqueueAnimation("Relax");
        animSystem->enqueueAnimation("Move");
    }

    // 兼容原有全局 drawable 指针
    drawable = animSystem->getDrawable();
}
