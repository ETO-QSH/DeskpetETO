#pragma once
#include <SFML/Graphics.hpp>
#include <spine/spine-sfml.h>
#include <Windows.h>

HRGN BitmapToRgnAlpha(HBITMAP hBmp, BYTE alphaThreshold = 16);
void setClickThrough(HWND hwnd, const sf::Image& image);

void initWindowAndShader();
void initSpineModel();

extern sf::RenderWindow window;
extern HWND hwnd;
extern sf::RenderTexture renderTexture;
extern spine::SkeletonDrawable* drawable;
