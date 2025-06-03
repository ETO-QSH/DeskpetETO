#define SPINE_SHORT_NAMES
#include <spine/spine-sfml.h>
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <fstream>
#include <vector>

using namespace spine;

// 凸包算法（Andrew's Monotone Chain）
static int cross(const sf::Vector2i& O, const sf::Vector2i& A, const sf::Vector2i& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}
static std::vector<sf::Vector2i> convexHull(std::vector<sf::Vector2i> P) {
    int n = P.size(), k = 0;
    if (n <= 1) return P;
    std::sort(P.begin(), P.end(), [](auto& a, auto& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
        });
    std::vector<sf::Vector2i> H(2 * n);
    for (int i = 0; i < n; ++i) {
        while (k >= 2 && cross(H[k - 2], H[k - 1], P[i]) <= 0) k--;
        H[k++] = P[i];
    }
    for (int i = n - 2, t = k + 1; i >= 0; --i) {
        while (k >= t && cross(H[k - 2], H[k - 1], P[i]) <= 0) k--;
        H[k++] = P[i];
    }
    H.resize(k - 1);
    return H;
}

inline void updateMinMax(float x, float y, float& minX, float& minY, float& maxX, float& maxY) {
    if (x < minX) minX = x;
    if (y < minY) minY = y;
    if (x > maxX) maxX = x;
    if (y > maxY) maxY = y;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: %s <skeleton.skel> <atlas.atlas> <output.png>\n", argv[0]);
        return 1;
    }

    // 加载资源
    const char* skeletonPath = argv[1];
    const char* atlasPath = argv[2];
    const char* outputPath = argv[3];

    Atlas* atlas = Atlas_createFromFile(atlasPath, 0);
    if (!atlas) {
        printf("Failed to load atlas: %s\n", atlasPath);
        return 2;
    }

    // 检查骨架文件类型
    std::ifstream file(skeletonPath, std::ios::binary);
    if (!file) {
        printf("Failed to open skeleton file: %s\n", skeletonPath);
        Atlas_dispose(atlas);
        return 3;
    }

    // 读取前10个字节
    std::vector<char> header(10);
    file.read(header.data(), 10);
    if (file.gcount() < 10) {
        printf("Skeleton file is too short: %s\n", skeletonPath);
        file.close();
        Atlas_dispose(atlas);
        return 4;
    }

    // 检查是否是JSON文件
    bool isJson = false;
    if (file.gcount() >= 10) {
        std::string jsonSignature(header.data() + 2, 8); // 第3到10个字节
        if (jsonSignature == "skeleton") {
            isJson = true;
        }
    }

    SkeletonData* skeletonData = nullptr;

    if (isJson) {
        // 使用JSON解析
        SkeletonJson* json = SkeletonJson_create(atlas);
        skeletonData = SkeletonJson_readSkeletonDataFile(json, skeletonPath);
        if (!skeletonData) {
            printf("Failed to load skeleton data from JSON file: %s\n", json->error);
            SkeletonJson_dispose(json);
            file.close();
            Atlas_dispose(atlas);
            return 5;
        }
        SkeletonJson_dispose(json);
    }
    else {
        // 使用二进制解析
        SkeletonBinary* binary = SkeletonBinary_create(atlas);
        skeletonData = SkeletonBinary_readSkeletonDataFile(binary, skeletonPath);
        if (!skeletonData) {
            printf("Failed to load skeleton data from binary file: %s\n", binary->error);
            SkeletonBinary_dispose(binary);
            file.close();
            Atlas_dispose(atlas);
            return 6;
        }
        SkeletonBinary_dispose(binary);
    }

    file.close();

    SkeletonDrawable* drawable = new SkeletonDrawable(skeletonData);
    drawable->timeScale = 1.0f;
    drawable->setUsePremultipliedAlpha(true);

    Skeleton* skeleton = drawable->skeleton;

    // 1. 计算初步包围盒
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    bool hasAttachment = false;

    Skeleton_setToSetupPose(skeleton);

    // 检查动画是否存在
    spAnimation* animation = spSkeletonData_findAnimation(skeletonData, "Default");
    if (animation) {
        AnimationState_setAnimationByName(drawable->state, 0, "Default", false);
    }
    else {
        // 如果Default动画不存在，则使用Relax动画
        AnimationState_setAnimationByName(drawable->state, 0, "Relax", false);
    }

    AnimationState_apply(drawable->state, skeleton);

    drawable->state->tracks[0]->trackTime = 0;
    Skeleton_updateWorldTransform(skeleton);

    // 手动创建 clipper
    spSkeletonClipping* clipper = spSkeletonClipping_create();

    for (int i = 0; i < skeleton->slotsCount; ++i) {
        spSlot* slot = skeleton->drawOrder[i];
        spAttachment* attachment = slot->attachment;

        // 跳过不可见或无效的 Slot
        if (!attachment || slot->color.a == 0 || !slot->bone->active) {
            spSkeletonClipping_clipEnd(clipper, slot);
            continue;
        }

        // 处理附件类型
        if (attachment->type == SP_ATTACHMENT_REGION) {
            spRegionAttachment* region = (spRegionAttachment*)attachment;
            float vertices[8];
            spRegionAttachment_computeWorldVertices(region, slot->bone, vertices, 0, 2);

            for (int v = 0; v < 8; v += 2) {
                updateMinMax(vertices[v], vertices[v + 1], minX, minY, maxX, maxY);
            }
        }
        else if (attachment->type == SP_ATTACHMENT_MESH) {
            spMeshAttachment* mesh = (spMeshAttachment*)attachment;
            std::vector<float> vertices(mesh->super.worldVerticesLength);
            spVertexAttachment_computeWorldVertices(SUPER(mesh), slot, 0, mesh->super.worldVerticesLength, vertices.data(), 0, 2);

            for (int v = 0; v < vertices.size(); v += 2) {
                updateMinMax(vertices[v], vertices[v + 1], minX, minY, maxX, maxY);
            }
        }
        else if (attachment->type == SP_ATTACHMENT_CLIPPING) {
            spClippingAttachment* clip = (spClippingAttachment*)attachment;
            spSkeletonClipping_clipStart(clipper, slot, clip);
            continue;
        }

        // 处理裁剪后的顶点
        if (spSkeletonClipping_isClipping(clipper)) {
            float* clippedVertices = clipper->clippedVertices->items;
            int count = clipper->clippedVertices->size;
            for (int v = 0; v < count; v += 2) {
                updateMinMax(clippedVertices[v], clippedVertices[v + 1], minX, minY, maxX, maxY);
            }
        }

        spSkeletonClipping_clipEnd(clipper, slot);
    }

    // 销毁 clipper
    spSkeletonClipping_dispose(clipper);

    float bboxWidth = maxX - minX;
    float bboxHeight = maxY - minY;

    // 2. 画布大小根据包围盒调整
    int margin = 0; // 可选：给包围盒加点边距
    int texWidth = static_cast<int>(bboxWidth) + margin * 2;
    int texHeight = static_cast<int>(bboxHeight) + margin * 2;

    // 让包围盒中心对齐画布中心
    skeleton->x = texWidth / 2.0f - (minX + bboxWidth / 2.0f);
    skeleton->y = texHeight / 2.0f - (minY + bboxHeight / 2.0f);
    Skeleton_updateWorldTransform(skeleton);

    // 3. 渲染到纹理
    sf::RenderTexture renderTexture;
    renderTexture.create(texWidth, texHeight);
    renderTexture.clear(sf::Color::Transparent);
    renderTexture.draw(*drawable);
    renderTexture.display();

    // 4. 提取像素点，计算像素级凸包
    sf::Image image = renderTexture.getTexture().copyToImage();
    std::vector<sf::Vector2i> points;
    float totalMoment = 0.0f; // 顺便计算水平重心

    for (unsigned y = 0; y < image.getSize().y; ++y) {
        for (unsigned x = 0; x < image.getSize().x; ++x) {
            if (image.getPixel(x, y).a > 0) {
                points.emplace_back(x, y);
                totalMoment += x * 1.0f; // 使用列索引作为力矩
            }
        }
    }
    if (points.empty()) {
        printf("No non-transparent pixels found!\n");
        return 7;
    }

    // 5. 计算凸包包围盒
    auto hull = convexHull(points);
    float centroidX = totalMoment / points.size(); // 重心 = 总力矩 / 总质量

    // 计算凸包包围盒
    int hullMinX = texWidth, hullMinY = texHeight, hullMaxX = 0, hullMaxY = 0;
    for (const auto& p : hull) {
        if (p.x < hullMinX) hullMinX = p.x;
        if (p.y < hullMinY) hullMinY = p.y;
        if (p.x > hullMaxX) hullMaxX = p.x;
        if (p.y > hullMaxY) hullMaxY = p.y;
    }
    int hullWidth = hullMaxX - hullMinX;
    int hullHeight = hullMaxY - hullMinY;

    // 6. 基于重心调整包围框
    int targetCenterX = static_cast<int>(centroidX);
    int adjustedMinX, adjustedMaxX;

    // 计算原始包围框的中心
    int originalCenterX = (hullMinX + hullMaxX) / 2;

    // 确定哪条边需要移动
    int adjustedWidth = hullWidth;
    int offset = (targetCenterX - originalCenterX) * 2;

    if (centroidX > originalCenterX) {
        adjustedMinX = hullMinX;
        adjustedMaxX = hullMinX + adjustedWidth + offset;
    }
    else {
        adjustedMaxX = hullMaxX;
        adjustedMinX = hullMaxX - adjustedWidth + offset;
    }

    // 更新最终的包围框尺寸
    int finalHullWidth = adjustedMaxX - adjustedMinX;
    int finalHullHeight = hullHeight;

    // 7. 重新居中调整后的包围框到画布
    const int finalSize = 768;
    int finalTextureMargin = (finalSize - std::max(finalHullWidth, finalHullHeight)) / 2;

    // 创建新的纹理并绘制调整后的图像
    sf::RenderTexture finalTexture;
    finalTexture.create(finalSize, finalSize);
    finalTexture.clear(sf::Color::Transparent);

    // 计算调整后的图像位置
    int finalCenterX = finalSize / 2;
    int finalCenterY = finalSize / 2;
    int offsetX = finalCenterX - ((adjustedMinX + adjustedMaxX) / 2);
    int offsetY = finalCenterY - ((hullMinY + hullMaxY) / 2);

    sf::Sprite sprite(renderTexture.getTexture());
    sprite.setPosition(static_cast<float>(offsetX), static_cast<float>(offsetY));
    finalTexture.draw(sprite);
    finalTexture.display();

    // 输出原始和调整后的包围框尺寸
    // printf("Center: (%d, %d)\n", originalCenterX, targetCenterX);
    // printf("Original Bounding Box: (%d, %d)\n", hullWidth, hullHeight);
    // printf("Adjusted Bounding Box: (%d, %d)\n", finalHullWidth, finalHullHeight);

    // 创建预览窗口
    // sf::RenderWindow window(sf::VideoMode(finalSize, finalSize), "Preview");
    // window.setVerticalSyncEnabled(true);

    // 事件循环
    // while (window.isOpen()) {
    //     sf::Event event;
    //     while (window.pollEvent(event)) {
    //         if (event.type == sf::Event::Closed)
    //             window.close();
    //         // 按ESC键也可以关闭
    //         if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
    //             window.close();
    //     }
    // 
    //     // 绘制到窗口
    //     window.clear(sf::Color(35, 35, 35)); // 深灰色背景
    //     sf::Sprite finalSprite(finalTexture.getTexture());
    //     window.draw(finalSprite);
    //     window.display();
    // }

    // 保存为PNG
    if (!finalTexture.getTexture().copyToImage().saveToFile(outputPath)) {
        printf("Failed to save image\n");
        return 8;
    }
    else {
        printf("Tex: (%d, %d)\n", finalHullWidth, finalHullHeight);
    }

    // 清理资源
    delete drawable;
    SkeletonData_dispose(skeletonData);
    Atlas_dispose(atlas);

    return 0;
}
