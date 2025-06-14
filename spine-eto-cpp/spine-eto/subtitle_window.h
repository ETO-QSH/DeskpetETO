#pragma once

#include <windows.h>

// 初始化弹幕窗口线程（只需调用一次，进程退出前调用 destroySubtitleWindow 释放）
void initSubtitleWindow(float subtitleDuration, size_t maxSubtitles, int modeWidth);

// 显示弹幕窗口（线程安全，可多次调用）
void showSubtitleWindow();

// 隐藏弹幕窗口（线程安全，可多次调用）
void hideSubtitleWindow();

// 立即销毁弹幕窗口和线程（线程安全，强制关闭窗口）
void forceCloseSubtitleWindow();

// 等待弹幕窗口线程安全退出
void waitSubtitleThreadExit();

// 获取弹幕窗口 HWND（可用于定位窗口）
HWND getSubtitleHwnd();
