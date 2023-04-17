#include <iostream>
#include "image.h"
#include "debug.h"

Image process_image(const Image& image, Profiler& profiler);

int main() {
    Profiler profiler;
    profiler.start();

    Image image;
    profiler.profile("loading image", [&] {
        //image = load_image("input/1.jpg");
        image = load_image("input/Untitled.png");
    });
    
    std::cout << "width: " << image.width() << " height: " << image.height() << " nr_channels: " << image.nr_channels() << '\n';
    std::cout << "nr_pixels: " << image.width() * image.height() << '\n';

    Image output= process_image(image, profiler);

    profiler.profile("saving image", [&] {
        save_image(output, "output/1.png");
    });

    profiler.stop();
    profiler.print_results();

    return 0;
}

Image process_image(const Image& image, Profiler& profiler) {
    auto width = image.width();
    auto height = image.height();
    auto nr_channels = image.nr_channels();

    Image greyscale_image{width, height, 1};
    profiler.profile("greyscaling image", [&] {
        greyscale_image.loop_2d([&] (Image::size_t i, Image::size_t j) {
            greyscale_image(i, j) = (image(i, j, 0) + image(i, j, 1) + image(i, j, 2)) / 3;
        });
    });
    
    /*Kernel g_x(3), g_y(3);
    g_x = {1,  0, -1,
           2,  0, -2,
           1,  0, -1};
    g_y = {-1, -2, -1,
            0,  0,  0,
            1,  2,  1};*/
    
    Kernel g_x(5), g_y(5);
    g_x = {
        -1,  -2, 0,  2, 1,
        -4,  -8, 0,  8, 4,
        -6, -12, 0, 12, 6,
        -4,  -8, 0,  8, 4,
        -1,  -2, 0,  2, 1,
    };
    g_y = {
         1,  4,   6,  4,  1,
         2,  8,  12,  8,  2,
         0,  0,   0,  0,  0,
        -2, -8, -12, -8, -2,
        -1, -4,  -6, -4, -1,
    };
    Image::data_t normalizing_factor = 16;

    Image gradient_x, gradient_y;
    Image gradient_angle(width, height, 1);
    Image gradient_magnitude(width, height, 1);
    profiler.profile("computing gradient", [&] {
        gradient_x = convolve_2d(greyscale_image, g_x, normalizing_factor);
        gradient_y = convolve_2d(greyscale_image, g_y, normalizing_factor);

        gradient_angle.loop_2d([&] (Image::size_t i, Image::size_t j) {
            gradient_angle(i, j) = std::atan2(gradient_y(i, j), gradient_x(i, j));
        });

        Image::data_t max_magnitude = 0;
        gradient_magnitude.loop_2d([&] (Image::size_t i, Image::size_t j) {
            auto g_x = gradient_x(i, j);
            auto g_y = gradient_y(i, j);
            auto magnitude = std::sqrt(g_x * g_x + g_y * g_y);
            max_magnitude = std::max(max_magnitude, magnitude);
            gradient_magnitude(i, j) = magnitude;
        });
        gradient_magnitude.loop_2d([&] (Image::size_t i, Image::size_t j) {
            gradient_magnitude(i, j) /= max_magnitude;
        });
    });

    profiler.profile("visualizing gradient", [&] {
        Image out(greyscale_image);
        Image::size_t freq = 10, length = 10;
        gradient_angle.loop_2d([&] (Image::size_t i, Image::size_t j) {
            if (i % freq || j % freq) return;
            if (gradient_magnitude(i, j) < 0.1) return;
            auto angle = gradient_angle(i, j);
            Image::ssize_t _i = i + std::cos(angle) * length;
            Image::ssize_t _j = j + std::sin(angle) * length;
            draw_line(out, i, j, _i, _j, {1.0});
            double pi = std::acos(-1);
            draw_line(out, _i, _j, _i - std::cos(angle + pi / 6) * length / 3, _j - std::sin(angle + pi / 6) * length / 3, {1.0});
            draw_line(out, _i, _j, _i - std::cos(angle - pi / 6) * length / 3, _j - std::sin(angle - pi / 6) * length / 3, {1.0});
        });
        save_image(out, "output/1_vis.png");
    });

    profiler.profile("saving grad", [&] {
        Image grad_x(width, height, 3);
        Image grad_y(width, height, 3);
        image.loop_2d([&] (Image::size_t i, Image::size_t j) {
            if (gradient_x(i, j) >= 0) grad_x(i, j, 0) = gradient_x(i, j);
            else grad_x(i, j, 1) = std::abs(gradient_x(i, j));
            if (gradient_y(i, j) >= 0) grad_y(i, j, 0) = gradient_y(i, j);
            else grad_y(i, j, 1) = std::abs(gradient_y(i, j));
        });
        save_image(grad_x, "output/1_x.png");
        save_image(grad_y, "output/1_y.png");
    });

    Image nms_output;
    profiler.profile("Applying nms", [&] {
        
        save_image(gradient_magnitude, "output/1_g.png");
        nms_output = apply_non_maximum_suppression(gradient_magnitude, gradient_angle);
    });

    Image output(nms_output);
    return output;
}