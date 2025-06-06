#pragma once
#include <Windows.h>

// 物理状态
struct WindowPhysicsState {
    float vx = 0.0f;
    float vy = 0.0f;
    float lastX = 0.0f;
    float lastY = 0.0f;
    bool isDragging = false;
};

// 工作区信息
struct WindowWorkArea {
    int minX, maxX, minY, maxY;
    int width, height;
};

// 物理更新
void updateWindowPhysics(HWND hwnd, WindowPhysicsState& state, const WindowWorkArea& area, float dt);

// 拖动限制
void clampWindowToWorkArea(HWND hwnd, const WindowWorkArea& area);
