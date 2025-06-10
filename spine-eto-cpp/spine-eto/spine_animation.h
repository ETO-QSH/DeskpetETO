#pragma once

#include <spine/spine-sfml.h>
#include <SFML/Graphics.hpp>

#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

struct SpineLoadInfo {
    bool valid = false;
    std::vector<std::string> animations;
    std::map<std::string, float> animationsWithDuration;
    std::shared_ptr<spine::SkeletonData> skeletonData;
    std::shared_ptr<spine::Atlas> atlas;
};

class SpineAnimation {
public:
    SpineAnimation(int width, int height);

    // 载入二进制
    static SpineLoadInfo loadFromBinary(const std::string& atlasFile, const std::string& skelFile);
    // 载入json
    static SpineLoadInfo loadFromJson(const std::string& atlasFile, const std::string& jsonFile);

    // --- 新增：统一实现声明 ---
    static SpineLoadInfo loadImpl(const std::string& atlasPath, const std::string& skeletonPath, bool isJson);

    // 应用载入的数据
    void apply(const SpineLoadInfo& info, int activeLevel);

    // 设置全局混合时间（秒）
    void setGlobalMixTime(float mixTime);

    // 设置动画位置（底边为x轴，y轴为动画中线）
    void setPosition(float x, float y);

    // 设置缩放（底边不动，自动修正y）
    void setScale(float scale);

    // 设置翻转
    void setFlip(bool flipX, bool flipY);

    // --- 新增：统一应用变换 ---
    void applyTransform();

    // 队列操作
    void enqueueAnimation(const std::string& anim, float mixTime = -1.f);
    void removeAnimation(const std::string& anim);
    void clearQueue();

    [[nodiscard]] std::vector<std::string> getQueue() const;
    [[nodiscard]] std::string getCurrentAnimation() const;

    // 临时播放动画（可循环/单次），立即打断队列
    void playTemp(const std::string& anim, bool loop = false, float mixDuration = -1.0f);

    // 设置默认动画（队列空且无临时动画时循环播放）
    void setDefaultAnimation(const std::string& anim);

    // 每帧更新与绘制
    void update(float dt);
    void draw(sf::RenderTarget& target);

    // 只读指针访问器
    [[nodiscard]] spine::SkeletonDrawable* getDrawable() const { return drawable.get(); }

    // 添加静态事件回调声明
    static void staticSpineEventCallback(spine::AnimationState*, spine::EventType, spine::TrackEntry*, spine::Event*);

    // 存储当前活跃系数
    int activeLevel = 2;

    // 新增：判断当前是否为临时动画
    bool isPlayingTemp() const { return playingTemp; }

private:
    int windowWidth, windowHeight;
    float defaultMixTime;
    std::shared_ptr<spine::SkeletonData> skeletonData;
    std::shared_ptr<spine::Atlas> atlas;
    std::unique_ptr<spine::AnimationStateData> stateData;
    std::unique_ptr<spine::SkeletonDrawable> drawable;
    std::queue<std::pair<std::string, float>> animQueue;
    std::string defaultAnim;
    bool debugVisible = false;
    float scale = 1.0f;
    bool flipX = false, flipY = false;
    sf::Vector2f position = {0, 0};
    bool playingTemp = false;
    std::string tempAnim;
    bool tempLoop = false;
};
