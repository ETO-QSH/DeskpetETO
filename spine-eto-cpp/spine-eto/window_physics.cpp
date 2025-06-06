#include <Windows.h>
#include <algorithm>
#include <cmath>

#include "spine_animation.h"

// 物理参数
constexpr float GRAVITY = 1800.0f;      // px/s^2
constexpr float HORIZ_DECAY = 0.85f;    // 水平速度衰减系数
constexpr float DT_LIMIT = 0.05f;       // 最大dt，防止卡顿穿透
constexpr float TOP_BOUNCE_GRAVITY = 2.0f; // 顶部反弹加速度倍数

struct WindowPhysicsState {
    float vx = 0.0f;
    float vy = 0.0f;
    float lastX = 0.0f;
    float lastY = 0.0f;
    bool isDragging = false;
};

struct WindowWorkArea {
    int minX, maxX, minY, maxY;
    int width, height;
};

// 步行参数
constexpr float WALK_SPEED = 100.0f; // px/s

// 新增步行状态
static bool walkEnabled = true;
static int walkDirection = 1; // 1=右，-1=左

void setWalkEnabled(bool enabled) {
    walkEnabled = enabled;
}

void setWalkDirection(int dir) {
    walkDirection = (dir >= 0) ? 1 : -1;
}

bool isWalkEnabled() {
    return walkEnabled;
}

int getWalkDirection() {
    return walkDirection;
}

// 外部声明，需加上类型声明头文件
extern SpineAnimation* animSystem;

static float getCurrentScale() {
    return animSystem ? animSystem->getScale() : 1.0f;
}

void updateWindowPhysics(HWND hwnd, WindowPhysicsState& state, const WindowWorkArea& area, float dt) {
    if (state.isDragging) return; // 拖动时不应用物理

    dt = std::min(dt, DT_LIMIT);

    // 获取当前窗口位置
    RECT rc;
    GetWindowRect(hwnd, &rc);
    auto x = static_cast<float>(rc.left);
    auto y = static_cast<float>(rc.top);

    // 应用重力
    state.vy += GRAVITY * dt;

    // 步行逻辑：只有接触底边且不在拖动状态且开启步行时
    float scale = getCurrentScale();
    if (walkEnabled && std::abs(y - static_cast<float>(area.maxY)) < 0.5f) {
        state.vx = WALK_SPEED * static_cast<float>(walkDirection) * scale;
    }

    // 位置更新
    x += state.vx * dt;
    y += state.vy * dt;

    // 边界检测与反弹
    bool hitLeft = false, hitRight = false, hitBottom = false, hitTop = false;

    if (x < static_cast<float>(area.minX)) {
        x = static_cast<float>(area.minX);
        hitLeft = true;
    }
    if (x > static_cast<float>(area.maxX)) {
        x = static_cast<float>(area.maxX);
        hitRight = true;
    }
    if (y > static_cast<float>(area.maxY)) {
        y = static_cast<float>(area.maxY);
        state.vy = 0.0f;
        hitBottom = true;
    }
    if (y < static_cast<float>(area.minY)) {
        state.vy += GRAVITY * TOP_BOUNCE_GRAVITY * dt;
        state.vx *= HORIZ_DECAY;
        y = static_cast<float>(area.minY);
        hitTop = true;
    }

    // 步行到边界时自动反向
    if (walkEnabled && hitLeft && walkDirection == -1 && animSystem) {
        animSystem->setFlip(true, false);
        walkDirection = 1;
    }
    if (walkEnabled && hitRight && walkDirection == 1 && animSystem) {
        animSystem->setFlip(true, false);
        walkDirection = -1;
    }

    // 水平速度衰减
    if (hitTop || hitBottom) { state.vx *= HORIZ_DECAY; }
    if (hitLeft || hitRight) { state.vx = 0.0f; }

    // 应用新位置
    SetWindowPos(hwnd, nullptr,
        static_cast<int>(std::round(x)),
        static_cast<int>(std::round(y)),
        0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    state.lastX = x;
    state.lastY = y;
}

// 拖动限制：窗口始终在工作区内
void clampWindowToWorkArea(HWND hwnd, const WindowWorkArea& area) {
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int x = std::min(std::max(static_cast<int>(rc.left), area.minX), area.maxX);
    int y = std::min(std::max(static_cast<int>(rc.top), area.minY), area.maxY);
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}
