#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
using namespace gl;

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

// Windows 亚克力效果支持
#ifdef _WIN32
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

// 全局资源声明
GLFWwindow* window;
GLuint file_tex, folder_tex, gear_tex, exit_tex;
ImFont* main_font = nullptr;

// 样式常量
constexpr float WINDOW_ROUNDING = 10.0f;
constexpr ImVec4 MENU_BG_COLOR(0.08f, 0.08f, 0.08f, 0.92f);
constexpr float ICON_SIZE = 24.0f;

void LoadFonts() {
    ImGuiIO& io = ImGui::GetIO();

    // 主字体（替换为你的字体路径）
    main_font = io.Fonts->AddFontFromFileTTF(
        "D:/Work Files/spine-runtimes/spine-glfw/src/fonts/Lolita.ttf", 18.0f,
        nullptr,
        io.Fonts->GetGlyphRangesChineseFull()
    );

    // 图标字体（可选）
    /*static const ImWchar icon_ranges[] = {0xE000, 0xF8FF, 0};
    ImFontConfig config;
    config.MergeMode = true;
    io.Fonts->AddFontFromFileTTF("fonts/iconfont.ttf", 16.0f, &config, icon_ranges);*/
}

GLuint LoadTexture(const char* path) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, c;
    unsigned char* data = stbi_load(path, &w, &h, &c, 4);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
    }
    return texture;
}

void ApplyModernStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    // 主要样式参数
    style.WindowRounding = WINDOW_ROUNDING;
    style.FrameRounding = 6.0f;
    style.PopupRounding = WINDOW_ROUNDING;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 3.0f;

    // 间距和边距
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(12, 6);
    style.ItemSpacing = ImVec2(8, 6);
    style.ScrollbarSize = 12.0f;

    // 颜色设置
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_PopupBg] = MENU_BG_COLOR;
    colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 0.8f);
    colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 0.8f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
}

void ShowContextMenu() {
    if (ImGui::BeginPopupContextWindow("MainContextMenu", ImGuiPopupFlags_MouseButtonRight)) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 12));

        // 使用表格布局对齐图标和文本
        if (ImGui::BeginTable("##MenuTable", 2,
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Icons", ImGuiTableColumnFlags_WidthFixed, ICON_SIZE + 8);
            ImGui::TableSetupColumn("Items", ImGuiTableColumnFlags_WidthStretch);

            // 文件菜单
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Image((ImTextureID)(intptr_t)file_tex, ImVec2(ICON_SIZE, ICON_SIZE));
            ImGui::TableSetColumnIndex(1);
            if (ImGui::BeginMenu("文件")) {
                if (ImGui::MenuItem("新建", "Ctrl+N")) { /* 新建操作 */ }
                if (ImGui::MenuItem("打开", "Ctrl+O")) { /* 打开操作 */ }
                ImGui::EndMenu();
            }

            // 导出项目
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Image((ImTextureID)(intptr_t)folder_tex, ImVec2(ICON_SIZE, ICON_SIZE));
            ImGui::TableSetColumnIndex(1);
            if (ImGui::MenuItem("导出项目...")) { /* 导出操作 */ }

            // 分隔线
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Dummy(ImVec2(ICON_SIZE, 1));
            ImGui::TableSetColumnIndex(1);
            ImGui::Separator();

            // 设置菜单
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Image((ImTextureID)(intptr_t)gear_tex, ImVec2(ICON_SIZE, ICON_SIZE));
            ImGui::TableSetColumnIndex(1);
            if (ImGui::BeginMenu("设置")) {
                if (ImGui::MenuItem("首选项")) { /* 首选项 */ }
                if (ImGui::BeginMenu("主题")) {
                    ImGui::MenuItem("暗黑主题", nullptr, false);
                    ImGui::MenuItem("明亮主题", nullptr, false);
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            // 退出
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Image((ImTextureID)(intptr_t)exit_tex, ImVec2(ICON_SIZE, ICON_SIZE));
            ImGui::TableSetColumnIndex(1);
            if (ImGui::MenuItem("退出", "Alt+F4")) {
                glfwSetWindowShouldClose(window, true);
            }

            ImGui::EndTable();
        }

        ImGui::PopStyleVar();
        ImGui::EndPopup();
    }
}

void SetupAcrylicEffect() {
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(window);

    // 使用正确的窗口属性值
    const DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
    DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));

    // 正确初始化 MARGINS 结构
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    // 使用现代模糊方法
    DWM_BLURBEHIND bb = {0};
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = TRUE;
    bb.hRgnBlur = nullptr;
    DwmEnableBlurBehindWindow(hwnd, &bb);
#endif
}

int main() {
    // 初始化GLFW
    if (!glfwInit()) return 1;

    // 配置OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); // 透明背景

    // 创建窗口
    window = glfwCreateWindow(1280, 720, "Modern Context Menu", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // 初始化glbinding
    glbinding::initialize([](const char* name) {
        return glfwGetProcAddress(name);
    });

    // 设置亚克力效果
    SetupAcrylicEffect();

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // 加载字体
    LoadFonts();
    ApplyModernStyle();

    // 初始化ImGui后端
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 加载图标（替换为你的图片路径）
    file_tex = LoadTexture("D:/Work Files/spine-runtimes/spine-glfw/src/images/control.png");
    folder_tex = LoadTexture("D:/Work Files/spine-runtimes/spine-glfw/src/images/download.png");
    gear_tex = LoadTexture("D:/Work Files/spine-runtimes/spine-glfw/src/images/view.png");
    exit_tex = LoadTexture("D:/Work Files/spine-runtimes/spine-glfw/src/images/quit.png");

    // 主循环
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // 处理快捷键
        bool ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) ||
                    glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL);
        if (ctrl && glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
            // 处理新建操作
        }
        if (ctrl && glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            // 处理打开操作
        }

        // 开始ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 主界面
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("MainWindow", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBackground);

        // 显示右键菜单
        ShowContextMenu();

        // 帮助文本
        ImGui::SetCursorPos(ImVec2(20, 20));
        ImGui::TextColored(ImVec4(1, 0.5, 0, 1), "右键单击显示现代风格菜单");

        ImGui::End();

        // 渲染
        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0, 0, 0, 0); // 透明背景
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 清理资源
    glDeleteTextures(1, &file_tex);
    glDeleteTextures(1, &folder_tex);
    glDeleteTextures(1, &gear_tex);
    glDeleteTextures(1, &exit_tex);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}