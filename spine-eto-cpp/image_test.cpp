#include <SFML/Graphics.hpp>

#include <algorithm>
#include <iostream>
#include <vector>

struct BlackPixel {
    unsigned x, y;
};

struct Rect {
    sf::IntRect outer;
};

std::vector<Rect> findRects(const sf::Image& image) {
    std::vector<BlackPixel> blackPixels;
    sf::Vector2u size = image.getSize();

    // 逐行查找，每行最多两个黑色像素
    for (unsigned y = 0; y < size.y; ++y) {
        int count = 0;
        for (unsigned x = 0; x < size.x; ++x) {
            sf::Color color = image.getPixel(x, y);
            if (color.r == 0 && color.g == 0 && color.b == 0 && color.a == 255) {
                blackPixels.push_back({x, y});
                count++;
                if (count == 2) break;
            }
        }
    }

    // 每四个黑色像素构成一个矩形
    std::vector<Rect> rects;
    for (size_t i = 0; i + 3 < blackPixels.size(); i += 4) {
        // 左上、右上、左下、右下
        BlackPixel lu = blackPixels[i];
        BlackPixel ru = blackPixels[i+1];
        BlackPixel ld = blackPixels[i+2];
        BlackPixel rd = blackPixels[i+3];

        // 内边界
        int left = std::min(lu.x, ld.x);
        int right = std::max(ru.x, rd.x);
        int top = std::min(lu.y, ru.y);
        int bottom = std::max(ld.y, rd.y);

        // 外边界（左-90，上-90，右+90，下+60）
        int outerLeft = left - 90;
        int outerTop = top - 90;
        int outerWidth = (right - left) + 1 + 90 + 90;
        int outerHeight = (bottom - top) + 1 + 60 + 90;

        rects.push_back({sf::IntRect(outerLeft, outerTop, outerWidth, outerHeight)});
    }
    return rects;
}

// 获取指定索引的矩形区域图片（无扩展，仅裁剪）
sf::Image getRectImageByIndex(const sf::Image& image, int index, const std::vector<Rect>& rects) {
    sf::Image subImage;
    sf::IntRect rect = rects[index].outer;
    subImage.create(rect.width, rect.height, sf::Color::Transparent);

    // 四个方向偏移
    const int dirs[4][2] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
    for (int y = 0; y < rect.height; ++y) {
        for (int x = 0; x < rect.width; ++x) {
            sf::Color c = image.getPixel(rect.left + x, rect.top + y);
            if (c.r == 0 && c.g == 0 && c.b == 0 && c.a == 255) {
                sf::Color lastColor, curColor;
                bool found = false;
                for (int i = 0; i < 4; ++i) {
                    curColor = image.getPixel(rect.left + x + dirs[i][0], rect.top + y + dirs[i][1]);
                    if (i > 0 && curColor == lastColor) {
                        subImage.setPixel(x, y, curColor);
                        found = true;
                        break;
                    }
                    lastColor = curColor;
                }
            } else {
                subImage.setPixel(x, y, c);
            }
        }
    }
    return subImage;
}

