#define SPINE_SHORT_NAMES
#include <spine/spine-sfml.h>
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <vector>

using namespace spine;

// ͹���㷨��Andrew's Monotone Chain��
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

    // ������Դ
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

    // 1. ���������Χ��
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    bool hasAttachment = false;

    Skeleton_setToSetupPose(skeleton);
    AnimationState_setAnimationByName(drawable->state, 0, "Default", false);
    AnimationState_apply(drawable->state, skeleton);

    drawable->state->tracks[0]->trackTime = 0;
    Skeleton_updateWorldTransform(skeleton);

    // �ֶ����� clipper
    spSkeletonClipping* clipper = spSkeletonClipping_create();

    for (int i = 0; i < skeleton->slotsCount; ++i) {
        spSlot* slot = skeleton->drawOrder[i];
        spAttachment* attachment = slot->attachment;

        // �������ɼ�����Ч�� Slot
        if (!attachment || slot->color.a == 0 || !slot->bone->active) {
            spSkeletonClipping_clipEnd(clipper, slot);
            continue;
        }

        // ����������
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

        // ����ü���Ķ���
        if (spSkeletonClipping_isClipping(clipper)) {
            float* clippedVertices = clipper->clippedVertices->items;
            int count = clipper->clippedVertices->size;
            for (int v = 0; v < count; v += 2) {
                updateMinMax(clippedVertices[v], clippedVertices[v + 1], minX, minY, maxX, maxY);
            }
        }

        spSkeletonClipping_clipEnd(clipper, slot);
    }

    // ���� clipper
    spSkeletonClipping_dispose(clipper);

    float bboxWidth = maxX - minX;
    float bboxHeight = maxY - minY;

    // 2. ������С���ݰ�Χ�е���
    int margin = 0; // ��ѡ������Χ�мӵ�߾�
    int texWidth = static_cast<int>(bboxWidth) + margin * 2;
    int texHeight = static_cast<int>(bboxHeight) + margin * 2;

    // �ð�Χ�����Ķ��뻭������
    skeleton->x = texWidth / 2.0f - (minX + bboxWidth / 2.0f);
    skeleton->y = texHeight / 2.0f - (minY + bboxHeight / 2.0f);
    Skeleton_updateWorldTransform(skeleton);

    // 3. ��Ⱦ������
    sf::RenderTexture renderTexture;
    renderTexture.create(texWidth, texHeight);
    renderTexture.clear(sf::Color::Transparent);
    renderTexture.draw(*drawable);
    renderTexture.display();

    // 4. ��ȡ���ص㣬�������ؼ�͹��
    sf::Image image = renderTexture.getTexture().copyToImage();
    std::vector<sf::Vector2i> points;
    float totalMoment = 0.0f; // ˳�����ˮƽ����

    for (unsigned y = 0; y < image.getSize().y; ++y) {
        for (unsigned x = 0; x < image.getSize().x; ++x) {
            if (image.getPixel(x, y).a > 0) {
                points.emplace_back(x, y);
                totalMoment += x * 1.0f; // ʹ����������Ϊ����
            }
        }
    }
    if (points.empty()) {
        printf("No non-transparent pixels found!\n");
        return 4;
    }

    // 5. ����͹����Χ��
    auto hull = convexHull(points);
    float centroidX = totalMoment / points.size(); // ���� = ������ / ������

    // ����͹����Χ��
    int hullMinX = texWidth, hullMinY = texHeight, hullMaxX = 0, hullMaxY = 0;
    for (const auto& p : hull) {
        if (p.x < hullMinX) hullMinX = p.x;
        if (p.y < hullMinY) hullMinY = p.y;
        if (p.x > hullMaxX) hullMaxX = p.x;
        if (p.y > hullMaxY) hullMaxY = p.y;
    }
    int hullWidth = hullMaxX - hullMinX;
    int hullHeight = hullMaxY - hullMinY;

    // 6. �������ĵ�����Χ��
    int targetCenterX = static_cast<int>(centroidX);
    int adjustedMinX, adjustedMaxX;

    // ����ԭʼ��Χ�������
    int originalCenterX = (hullMinX + hullMaxX) / 2;

    // ȷ����������Ҫ�ƶ�
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

    // �������յİ�Χ��ߴ�
    int finalHullWidth = adjustedMaxX - adjustedMinX;
    int finalHullHeight = hullHeight;

    // 7. ���¾��е�����İ�Χ�򵽻���
    const int finalSize = 768;
    int finalTextureMargin = (finalSize - std::max(finalHullWidth, finalHullHeight)) / 2;

    // �����µ��������Ƶ������ͼ��
    sf::RenderTexture finalTexture;
    finalTexture.create(finalSize, finalSize);
    finalTexture.clear(sf::Color::Transparent);

    // ����������ͼ��λ��
    int finalCenterX = finalSize / 2;
    int finalCenterY = finalSize / 2;
    int offsetX = finalCenterX - ((adjustedMinX + adjustedMaxX) / 2);
    int offsetY = finalCenterY - ((hullMinY + hullMaxY) / 2);

    sf::Sprite sprite(renderTexture.getTexture());
    sprite.setPosition(static_cast<float>(offsetX), static_cast<float>(offsetY));
    finalTexture.draw(sprite);
    finalTexture.display();

    // ���ԭʼ�͵�����İ�Χ��ߴ�
    // printf("Center: (%d, %d)\n", originalCenterX, targetCenterX);
    // printf("Original Bounding Box: (%d, %d)\n", hullWidth, hullHeight);
    // printf("Adjusted Bounding Box: (%d, %d)\n", finalHullWidth, finalHullHeight);

    // ����Ԥ������
    // sf::RenderWindow window(sf::VideoMode(finalSize, finalSize), "Preview");
    // window.setVerticalSyncEnabled(true);

    // �¼�ѭ��
    // while (window.isOpen()) {
    //     sf::Event event;
    //     while (window.pollEvent(event)) {
    //         if (event.type == sf::Event::Closed)
    //             window.close();
    //         // ��ESC��Ҳ���Թر�
    //         if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
    //             window.close();
    //     }
    // 
    //     // ���Ƶ�����
    //     window.clear(sf::Color(35, 35, 35)); // ���ɫ����
    //     sf::Sprite finalSprite(finalTexture.getTexture());
    //     window.draw(finalSprite);
    //     window.display();
    // }

    // ����ΪPNG
    if (!finalTexture.getTexture().copyToImage().saveToFile(path)) {
        printf("Failed to save image\n");
        return 5;
    }
    else {
        printf("Tex: (%d, %d)\n", finalHullWidth, finalHullHeight);
    }

    // ������Դ
    delete drawable;
    SkeletonData_dispose(skeletonData);
    Atlas_dispose(atlas);
    SkeletonBinary_dispose(binary);

    return 0;
}
