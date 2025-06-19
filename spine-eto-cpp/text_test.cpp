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

    // 设置是否启用虚线下划线及其参数
    void setDashedUnderline(bool enabled, float underlineHeight = 2.f, float dashLen = 12.f, float gapLen = 6.f, float underlineOffset = 6.f, float drawOffset = 2.f) {
        dashedUnderline = enabled;
        underlineHeightPx = underlineHeight;
        dashLength = dashLen;
        gapLength = gapLen;
        underlineOffsetPx = underlineOffset;
        underlineDrawOffsetPx = drawOffset;
        updateLines();
    }

    void draw(sf::RenderWindow& window) {
        window.draw(box);
        for (size_t i = 0; i < sfLines.size(); ++i) {
            const auto& text = sfLines[i];
            window.draw(text);
            // 每行文字下方绘制虚线下划线
            if (dashedUnderline) {
                sf::FloatRect bounds = text.getGlobalBounds();
                float x = bounds.left;
                float y = bounds.top + bounds.height + underlineOffsetPx;
                float w = bounds.width;
                sf::Color lineColor = textColor;
                float h = underlineHeightPx;
                float dash = dashLength, gap = gapLength;
                float curX = x;
                while (curX < x + w) {
                    float seg = std::min(dash, x + w - curX);
                    sf::RectangleShape dashRect(sf::Vector2f(seg, h));
                    dashRect.setPosition(curX, y + underlineDrawOffsetPx);
                    dashRect.setFillColor(lineColor);
                    window.draw(dashRect);
                    curX += dash + gap;
                }
            }
        }
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
    bool dashedUnderline = false;
    float underlineHeightPx = 2.f;
    float dashLength = 12.f;
    float gapLength = 6.f;
    float underlineOffsetPx = 6.f;
    float underlineDrawOffsetPx = 2.f;

    void setupBox() {
        box.setPosition(50, 50);
        if (boxWidth > 0 && boxHeight > 0)
            box.setSize(sf::Vector2f(boxWidth, boxHeight));
    }

    // 自动换行文本分割（\t视为两个中文字符宽度）
    std::vector<sf::String> wrapText(const sf::String& text, float maxWidth) {
        std::vector<sf::String> lines;
        sf::String line;
        for (std::size_t i = 0; i < text.getSize(); ++i) {
            sf::Uint32 ch = text[i];
            if (ch == '\t') {
                // 计算两个中文字符宽度
                sf::Text testText(L"喵呜", font, characterSize);
                testText.setLetterSpacing(letterSpacing);
                float tabWidth = testText.getLocalBounds().width;
                sf::Text lineText(line, font, characterSize);
                lineText.setLetterSpacing(letterSpacing);
                float curWidth = lineText.getLocalBounds().width;
                // 如果加tab会超出，先换行
                if (curWidth + tabWidth > maxWidth && !line.isEmpty()) {
                    lines.push_back(line);
                    line.clear();
                }
                // 用两个全角空格代替tab
                line += L"　　";
            } else {
                sf::String testLine = line + ch;
                sf::Text testText(testLine, font, characterSize);
                testText.setLetterSpacing(letterSpacing);
                if (testText.getLocalBounds().width > maxWidth && !line.isEmpty()) {
                    lines.push_back(line);
                    line = ch;
                } else {
                    line = testLine;
                }
            }
        }
        if (!line.isEmpty()) lines.push_back(line);
        return lines;
    }

    void updateLines() {
        sfLines.clear();
        if (!font.getInfo().family.empty() && !inputText.isEmpty()) {
            float basePadding = 15.f;
            float maxWidth = boxWidth > 0 ? boxWidth - 2 * basePadding : 400;
            float maxHeight = boxHeight > 0 ? boxHeight - 2 * basePadding : 160;

            if (aspectRatio > 0) {
                float bestHeight = 0;
                float minDiff = 1e9f;
                std::vector<sf::String> bestLines;

                for (float w = 100; w <= 1200; w += 5) {
                    std::vector<sf::String> lines = wrapText(inputText, w);

                    // 判断解的有效性：如果行数>2，最后一行长度需在1/3~4/5之间
                    bool valid = true;
                    if (lines.size() > 2) {
                        float sum = 0.f;
                        for (size_t i = 0; i + 1 < lines.size(); ++i) {
                            sf::Text t(lines[i], font, characterSize);
                            t.setLetterSpacing(letterSpacing);
                            sum += t.getLocalBounds().width;
                        }
                        float avg = sum / (lines.size() - 1);
                        sf::Text lastT(lines.back(), font, characterSize);
                        lastT.setLetterSpacing(letterSpacing);
                        float lastW = lastT.getLocalBounds().width;
                        float minW = avg / 3.f;
                        float maxW = avg * 4.f / 5.f;
                        if (lastW < minW || lastW > maxW) {
                            valid = false;
                        }
                    }

                    if (!valid) continue;

                    float totalHeight = 0;
                    for (const auto& line : lines) {
                        sf::Text text(line, font, characterSize);
                        text.setLetterSpacing(letterSpacing);
                        totalHeight += characterSize + lineSpacing + (dashedUnderline ? (underlineOffsetPx + underlineHeightPx + underlineDrawOffsetPx) : 0.f);
                    }
                    float h = totalHeight;
                    float ratio = w / h;
                    float diff = std::abs(ratio - aspectRatio);
                    if (diff < minDiff) {
                        minDiff = diff;
                        bestHeight = h;
                        bestLines = lines;
                    }
                }

                // 计算最长行宽度
                float maxLineWidth = 0.f;
                for (const auto& line : bestLines) {
                    sf::Text text(line, font, characterSize);
                    text.setLetterSpacing(letterSpacing);
                    float lineWidth = text.getLocalBounds().width;
                    if (lineWidth > maxLineWidth) maxLineWidth = lineWidth;
                }

                // 修正padding，使内容在新box尺寸下左右边距一致，全部左对齐
                float contentWidth = maxLineWidth;
                float contentHeight = bestHeight;
                float newBoxWidth = contentWidth + 2 * basePadding;
                float newBoxHeight = contentHeight + 2 * basePadding;
                boxWidth = newBoxWidth;
                boxHeight = newBoxHeight;
                box.setSize(sf::Vector2f(boxWidth, boxHeight));

                maxHeight = contentHeight;
                float padding = (boxWidth - maxLineWidth) / 2.f;
                float x = box.getPosition().x + padding;
                float y = box.getPosition().y + basePadding / 2.f;

                // 居中起始y
                float totalTextHeight = bestLines.size() * (characterSize + lineSpacing + (dashedUnderline ? (underlineOffsetPx + underlineHeightPx + underlineDrawOffsetPx) : 0.f));
                if (!bestLines.empty()) totalTextHeight -= lineSpacing;
                float yStart = y + (maxHeight - totalTextHeight) / 1.5f;

                sfLines.clear();
                float curY = yStart;
                for (const auto& line : bestLines) {
                    sf::Text text(line, font, characterSize);
                    text.setFillColor(textColor);
                    text.setLetterSpacing(letterSpacing);
                    // 左对齐
                    text.setPosition(x, curY);
                    sfLines.push_back(text);
                    curY += characterSize + lineSpacing + (dashedUnderline ? (underlineOffsetPx + underlineHeightPx + underlineDrawOffsetPx) : 0.f);
                }
                return;
            }

            std::vector<sf::String> lines = wrapText(inputText, maxWidth);

            // 计算最长行宽度
            float maxLineWidth = 0.f;
            for (const auto& line : lines) {
                sf::Text text(line, font, characterSize);
                text.setLetterSpacing(letterSpacing);
                float lineWidth = text.getLocalBounds().width;
                if (lineWidth > maxLineWidth) maxLineWidth = lineWidth;
            }
            float padding = (boxWidth - maxLineWidth) / 2.f;
            float x = box.getPosition().x + padding;
            float y = box.getPosition().y + basePadding / 2.f;

            float totalTextHeight = lines.size() * (characterSize + lineSpacing + (dashedUnderline ? (underlineOffsetPx + underlineHeightPx + underlineDrawOffsetPx) : 0.f));
            if (!lines.empty()) totalTextHeight -= lineSpacing;
            float yStart = y + (maxHeight - totalTextHeight) / 1.5f;

            float curY = yStart;
            for (const auto& line : lines) {
                sf::Text text(line, font, characterSize);
                text.setFillColor(textColor);
                text.setLetterSpacing(letterSpacing);
                // 左对齐
                text.setPosition(x, curY);
                sfLines.push_back(text);
                curY += characterSize + lineSpacing + (dashedUnderline ? (underlineOffsetPx + underlineHeightPx + underlineDrawOffsetPx) : 0.f);
            }
        }
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "TextBox Demo", sf::Style::Default);
    window.setVerticalSyncEnabled(true);

    // 你可以选择如下两种方式初始化
    // TextBox textBox(420, 270); // 直接指定宽高
    TextBox textBox(4.0f); // 指定宽高比

    textBox.setFont("./source/font/Lolita.ttf");
    textBox.setCharacterSize(24);
    textBox.setTextColor(sf::Color::Red);
    textBox.setPosition(50, 50);
    textBox.setOutline(sf::Color::Blue, 2);
    textBox.setBackgroundColor(sf::Color(223, 223, 223));
    textBox.setLineSpacing(12);      // 设置行间距
    textBox.setLetterSpacing(2.0f);  // 设置字间距
    textBox.setDashedUnderline(true, 2.f, 12.f, 6.f, 6.f, 2.f); // 启用虚线下划线
    textBox.setText(L"\t这是一个自动换行的文本框示例。你可以指定文本框的尺寸，文本会自动换行显示，支持中英文混排。This is a test for auto line wrapping in a textbox using SFML.");

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