// 对getRectImageByIndex的结果进行中心列/行模板拓展（复制完整行和列，先移动再填充）
sf::Image expandRectImage(const sf::Image& rawImage, int targetWidth, int targetHeight) {
    int srcW = rawImage.getSize().x;
    int srcH = rawImage.getSize().y;

    int leftMargin = 90;
    int rightMargin = 90;
    int topMargin = 90;
    int bottomMargin = 60;

    int minW = srcW - leftMargin - rightMargin;
    int minH = srcH - topMargin - bottomMargin;

    int outW = (targetWidth > minW) ? targetWidth : minW;
    int outH = (targetHeight > minH) ? targetHeight : minH;

    int totalW = leftMargin + rightMargin + outW;
    int totalH = topMargin + bottomMargin + outH;

    int expandW = outW - minW;
    int midX = leftMargin + minW / 2 - 1;

    int expandH = outH - minH;
    int midY = topMargin + minH / 2 - 1;

    sf::Image subImage;
    subImage.create(totalW, totalH, sf::Color::Transparent);

    // 1. 先复制原始内容到目标
    for (int y = 0; y < srcH; ++y)
        for (int x = 0; x < srcW; ++x)
            subImage.setPixel(x, y, rawImage.getPixel(x, y));

    // 2. 横向扩展（严格先移动再填充）
    if (outW > minW) {
        // 先将中心列右边的内容整体右移expandW，避免覆盖
        for (int y = 0; y < srcH; ++y) {
            for (int x = srcW - 1; x > midX - 1; --x) {
                sf::Color c = subImage.getPixel(x, y);
                subImage.setPixel(x + expandW, y, c);
            }
        }
        // 再填充扩展区
        for (int y = 0; y < srcH; ++y) {
            sf::Color midColor = subImage.getPixel(midX, y);
            for (int dx = 0; dx < expandW; ++dx) {
                subImage.setPixel(midX + dx, y, midColor);
            }
        }
    }

    // 3. 纵向扩展（严格先移动再填充）
    if (outH > minH) {
        // 先将中心行下方的内容整体下移expandH，避免覆盖
        for (int y = srcH - 1; y > midY - 1; --y) {
            for (int x = 0; x < srcW + expandW; ++x) {
                sf::Color c = subImage.getPixel(x, y);
                subImage.setPixel(x, y + expandH, c);
            }
        }
        // 再填充扩展区
        for (int x = 0; x < totalW; ++x) {
            sf::Color midColor = subImage.getPixel(x, midY);
            for (int dy = 0; dy < expandH; ++dy) {
                subImage.setPixel(x, midY + dy, midColor);
            }
        }
    }

    return subImage;
}

void drawRectByIndex(sf::RenderWindow& window, const sf::Image& image, int index, const std::vector<Rect>& rects) {
    sf::Texture texture;
    texture.loadFromImage(image, rects[index].outer);
    sf::Sprite sprite(texture);
    sprite.setPosition(0, 0);
    window.clear(sf::Color::White);
    window.draw(sprite);
    window.display();
}

// 打包为一个函数，返回最终texture，支持指定字体、字号、颜色
sf::Texture getBubbleTexture(
    int index,
    int targetWidth,
    int targetHeight,
    const std::string& fontPath = "",
    unsigned int fontSize = 24,
    sf::Color color = sf::Color::White
) {
    sf::Image image;
    image.loadFromFile("source/image/bubble_change.png");
    auto rects = findRects(image);

    sf::Image rawImage = getRectImageByIndex(image, index, rects);
    sf::Image subImage = expandRectImage(rawImage, targetWidth, targetHeight);

    // 如果需要字体和颜色处理（比如在气泡上绘制文字），这里可以绘制
    // 这里只设置整体颜色（如 tint 效果），如需绘制文字请自行添加
    if (color != sf::Color::White) {
        for (unsigned y = 0; y < subImage.getSize().y; ++y) {
            for (unsigned x = 0; x < subImage.getSize().x; ++x) {
                sf::Color c = subImage.getPixel(x, y);
                // 仅对非透明像素着色
                if (c.a > 0) {
                    c.r = color.r;
                    c.g = color.g;
                    c.b = color.b;
                    subImage.setPixel(x, y, c);
                }
            }
        }
    }

    sf::Texture texture;
    texture.loadFromImage(subImage);
    return texture;
}

int main() {
    int index = 1;
    int targetWidth = 600;
    int targetHeight = 300;
    std::string fontPath = "C:/Windows/Fonts/simhei.ttf"; // 可自定义
    unsigned int fontSize = 32;
    sf::Color color = sf::Color(200, 255, 200);

    sf::Texture texture = getBubbleTexture(index, targetWidth, targetHeight, fontPath, fontSize, color);
    sf::Sprite sprite(texture);

    sf::RenderWindow window(sf::VideoMode(texture.getSize().x, texture.getSize().y), "Rect Crop Demo");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        window.clear(sf::Color::White);
        window.draw(sprite);
        window.display();
    }

    return 0;
}
