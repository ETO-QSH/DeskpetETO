#include <spine/spine-sfml.h>

#include <spine/Atlas.h>
#include <spine/AnimationState.h>
#include <spine/AnimationStateData.h>
#include <spine/SkeletonBinary.h>
#include <spine/SkeletonJson.h>

#include <fstream>
#include <iostream>

#include "console_colors.h"
#include "queue_utils.h"
#include "spine_animation.h"
#include "window_physics.h"

using namespace spine;

SpineAnimation::SpineAnimation(int width, int height)
    : windowWidth(width), windowHeight(height), defaultMixTime(0.2f) {
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
    info.atlas = std::make_shared<Atlas>(atlasPath.c_str(), new SFMLTextureLoader());
    if (info.atlas->getPages().size() == 0) {
        std::cout << CONSOLE_BRIGHT_RED << "Atlas load error: " << atlasPath << CONSOLE_RESET << std::endl;
        info.atlas.reset();
        return info;
    }
    std::cout << CONSOLE_BRIGHT_GREEN << "Atlas load down!" << CONSOLE_RESET << std::endl;

    // 检查骨架文件类型
    std::ifstream file(skeletonPath, std::ios::binary);
    if (!file) {
        std::cout << "Failed to open skeleton file: " << skeletonPath << std::endl;
        info.atlas.reset();
        return info;
    }
    std::vector<char> header(10);
    file.read(header.data(), 10);
    if (file.gcount() < 10) {
        std::cout << "Skeleton file is too short: " << skeletonPath << std::endl;
        file.close();
        info.atlas.reset();
        return info;
    }
    // 检查是否是JSON文件
    bool fileIsJson = false;
    if (file.gcount() >= 10) {
        std::string jsonSignature(header.data() + 2, 8); // 第3到10个字节
        if (jsonSignature == "skeleton") {
            fileIsJson = true;
        }
    }
    file.close();

    // 优先以参数isJson为准，否则用文件头判断
    bool useJson = isJson || fileIsJson;

    if (useJson) {
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
    static int flipCount = 0;
    if (newFlipX) {
        flipX = !flipX;
        const char* dir = (flipCount++ % 2 == 0) ? "Left" : "Right";
        printf(CONSOLE_BRIGHT_RED "[FLIP] Face to %s\n" CONSOLE_RESET, dir);
    }
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
        // 先清除当前动画，避免播放结束时触发COMPLETE事件
        drawable->state->clearTracks();
        auto* entry = drawable->state->setAnimation(0, name.c_str(), loop);
        if (mixDuration >= 0 && entry) {
            entry->setMixDuration(mixDuration);
        }
        playingTemp = true;
        tempLoop = loop; // 记录loop参数
        tempAnim = name;
    }
}

// --- 应用加载的数据 ---
void SpineAnimation::apply(const SpineLoadInfo& info, int activeLevel) {
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

        // 存储活跃系数
        this->activeLevel = activeLevel;
        // 设置事件监听器（只能用静态函数指针）
        drawable->state->setListener(&SpineAnimation::staticSpineEventCallback);
        drawable->state->setRendererObject(this);
    }
}

// 静态事件回调（AnimationStateListener签名）
void SpineAnimation::staticSpineEventCallback(AnimationState* state, EventType type, TrackEntry* entry, Event* event) {
    auto* self = reinterpret_cast<SpineAnimation*>(state->getRendererObject());
    if (!self || !entry || !entry->getAnimation()) return;
    int activeLevel = self->activeLevel;
    std::string animationName = entry->getAnimation()->getName().buffer();
    switch (type) {
        case EventType_Start:
            std::cout << CONSOLE_BRIGHT_BLACK << "[START] Animation: " << animationName << CONSOLE_RESET << std::endl;
            break;
        case EventType_Complete:
            std::cout << CONSOLE_BRIGHT_BLACK << "[COMPLETE] Animation: " << animationName << CONSOLE_RESET << std::endl;
            // 优先处理临时动画循环
            if (self->playingTemp) {
                if (self->tempLoop) {
                    auto* newEntry = self->drawable->state->setAnimation(0, self->tempAnim.c_str(), false);
                    if (newEntry) {
                        newEntry->setMixDuration(self->defaultMixTime);
                    }
                    return;
                } else {
                    self->playingTemp = false;
                }
            }
            // 队列补充机制
            if (self->animQueue.size() <= 32) {
                std::queue<std::pair<std::string, float>> qCopy = self->animQueue;
                std::vector<std::string> prefix;
                while (!qCopy.empty()) {
                    prefix.push_back(qCopy.front().first);
                    qCopy.pop();
                }
                if (prefix.size() > 32) prefix.resize(32);
                std::map<std::string, float> animDur;
                if (self->drawable && self->drawable->skeleton) {
                    auto* skelData = self->drawable->skeleton->getData();
                    for (int i = 0; i < skelData->getAnimations().size(); ++i) {
                        auto* anim = skelData->getAnimations()[i];
                        animDur[anim->getName().buffer()] = anim->getDuration();
                    }
                }
                ActiveParams params = getActiveParams(activeLevel);
                auto newQueue = generateRandomAnimQueueWithPrefix(
                    prefix, params.relaxToMoveRatio, params.specialRatio, 64, animDur);
                for (const auto& anim : newQueue) {
                    self->animQueue.push(std::make_pair(anim, -1.f));
                }
            }
            // 动画完成时：优先弹队列，否则播放一次默认动画
            while (!self->animQueue.empty()) {
                auto next = self->animQueue.front();
                self->animQueue.pop();
                if (next.first == "Turn") {
                    self->setFlip(true, false);
                    setWalkDirection();
                } else {
                    auto* newEntry = self->drawable->state->setAnimation(0, next.first.c_str(), false);
                    if (newEntry && next.second >= 0)
                        newEntry->setMixDuration(next.second);
                    return;
                }
            }
            // 队列空，循环播放默认动画
            if (!self->defaultAnim.empty()) {
                self->drawable->state->setAnimation(0, self->defaultAnim.c_str(), true);
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

    // 优先处理临时动画
    auto* entry = drawable->state->getCurrent(0);
    if (playingTemp) {
        if (entry && !entry->getLoop() && entry->getTrackTime() >= entry->getAnimationEnd()) {
            playingTemp = false;
        }
    }
}

void SpineAnimation::draw(sf::RenderTarget& target) {
    if (drawable)
        target.draw(*drawable);
}
