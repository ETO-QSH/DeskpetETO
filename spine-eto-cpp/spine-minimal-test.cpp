#include <spine/spine-sfml.h>
#include <SFML/Graphics.hpp>
#include <memory>

using namespace spine;

int main() {
    system("chcp 65001");

    sf::RenderWindow window(sf::VideoMode(800, 600), "Spine Minimal Test");
    window.setFramerateLimit(60);

    SFMLTextureLoader textureLoader;
    auto atlas = std::make_unique<Atlas>("./models/lisa/build_char_358_lisa_lxh#1.atlas", &textureLoader);
    SkeletonBinary binary(atlas.get());
    auto skeletonData = std::shared_ptr<SkeletonData>(binary.readSkeletonDataFile("./models/lisa/build_char_358_lisa_lxh#1.skel"));

    if (!skeletonData) {
        printf("Error loading skeleton data: %s\n", binary.getError().buffer());
        return 1;
    }

    AnimationStateData stateData(skeletonData.get());
    SkeletonDrawable drawable(skeletonData.get(), &stateData);
    drawable.timeScale = 1.0f;
    drawable.setUsePremultipliedAlpha(false);

    drawable.skeleton->setPosition(400, 500);
    drawable.skeleton->updateWorldTransform();
    drawable.state->setAnimation(0, "Special", true);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        float delta = deltaClock.restart().asSeconds();
        drawable.update(delta);

        window.clear(sf::Color(0, 255, 0)); // 绿色背景
        window.draw(drawable);
        window.display();
    }
    return 0;
}
