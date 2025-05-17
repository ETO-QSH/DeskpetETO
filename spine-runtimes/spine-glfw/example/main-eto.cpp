#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
#include <spine/spine.h>
#include <spine-glfw.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <map>

using namespace gl;
using namespace std;
using namespace spine;

GLFWwindow* init_glfw(int width, int height) {
    if (!glfwInit()) return nullptr;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Spine Example", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glbinding::initialize(glfwGetProcAddress);
    return window;
}

class SpineAnimation {
public:
    struct AnimationInfo {
        bool valid = false;
        vector<pair<string, float>> animationsWithDuration; // 存储动画名称和时长
        SkeletonData* skeletonData = nullptr;
        Atlas* atlas = nullptr;
    };

    // 获取动画队列的拷贝
    vector<pair<string, float>> getAnimationQueue() const {
        vector<pair<string, float>> copy;
        queue<pair<string, float>> temp = animationQueue;
        while (!temp.empty()) {
            copy.push_back(temp.front());
            temp.pop();
        }
        return copy;
    }

    SpineAnimation(int width, int height)
        : windowWidth(width), windowHeight(height), defaultMixTime(0.2f) {
        currentInstance = this; // 设置当前实例指针
    }

    // 1. 加载动画文件
    static AnimationInfo loadFromJson(const string& atlasPath, const string& skeletonPath) {
        return loadImpl(atlasPath, skeletonPath, true);
    }

    static AnimationInfo loadFromBinary(const string& atlasPath, const string& skeletonPath) {
        return loadImpl(atlasPath, skeletonPath, false);
    }

    // 2. 设置全局混合时间
    void setGlobalMixTime(float delay) {
        if(animationStateData) {
            animationStateData->setDefaultMix(delay);
        }
    }

    // 3. 设置动画位置
    void setPosition(float pox_x, float pox_y) {
        if(skeleton) {
            skeleton->setX(pox_x);
            skeleton->setY((float) windowHeight - pox_y);
            skeleton->updateWorldTransform(Physics_Update);
        }
    }

    // 4. 缩放和翻转控制
    void setFlip(bool newFlipX, bool newFlipY) {
        flipX = newFlipX;
        flipY = newFlipY;
        applyTransform();
    }

    void setScale(float scale) {
        this->scale = scale;
        applyTransform();
    }

    // 统一应用变换方法
    void applyTransform() {
        if(skeleton) {
            float actualScaleX = flipX ? -scale : scale;
            float actualScaleY = flipY ? -scale : scale;
            skeleton->setScaleX(actualScaleX);
            skeleton->setScaleY(actualScaleY);
            skeleton->updateWorldTransform(Physics_Update);
        }
    }

    // 5. 动画队列管理
    void enqueueAnimation(const string& name, float delay = 0.0f) {
        animationQueue.emplace(name, delay);
    }

    void clearQueue() {
        queue<pair<string, float>> empty;
        swap(animationQueue, empty);
    }

    string getCurrentAnimation() const {
        return currentTrackEntry ? currentTrackEntry->getAnimation()->getName().buffer() : "";
    }

    // 6. 临时播放动画
    void playTemp(const string& name, bool loop = false, float mixDuration = -1.0f) {
        if(animationState) {
            currentTrackEntry = animationState->setAnimation(0, name.c_str(), loop);
            if(mixDuration >= 0) {
                currentTrackEntry->setMixDuration(mixDuration);
            }
        }
    }

    // 7. 应用加载的数据
    void apply(const AnimationInfo& info) {
        if(info.valid) {
            reset();

            // 初始化动画时长映射
            animationDurations.clear();
            for(const auto& anim : info.animationsWithDuration) {
                animationDurations[anim.first] = anim.second;
            }

            skeleton = new Skeleton(info.skeletonData);
            animationStateData = new AnimationStateData(info.skeletonData);
            animationState = new AnimationState(animationStateData);

            // 设置事件监听器
            animationState->setListener(staticCallback);

            animationStateData->setDefaultMix(defaultMixTime);

            setPosition((float) windowWidth/2, 0);
            setScale(1.0f);
        }
    }

    // 8. 设置默认动画
    void setDefaultAnimation(const string& name) {
        defaultAnimation = name;
    }

    // 更新逻辑
    void update(float deltaTime) {
        if(animationState && skeleton) {
            animationState->update(deltaTime);
            animationState->apply(*skeleton);

            handleAnimationQueue();
            handleDefaultAnimation();

            skeleton->update(deltaTime);
            skeleton->updateWorldTransform(Physics_Update);
        }
    }

    // 渲染
    void render(renderer_t* renderer) {
        if(skeleton && renderer) {
            renderer_draw(renderer, skeleton, true);
        }
    }

    ~SpineAnimation() {
        // 清理当前实例指针
        if (currentInstance == this) {
            currentInstance = nullptr;
        }
        reset();
    }

private:
    static SpineAnimation* currentInstance;

    // 静态回调函数
    static void staticCallback(AnimationState* state, EventType type, TrackEntry* entry, Event* event) {
        if (currentInstance) {
            handleEvent(state, type, entry, event);
        }
    }

    // 事件处理函数
    static void handleEvent(AnimationState* state, EventType type, TrackEntry* entry, Event* event) {
        if (!entry || !entry->getAnimation()) return;

        const string animationName = entry->getAnimation()->getName().buffer();
        switch (type) {
            case EventType_Start:
                cout << "[START] Animation: " << animationName << endl;
            break;
            case EventType_Complete:
                cout << "[COMPLETE] Animation: " << animationName << endl;
            break;
            default:
                break;
        }
    }

