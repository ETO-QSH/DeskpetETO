#pragma once

#include <SFML/Graphics.hpp>

#include <windows.h>

// 向前声明，避免头文件循环依赖和不必要的包含
namespace spine {
    class SkeletonDrawable;
}

// 全局变量声明
extern HWND hwnd;
extern sf::RenderWindow window;
extern sf::RenderTexture renderTexture;
extern spine::SkeletonDrawable* drawable;

HRGN BitmapToRgnAlpha(HBITMAP hBmp, BYTE alphaThreshold = 16);
void setClickThrough(HWND hwnd, const sf::Image& image);

// 辉光效果函数声明
sf::Image addGlowToAlphaEdge(const sf::Image& src, sf::Color glowColor, int glowWidth = 4);

void initWindowAndShader(int width, int height, int offset);
void initSpineModel(int width, int height, int yOffset, int activeLevel, float mixTime, float Scale);
void reinitSpineModel();
