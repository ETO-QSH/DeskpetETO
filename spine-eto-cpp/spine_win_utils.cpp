#include <spine/spine-sfml.h>
#include <memory>

#include "spine_win_utils.h"

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

// 全局变量定义
HWND hwnd;
sf::RenderWindow window;
sf::RenderTexture renderTexture;
SkeletonDrawable* drawable = nullptr;

void initWindowAndShader() {
    window.create(sf::VideoMode(800, 600), "Spine SFML", sf::Style::None);
    window.setFramerateLimit(60);
    hwnd = window.getSystemHandle();

    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_LAYERED;
    exStyle &= ~WS_EX_TRANSPARENT;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    renderTexture.create(800, 600);
}

void initSpineModel() {
    static SFMLTextureLoader textureLoader;
    static std::unique_ptr<Atlas> atlas = std::make_unique<Atlas>("./models/lisa/build_char_358_lisa_lxh#1.atlas", &textureLoader);
    static SkeletonBinary binary(atlas.get());
    static std::shared_ptr<SkeletonData> skeletonData = std::shared_ptr<SkeletonData>(binary.readSkeletonDataFile("./models/lisa/build_char_358_lisa_lxh#1.skel"));

    if (!skeletonData) {
        printf("Error loading skeleton data: %s\n", binary.getError().buffer());
        exit(1);
    }

    static AnimationStateData stateData(skeletonData.get());
    static SkeletonDrawable staticDrawable(skeletonData.get(), &stateData);
    staticDrawable.timeScale = 1.0f;
    staticDrawable.setUsePremultipliedAlpha(true);
    staticDrawable.skeleton->setPosition(400, 500);
    staticDrawable.skeleton->updateWorldTransform();
    staticDrawable.state->setAnimation(0, "Relax", true);

    drawable = &staticDrawable;
}