    map<string, float> animationDurations; // 存储动画时长

    void reset() {
        delete skeleton;
        delete animationState;
        delete animationStateData;

        skeleton = nullptr;
        animationState = nullptr;
        animationStateData = nullptr;
        currentTrackEntry = nullptr;
    }

    void handleAnimationQueue() {
        if(!animationQueue.empty() && (!currentTrackEntry || currentTrackEntry->isComplete())) {
            auto& next = animationQueue.front();
            currentTrackEntry = animationState->setAnimation(0, next.first.c_str(), false);
            currentTrackEntry->setMixDuration(next.second);
            animationQueue.pop();
        }
    }

    void handleDefaultAnimation() {
        if(animationQueue.empty() && (!currentTrackEntry || currentTrackEntry->isComplete()) && !defaultAnimation.empty()) {
            currentTrackEntry = animationState->setAnimation(0, defaultAnimation.c_str(), true);
        }
    }

    static AnimationInfo loadImpl(const string& atlasPath, const string& skeletonPath, bool isJson) {
        AnimationInfo info;

        // 加载Atlas
        info.atlas = new Atlas(atlasPath.c_str(), new GlTextureLoader());
        if(info.atlas->getPages().size() == 0) {
            cout << "Atlas load error: " << atlasPath << endl;
            delete info.atlas;
            return info;
        }
        cout << "Atlas load down!" << endl;

        // 加载SkeletonData
        if(isJson) {
            SkeletonJson json(info.atlas);
            json.setScale(1.0f);
            info.skeletonData = json.readSkeletonDataFile(skeletonPath.c_str());
            if(!info.skeletonData) {
                cout << "JSON load error: " << json.getError().buffer() << endl;
            }
        } else {
            SkeletonBinary binary(info.atlas);
            binary.setScale(1.0f);
            info.skeletonData = binary.readSkeletonDataFile(skeletonPath.c_str());
            if(!info.skeletonData) {
                cout << "Binary load error: " << binary.getError().buffer() << endl;
            }
        }

        if(!info.skeletonData) {
            delete info.atlas;
            return info;
        }
        cout << "Skeleton load down!" << endl;

        // 收集动画列表+时长信息
        auto& anims = info.skeletonData->getAnimations();
        info.animationsWithDuration.reserve(anims.size());
        for(size_t i = 0; i < anims.size(); ++i) {
            auto* animation = anims[i];
            info.animationsWithDuration.emplace_back(
                animation->getName().buffer(),
                animation->getDuration()
            );
        }

        info.valid = true;
        return info;
    }

    // 成员变量
    int windowWidth;
    int windowHeight;
    float defaultMixTime;
    bool flipX = false;
    bool flipY = false;
    float scale = 1.0;

    Skeleton* skeleton = nullptr;
    AnimationState* animationState = nullptr;
    AnimationStateData* animationStateData = nullptr;
    TrackEntry* currentTrackEntry = nullptr;

    queue<pair<string, float>> animationQueue;
    string defaultAnimation;

public:
    float getAnimationDuration(const string& name) const {
        auto it = animationDurations.find(name);
        return it != animationDurations.end() ? it->second : 0.0f;
    }
};

// 初始化静态成员
SpineAnimation* SpineAnimation::currentInstance = nullptr;

int main() {
    const int WIDTH = 800;
    const int HEIGHT = 600;

    // 初始化窗口
    GLFWwindow* window = init_glfw(WIDTH, HEIGHT);
    if (!window) return -1;

    // 创建渲染器
    renderer_t* renderer = renderer_create();
    renderer_set_viewport_size(renderer, WIDTH, HEIGHT);

    // 创建动画系统
    SpineAnimation animSystem(WIDTH, HEIGHT);

    // 加载资源
    auto info = SpineAnimation::loadFromBinary(
        "data/official/spineboy-pma.atlas",
        "data/official/spineboy-pro.skel"
    );

    // "data/amy/build_char_1037_amiya3_sale_13.atlas"
    // "data/amy/build_char_1037_amiya3_sale_13.skel"

    cout << "\nAnimation Durations:" << endl;
    for(const auto& anim : info.animationsWithDuration) {
        cout << "- " << anim.first << ": " << anim.second << "s" << endl;
    }

    // animSystem 操作区
    if(info.valid) {
        animSystem.apply(info);
        animSystem.setGlobalMixTime(0.2f);
        animSystem.setDefaultAnimation("idle");
        animSystem.enqueueAnimation("run", 0.2f);
        animSystem.enqueueAnimation("idle", 0.2f);
        animSystem.enqueueAnimation("run", 0.2f);
        animSystem.setScale(0.6);
        animSystem.setFlip(false, true);
        animSystem.setPosition((float) WIDTH/2, 50.0);
        animSystem.playTemp("portal");
    }

    // 主循环
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currTime = glfwGetTime();
        auto delta = static_cast<float>(currTime - lastTime);
        lastTime = currTime;

        animSystem.update(delta);

        glClear(GL_COLOR_BUFFER_BIT);
        animSystem.render(renderer);
        // cout << animSystem.getCurrentAnimation() << endl;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理资源
    renderer_dispose(renderer);
    glfwTerminate();
    return 0;
}
