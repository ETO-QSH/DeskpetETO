#define SPINE_SHORT_NAMES
#include <spine/spine-sfml.h>
#include <SFML/Graphics.hpp>
#include <algorithm>
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

int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: %s <skeleton.skel> <atlas.atlas> <output.png>\n", argv[0]);
        return 1;
    }

    // 加载资源
    const char* path = argv[3];
    Atlas* atlas = Atlas_createFromFile(argv[2], 0);
    SkeletonBinary* binary = SkeletonBinary_create(atlas);
    SkeletonData* skeletonData = SkeletonBinary_readSkeletonDataFile(binary, argv[1]);
    if (!skeletonData) {
        printf("Error loading skeleton data: %s\n", binary->error);
        Atlas_dispose(atlas);
        SkeletonBinary_dispose(binary);
        return 2;
    }

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

    AnimationState_setAnimationByName(drawable->state, 0, "Default", false);
    drawable->state->tracks[0]->trackTime = 16;
    Skeleton_setToSetupPose(skeleton);
    AnimationState_apply(drawable->state, skeleton);
    Skeleton_updateWorldTransform(skeleton);

    for (int i = 0; i < skeleton->slotsCount; ++i) {
        spSlot* slot = skeleton->drawOrder[i];
        spAttachment* attachment = slot->attachment;
        if (!attachment) continue;

        if (attachment->type == SP_ATTACHMENT_REGION) {
            spRegionAttachment* region = (spRegionAttachment*)attachment;
            hasAttachment = true;
            float vertices[8];
            spRegionAttachment_computeWorldVertices(region, slot->bone, vertices, 0, 2);
            for (int v = 0; v < 8; v += 2) {
                float x = vertices[v];
                float y = vertices[v + 1];
                if (x < minX) minX = x;
                if (y < minY) minY = y;
                if (x > maxX) maxX = x;
                if (y > maxY) maxY = y;
            }
        }
        if (attachment->type == SP_ATTACHMENT_MESH) {
            spMeshAttachment* mesh = (spMeshAttachment*)attachment;
            hasAttachment = true;
            int vertexCount = mesh->super.worldVerticesLength;
            std::vector<float> worldVertices(vertexCount);
            spVertexAttachment_computeWorldVertices(
                (spVertexAttachment*)mesh, slot, 0, vertexCount, worldVertices.data(), 0, 2
            );
            for (int v = 0; v < vertexCount; v += 2) {
                float x = worldVertices[v];
                float y = worldVertices[v + 1];
                if (x < minX) minX = x;
                if (y < minY) minY = y;
                if (x > maxX) maxX = x;
                if (y > maxY) maxY = y;
            }
        }
    }
    if (!hasAttachment) {
        printf("No RegionAttachment found!\n");
        return 3;
    }

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
    for (unsigned y = 0; y < image.getSize().y; ++y) {
        for (unsigned x = 0; x < image.getSize().x; ++x) {
            if (image.getPixel(x, y).a > 0) {
                points.emplace_back(x, y);
            }
        }
    }
    if (points.empty()) {
        printf("No non-transparent pixels found!\n");
        return 4;
    }
    auto hull = convexHull(points);

    // 5. 计算凸包包围盒
    int hullMinX = texWidth, hullMinY = texHeight, hullMaxX = 0, hullMaxY = 0;
    for (const auto& p : hull) {
        if (p.x < hullMinX) hullMinX = p.x;
        if (p.y < hullMinY) hullMinY = p.y;
        if (p.x > hullMaxX) hullMaxX = p.x;
        if (p.y > hullMaxY) hullMaxY = p.y;
    }
    int hullWidth = hullMaxX - hullMinX;
    int hullHeight = hullMaxY - hullMinY;

    // 6. 重新居中凸包到画布
    const int finalSize = 768;

    int hullCenterX = (hullMinX + hullMaxX) / 2;
    int hullCenterY = (hullMinY + hullMaxY) / 2;

    int offsetX = finalSize / 2 - hullCenterX;
    int offsetY = finalSize / 2 - hullCenterY;

    sf::RenderTexture finalTexture;
    finalTexture.create(finalSize, finalSize);
    finalTexture.clear(sf::Color::Transparent);

    // 重新渲染，平移到新画布中心
    sf::Sprite sprite(renderTexture.getTexture());
    sprite.setPosition(static_cast<float>(offsetX), static_cast<float>(offsetY));
    finalTexture.draw(sprite);
    finalTexture.display();

    // 保存为PNG
    if (!finalTexture.getTexture().copyToImage().saveToFile(path)) {
        printf("Failed to save image\n");
        return 5;
    }
    else {
        printf("Tex: (%d, %d)\n", hullWidth, hullHeight);
    }

    // 清理资源
    delete drawable;
    SkeletonData_dispose(skeletonData);
    Atlas_dispose(atlas);
    SkeletonBinary_dispose(binary);

    return 0;
}
