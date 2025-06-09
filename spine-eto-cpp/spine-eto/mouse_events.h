#pragma once

#include <SFML/Graphics.hpp>

enum class MouseButtonType {
    Left,
    Right,
    Other
};

struct DragState {
    bool dragging = false;
    sf::Vector2i startPos;
    sf::Vector2i lastPos;
    sf::Vector2i currentPos;
    sf::Vector2f velocity; // px/s
    sf::Clock dragClock;
};

class MouseEventManager {
public:
    MouseEventManager();

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    bool isDragging() const;
    sf::Vector2i getDragStart() const;
    sf::Vector2i getDragCurrent() const;
    sf::Vector2f getDragVelocity() const;

private:
    DragState dragState;
    sf::Clock velocityClock;
};
