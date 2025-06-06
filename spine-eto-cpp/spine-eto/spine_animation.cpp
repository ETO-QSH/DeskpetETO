#include <spine/spine-sfml.h>

#include <spine/Atlas.h>
#include <spine/SkeletonJson.h>
#include <spine/SkeletonBinary.h>
#include <spine/AnimationState.h>
#include <spine/AnimationStateData.h>

#include <iostream>

#include "spine_animation.h"
#include "console_colors.h"

using namespace spine;

SpineAnimation::SpineAnimation(int width, int height)
    : windowWidth(width), windowHeight(height), defaultMixTime(0.2f) {
    // 可扩展GLFW光标等
}

// --- 加载动画文件 ---
SpineLoadInfo SpineAnimation::loadFromBinary(const std::string& atlasFile, const std::string& skelFile) {
    return loadImpl(atlasFile, skelFile, false);
}
SpineLoadInfo SpineAnimation::loadFromJson(const std::string& atlasFile, const std::string& jsonFile) {
    return loadImpl(atlasFile, jsonFile, true);
}

// --- 统一加载实现 ---
SpineLoadInfo SpineAnimation::loadImpl(const std::string& atlasPath, const std::string& skeletonPath, bool isJson) {
    SpineLoadInfo info;
    info.atlas = std::make_shared<spine::Atlas>(atlasPath.c_str(), new SFMLTextureLoader());
    if (info.atlas->getPages().size() == 0) {
        std::cout << CONSOLE_BRIGHT_RED << "Atlas load error: " << atlasPath << CONSOLE_RESET << std::endl;
        info.atlas.reset();
        return info;
    }
    std::cout << CONSOLE_BRIGHT_GREEN << "Atlas load down!" << CONSOLE_RESET << std::endl;

    if (isJson) {
        SkeletonJson json(info.atlas.get());
        json.setScale(1.0f);
        info.skeletonData.reset(json.readSkeletonDataFile(skeletonPath.c_str()));
        if (!info.skeletonData) {
            std::cout << CONSOLE_BRIGHT_RED << "JSON load error: " << json.getError().buffer() << CONSOLE_RESET << std::endl;
            info.atlas.reset();
            return info;
        }
    } else {
        SkeletonBinary binary(info.atlas.get());
        binary.setScale(1.0f);
        info.skeletonData.reset(binary.readSkeletonDataFile(skeletonPath.c_str()));
        if (!info.skeletonData) {
            std::cout << CONSOLE_BRIGHT_RED << "Binary load error: " << binary.getError().buffer() << CONSOLE_RESET << std::endl;
            info.atlas.reset();
            return info;
        }
    }
    std::cout << CONSOLE_BRIGHT_GREEN << "Skeleton load down!" << CONSOLE_RESET << std::endl;

    auto& anims = info.skeletonData->getAnimations();
    for (size_t i = 0; i < anims.size(); ++i) {
        auto* animation = anims[i];
        info.animations.emplace_back(animation->getName().buffer());
        info.animationsWithDuration[animation->getName().buffer()] = animation->getDuration();
    }

    std::cout << CONSOLE_BRIGHT_MAGENTA << "Animation Durations:" << CONSOLE_RESET << std::endl;
    for (const auto& anim : info.animationsWithDuration) {
        std::cout << CONSOLE_BRIGHT_MAGENTA << "- " << anim.first << ": " << anim.second << "s" << CONSOLE_RESET << std::endl;
    }

    info.valid = true;
    return info;
}

// --- 设置全局混合时间 ---
void SpineAnimation::setGlobalMixTime(float delay) {
    defaultMixTime = delay;
    if (stateData) {
        stateData->setDefaultMix(delay);
    }
}

// --- 设置动画位置 ---
void SpineAnimation::setPosition(float pos_x, float pos_y) {
    position = {pos_x, pos_y};
    if (drawable) {
        // 以底边为x轴，y轴为动画中线
        // SFML窗口左上为(0,0)，spine骨架y向上，窗口y向下
        float ySpine = static_cast<float>(windowHeight) - pos_y;
        drawable->skeleton->setX(pos_x);
        drawable->skeleton->setY(ySpine);
        drawable->skeleton->updateWorldTransform();
    }
}

// --- 缩放和翻转控制 ---
void SpineAnimation::setFlip(bool newFlipX, bool newFlipY) {
    if (newFlipX) flipX = !flipX;
    if (newFlipY) flipY = !flipY;
    applyTransform();
}

void SpineAnimation::setScale(float s) {
    scale = s;
    applyTransform();
}

void SpineAnimation::applyTransform() {
    if (drawable) {
        float actualScaleX = flipX ? -scale : scale;
        float actualScaleY = flipY ? -scale : scale;
        drawable->skeleton->setScaleX(actualScaleX);
        drawable->skeleton->setScaleY(actualScaleY);
        drawable->skeleton->updateWorldTransform();
    }
}

// --- 动画队列管理 ---
void SpineAnimation::enqueueAnimation(const std::string& anim, float delay) {
    animQueue.emplace(anim, delay);
}

