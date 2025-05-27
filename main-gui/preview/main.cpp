#define SPINE_SHORT_NAMES
#include <spine/spine-sfml.h>
#include <SFML/Graphics.hpp>

using namespace spine;

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
        return 1;
    }

    // 创建可绘制对象
    SkeletonDrawable* drawable = new SkeletonDrawable(skeletonData);
    drawable->timeScale = 1.0f;
    drawable->setUsePremultipliedAlpha(true);

    // 初始化骨骼
    Skeleton* skeleton = drawable->skeleton;
    int size = 512;

    // 设置默认动画并应用一帧
    AnimationState_setAnimationByName(drawable->state, 0, "Default", false);
    drawable->update(0); // 应用第0帧

    // 计算包围盒
    SkeletonBounds* bounds = SkeletonBounds_create();
    SkeletonBounds_update(bounds, skeleton, 1);

    // 获取四至点
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    bool hasAttachment = false;

    for (int i = 0; i < skeleton->slotsCount; ++i) {
        spSlot* slot = skeleton->drawOrder[i];
        if (!slot->attachment) continue;

        if (slot->attachment->type == SP_ATTACHMENT_REGION) {
            hasAttachment = true;
            spRegionAttachment* region = (spRegionAttachment*)slot->attachment;
            float worldVertices[8];
            spRegionAttachment_computeWorldVertices(region, slot->bone, worldVertices, 0, 2);
            for (int v = 0; v < 8; v += 2) {
                float x = worldVertices[v];
                float y = worldVertices[v + 1];
                if (x < minX) minX = x;
                if (y < minY) minY = y;
                if (x > maxX) maxX = x;
                if (y > maxY) maxY = y;
            }
        }
        else if (slot->attachment->type == SP_ATTACHMENT_MESH) {
            hasAttachment = true;
            spMeshAttachment* mesh = (spMeshAttachment*)slot->attachment;
            int vertexCount = mesh->super.worldVerticesLength;
            std::vector<float> worldVertices(vertexCount);
            spVertexAttachment_computeWorldVertices(
                (spVertexAttachment*)mesh,
                slot,
                0, vertexCount,
                worldVertices.data(),
                0, 2
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
        return 2;
    }
    printf("RegionAttachment Bounding Box: Min(%.2f, %.2f), Max(%.2f, %.2f)\n", minX, minY, maxX, maxY);

    // 包围盒宽高
    float bboxWidth = maxX - minX;
    float bboxHeight = maxY - minY;

    // 让包围盒中心对齐画布中心
    skeleton->x = size / 2.0f - (minX + bboxWidth / 2.0f);
    skeleton->y = size / 2.0f - (minY + bboxHeight / 2.0f);

    Skeleton_updateWorldTransform(skeleton);

    // 渲染到纹理
    sf::RenderTexture renderTexture;
    renderTexture.create(size, size); // 画布大小可根据包围盒调整
    renderTexture.clear(sf::Color::Transparent);
    renderTexture.draw(*drawable);
    renderTexture.display();

    // 保存为PNG
    if (!renderTexture.getTexture().copyToImage().saveToFile(path)) {
        printf("Failed to save image\n");
    }
    else {
        printf("Successfully exported to: %s\n", path);
    }

    // 清理资源
    delete drawable;
    SkeletonData_dispose(skeletonData);
    Atlas_dispose(atlas);
    SkeletonBinary_dispose(binary);

    return 0;
}
