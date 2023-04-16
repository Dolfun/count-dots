#include <iostream>
#include "image.h"

int main() {
    auto image = load_image("input/1.jpg");
    std::cout << "width: " << image.width() << " ,height: " << image.height() << " ,nr_channels: " << image.nr_channels() << '\n';
    save_image(image, "output/1.png");

    return 0;
}