void SpineAnimation::removeAnimation(const std::string& anim) {
    std::queue<std::pair<std::string, float>> newQueue;
    while (!animQueue.empty()) {
        if (animQueue.front().first != anim)
            newQueue.push(animQueue.front());
        animQueue.pop();
    }
    animQueue = std::move(newQueue);
}

void SpineAnimation::clearQueue() {
    std::queue<std::pair<std::string, float>> empty;
    std::swap(animQueue, empty);
}

std::vector<std::string> SpineAnimation::getQueue() const {
    std::vector<std::string> result;
    std::queue<std::pair<std::string, float>> q = animQueue;
    while (!q.empty()) {
        result.push_back(q.front().first);
        q.pop();
    }
    return result;
}

std::string SpineAnimation::getCurrentAnimation() const {
    if (drawable && drawable->state->getCurrent(0))
        return drawable->state->getCurrent(0)->getAnimation()->getName().buffer();
    return "";
}

// --- 临时播放动画 ---
void SpineAnimation::playTemp(const std::string& name, bool loop, float mixDuration) {
    if (drawable) {
        auto* entry = drawable->state->setAnimation(0, name.c_str(), loop);
        if (mixDuration >= 0 && entry) {
            entry->setMixDuration(mixDuration);
        }
        playingTemp = true;
    }
}

// --- 应用加载的数据 ---
void SpineAnimation::apply(const SpineLoadInfo& info) {
    if (info.valid) {
        skeletonData = info.skeletonData;
        atlas = info.atlas;
        stateData = std::make_unique<AnimationStateData>(skeletonData.get());
        drawable = std::make_unique<SkeletonDrawable>(skeletonData.get(), stateData.get());
        drawable->setUsePremultipliedAlpha(true);
        setScale(scale);
        setPosition(position.x, position.y);
        setFlip(flipX, flipY);
        clearQueue();
        playingTemp = false;

        // 设置事件监听器（必须用函数指针或静态成员函数）
        drawable->state->setListener(&SpineAnimation::staticSpineEventCallback);
        // 绑定this到rendererObject
        drawable->state->setRendererObject(this);
    }
}

// 静态事件回调（AnimationStateListener签名）
void SpineAnimation::staticSpineEventCallback(AnimationState* state, EventType type, TrackEntry* entry, Event* event) {
    // 通过rendererObject获取this指针
    auto* self = reinterpret_cast<SpineAnimation*>(state->getRendererObject());
    if (!self || !entry || !entry->getAnimation()) return;
    std::string animationName = entry->getAnimation()->getName().buffer();
    switch (type) {
        case EventType_Start:
            std::cout << CONSOLE_BRIGHT_BLACK << "[START] Animation: " << animationName << CONSOLE_RESET << std::endl;
            break;
        case EventType_Complete:
            std::cout << CONSOLE_BRIGHT_BLACK << "[COMPLETE] Animation: " << animationName << CONSOLE_RESET << std::endl;
            // 动画完成时：优先弹队列，否则播放一次默认动画
            if (!self->animQueue.empty()) {
                auto next = self->animQueue.front();
                self->animQueue.pop();
                auto* newEntry = self->drawable->state->setAnimation(0, next.first.c_str(), false);
                if (newEntry && next.second >= 0)
                    newEntry->setMixDuration(next.second);
            } else if (!self->defaultAnim.empty()) {
                self->drawable->state->setAnimation(0, self->defaultAnim.c_str(), false); // 只播放一次
            }
            break;
        default:
            break;
    }
}

// --- 设置默认动画 ---
void SpineAnimation::setDefaultAnimation(const std::string& anim) {
    defaultAnim = anim;
}

// --- 更新逻辑 ---
void SpineAnimation::update(float dt) {
    if (!drawable) return;
    drawable->update(dt);

    auto* entry = drawable->state->getCurrent(0);

    // 优先处理临时动画
    if (playingTemp) {
        if (entry && !entry->getLoop() && entry->getTrackTime() >= entry->getAnimationEnd()) {
            playingTemp = false;
        } else if (entry) {
            return;
        }
    }

    // 队列动画：如果当前动画结束或没有动画，弹出队列
    if (!entry || (!entry->getLoop() && entry->getTrackTime() >= entry->getAnimationEnd())) {
        if (!animQueue.empty()) {
            auto next = animQueue.front();
            animQueue.pop();
            auto* newEntry = drawable->state->setAnimation(0, next.first.c_str(), false);
            if (newEntry && next.second >= 0)
                newEntry->setMixDuration(next.second);
            return;
        }
        // 队列空，循环播放默认动画
        if (!defaultAnim.empty()) {
            drawable->state->setAnimation(0, defaultAnim.c_str(), true);
        }
    }
}

void SpineAnimation::draw(sf::RenderTarget& target) {
    if (drawable)
        target.draw(*drawable);
}

void SpineAnimation::playNextInQueue() {
    if (!animQueue.empty()) {
        auto next = animQueue.front();
        animQueue.pop();
        drawable->state->setAnimation(0, next.first.c_str(), false);
    } else if (!defaultAnim.empty()) {
        drawable->state->setAnimation(0, defaultAnim.c_str(), true);
    }
}
