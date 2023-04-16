#include <iostream>
#include "image.h"

Image process_image(const Image& image);

int main() {
    auto image = load_image("input/1.jpg");
    std::cout << "width: " << image.width() << " ,height: " << image.height() << " ,nr_channels: " << image.nr_channels() << '\n';
    save_image(process_image(image), "output/1.png");

    return 0;
}

Image process_image(const Image& image) {
    auto width = image.width();
    auto height = image.height();
    auto nr_channels = image.nr_channels();

    Image greyscale_image(width, height, 1);
    greyscale_image.loop_2d([&] (Image::size_t i, Image::size_t j) {
        greyscale_image(i, j) = (image(i, j, 0) + image(i, j, 1) + image(i, j, 2)) / 3;
    });

    Kernel g_x(3);
    g_x = {1,  0, -1,
           2,  0, -2,
           1,  0, -1};
    


    Image output(greyscale_image);

    return output;
}