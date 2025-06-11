#pragma once

#include <Windows.h>

// 物理状态
struct WindowPhysicsState {
    float vx = 0.0f;
    float vy = 0.0f;
    float lastX = 0.0f;
    float lastY = 0.0f;
    bool isDragging = false;
    bool locked = false;
};

// 工作区信息
struct WindowWorkArea {
    int minX, maxX, minY, maxY;
    int width, height;
};

// 物理更新
void updateWindowPhysics(HWND hwnd, WindowPhysicsState& state, const WindowWorkArea& area, float speed, float gravity, float dt);

// 重力开关
void setGravityEnabled(bool enabled);
bool isGravityEnabled();

// 方向翻转接口
void setWalkDirection();
int getWalkDirection();
