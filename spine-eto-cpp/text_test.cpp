#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>

class TextBox {
public:
    TextBox(float width, float height)
        : boxWidth(width), boxHeight(height), characterSize(24), textColor(sf::Color::Black), aspectRatio(0) {
        setupBox();
    }

    TextBox(float aspectRatio)
        : boxWidth(0), boxHeight(0), characterSize(24), textColor(sf::Color::Black), aspectRatio(aspectRatio) {
        setupBox();
    }

    void setFont(const std::string& fontPath) {
        font.loadFromFile(fontPath);
    }

    void setCharacterSize(unsigned size) {
        characterSize = size;
    }

    void setTextColor(const sf::Color& color) {
        textColor = color;
    }

    void setText(const sf::String& text) {
        inputText = text;
        updateLines();
    }

    void setPosition(float x, float y) {
        box.setPosition(x, y);
        updateLines();
    }

    void setBoxSize(float width, float height) {
        boxWidth = width;
        boxHeight = height;
        aspectRatio = 0;
        box.setSize(sf::Vector2f(boxWidth, boxHeight));
        updateLines();
    }

    void setAspectRatio(float ratio) {
        aspectRatio = ratio;
        boxWidth = 0;
        boxHeight = 0;
        updateLines();
    }

    void setOutline(const sf::Color& color, float thickness) {
        box.setOutlineColor(color);
        box.setOutlineThickness(thickness);
    }

    void setBackgroundColor(const sf::Color& color) {
        box.setFillColor(color);
    }

    void setLineSpacing(float spacing) {
        lineSpacing = spacing;
        updateLines();
    }

    void setLetterSpacing(float spacing) {
        letterSpacing = spacing;
        updateLines();
    }

    void draw(sf::RenderWindow& window) {
        window.draw(box);
        for (const auto& text : sfLines)
            window.draw(text);
    }

private:
    float boxWidth, boxHeight;
    unsigned characterSize;
    sf::Color textColor;
    sf::Font font;
    sf::String inputText;
    sf::RectangleShape box;
    std::vector<sf::Text> sfLines;
    float aspectRatio;
    float lineSpacing = 6.f;
    float letterSpacing = 0.f;

    void setupBox() {
        box.setPosition(50, 50);
        if (boxWidth > 0 && boxHeight > 0)
            box.setSize(sf::Vector2f(boxWidth, boxHeight));
    }

    // 自动换行文本分割（基于 sf::String，支持中英文混排，支持字间距）
    std::vector<sf::String> wrapText(const sf::String& text, float maxWidth) {
        std::vector<sf::String> lines;
        sf::String line;
        for (std::size_t i = 0; i < text.getSize(); ++i) {
            sf::String testLine = line + text[i];
            sf::Text testText(testLine, font, characterSize);
            testText.setLetterSpacing(letterSpacing); // 宽度测量时也要加字间距
            if (testText.getLocalBounds().width > maxWidth && !line.isEmpty()) {
                lines.push_back(line);
                line = text[i];
            } else {
                line = testLine;
            }
        }
        if (!line.isEmpty()) lines.push_back(line);
        return lines;
    }

    void updateLines() {
        sfLines.clear();
        if (!font.getInfo().family.empty() && !inputText.isEmpty()) {
            float x = box.getPosition().x + 10;
            float y = box.getPosition().y + 10;
            float maxWidth = boxWidth > 0 ? boxWidth - 20 : 400;
            float maxHeight = boxHeight > 0 ? boxHeight - 20 : 160;

            // 如果设置了宽高比，自动计算合适的宽高
            if (aspectRatio > 0) {
                float bestWidth = 400;
                float bestHeight = 0;
                float minDiff = 1e9f;
                std::vector<sf::String> bestLines;

                // 每次步进5像素
                for (float w = 100; w <= 1200; w += 5) {
                    std::vector<sf::String> lines = wrapText(inputText, w);
                    float totalHeight = 0;
                    for (const auto& line : lines) {
                        sf::Text text(line, font, characterSize);
                        text.setLetterSpacing(letterSpacing);
                        totalHeight += characterSize + lineSpacing;
                    }
                    float h = totalHeight;
                    float ratio = w / h;
                    float diff = std::abs(ratio - aspectRatio);
                    if (diff < minDiff) {
                        minDiff = diff;
                        bestWidth = w;
                        bestHeight = h;
                        bestLines = lines;
                    }
                }

                boxWidth = bestWidth + 20;
                boxHeight = bestHeight + 20;
                box.setSize(sf::Vector2f(boxWidth, boxHeight));

                maxHeight = boxHeight - 20;
                x = box.getPosition().x + 10;
                y = box.getPosition().y + 10;

                // 用最优行
                sfLines.clear();
                float curY = y;
                for (const auto& line : bestLines) {
                    sf::Text text(line, font, characterSize);
                    text.setFillColor(textColor);
                    text.setPosition(x, curY);
                    text.setLetterSpacing(letterSpacing);
                    sfLines.push_back(text);
                    curY += characterSize + lineSpacing;
                    if (curY > y + maxHeight - characterSize) break;
                }
                return;
            }

            std::vector<sf::String> lines = wrapText(inputText, maxWidth);
            float curY = y;
            for (const auto& line : lines) {
                sf::Text text(line, font, characterSize);
                text.setFillColor(textColor);
                text.setPosition(x, curY);
                text.setLetterSpacing(letterSpacing);
                sfLines.push_back(text);
                curY += characterSize + lineSpacing;
                if (curY > y + maxHeight - characterSize) break;
            }
        }
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "TextBox Demo", sf::Style::Default);
    window.setVerticalSyncEnabled(true);

    // 你可以选择如下两种方式初始化
    TextBox textBox(420, 270); // 直接指定宽高
    // TextBox textBox(4.0f); // 指定宽高比

    textBox.setFont("./source/font/Lolita.ttf");
    textBox.setCharacterSize(24);
    textBox.setTextColor(sf::Color::Red);
    textBox.setPosition(50, 50);
    textBox.setOutline(sf::Color::Blue, 2);
    textBox.setBackgroundColor(sf::Color(223, 223, 223));
    textBox.setLineSpacing(12);      // 设置行间距
    textBox.setLetterSpacing(2.0f);  // 设置字间距
    textBox.setText(L"这是一个自动换行的文本框示例。你可以指定文本框的尺寸，文本会自动换行显示，支持中英文混排。This is a test for auto line wrapping in a textbox using SFML.");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        window.clear(sf::Color::White);
        textBox.draw(window);
        window.display();
    }
    return 0;
}
