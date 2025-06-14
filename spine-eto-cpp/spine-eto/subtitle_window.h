#pragma once

#include <windows.h>

// 获取弹幕窗口 HWND
HWND getSubtitleHwnd();

// 初始化弹幕窗口，参数为字幕持续时间、最大字幕数、窗口宽度
void launchSubtitleWindow(float subtitleDuration, size_t maxSubtitles, int modeWidth);

// 关闭弹幕窗口
void closeSubtitleWindow();
