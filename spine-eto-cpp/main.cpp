#include <spine/spine-sfml.h>
#include <SFML/Graphics.hpp>

#include "spine_win_utils.h"
#include "mouse_events.h"

using namespace spine;

int main() {
    system("chcp 65001");

    initWindowAndShader();
    initSpineModel();

    MouseEventManager mouseEventManager;

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            mouseEventManager.handleEvent(event, window);
        }

        float delta = deltaClock.restart().asSeconds();
        drawable->update(delta);

        renderTexture.clear(sf::Color::Transparent);
        renderTexture.draw(*drawable);
        renderTexture.display();

        setClickThrough(hwnd, renderTexture.getTexture().copyToImage());

        window.clear(sf::Color::Transparent);
        window.draw(*drawable);
        window.display();
    }

    return 0;
}
