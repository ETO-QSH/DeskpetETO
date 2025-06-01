#include <iostream>
#include <string.h>
#define SPINE_SHORT_NAMES
#include <spine/spine-sfml.h>
#include <SFML/Graphics.hpp>

using namespace std;
using namespace spine;

SkeletonData* readSkeletonBinaryData(const char* filename, Atlas* atlas, float scale) {
    SkeletonBinary* binary = SkeletonBinary_create(atlas);
    binary->scale = scale;
    SkeletonData* skeletonData = SkeletonBinary_readSkeletonDataFile(binary, filename);
    if (!skeletonData) {
        printf("%s\n", binary->error);
        exit(0);
    }
    SkeletonBinary_dispose(binary);
    return skeletonData;
}

int main() {
    Atlas* atlas = Atlas_createFromFile("data/build_char_4027_heyak_ambienceSynesthesia_4.atlas", 0);
    SkeletonData* skeletonData = readSkeletonBinaryData("data/build_char_4027_heyak_ambienceSynesthesia_4.skel", atlas, 0.6f);
    SkeletonDrawable* drawable = new SkeletonDrawable(skeletonData);
    
    drawable->timeScale = 1;
    drawable->setUsePremultipliedAlpha(true);
    
    Skeleton* skeleton = drawable->skeleton;
    
    skeleton->x = 320;
    skeleton->y = 590;

    AnimationState_setAnimationByName(drawable->state, 0, "Default", true);

    sf::RenderWindow window(sf::VideoMode(640, 640), "Spine SFML");
    window.setFramerateLimit(60);

    sf::Event event;
    sf::Clock deltaClock;

    while (window.isOpen()) {
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        float delta = deltaClock.getElapsedTime().asSeconds();
        deltaClock.restart();

        drawable->update(delta);

        window.clear();
        window.draw(*drawable);
        window.display();
    }

    SkeletonData_dispose(skeletonData);
    Atlas_dispose(atlas);

    return 0;
}