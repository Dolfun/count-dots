#include <iostream>
#include <string>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "image.h"
using size_t = Image::size_t;
using data_t = Image::data_t;

#define MAX_CHAR 255
const auto pi = std::acos(-1.0);

Image loadImage(std::string path);
void saveImage(const Image& image, std::string path, bool negative = false);
Image convolve(const Image& input, const Kernel& kernel, data_t norm_factor = 1);
std::pair<Image, Image> computeGradient(const Image& image);
Image nonMinimalSuppression(const Image& gradX, const Image& gradY);

int main() {
    auto image = loadImage("input/1.jpg");
    auto sizeX = image.getSizeX();
    auto sizeY = image.getSizeY();
    saveImage(image, "output/1.png");

    auto [gradMagnitude, gradAngle] = computeGradient(image);

    Image gradX(sizeX, sizeY), gradY(sizeX, sizeY);
    image.process2d([&](size_t i, size_t j) {
        gradX(i, j) = gradMagnitude(i, j) * std::cos(gradAngle(i, j));
        gradY(i, j) = gradMagnitude(i, j) * std::sin(gradAngle(i, j));
    });
    saveImage(gradX, "output/1_x.png", true);
    saveImage(gradY, "output/1_y.png", true);

    saveImage(gradMagnitude, "output/1_g.png");

    Image nms = nonMinimalSuppression(gradX, gradY);
    saveImage(nms, "output/1_nms.png");

    return 0;
} 

Image loadImage(std::string path) {
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data == nullptr) {
        std::cerr << "Failed to load image at path: " + path << std::endl;
    }

    Image image(width, height);
    image.process1d([&](size_t i) {
        int r = data[i * nrChannels];
        int g = data[i * nrChannels + 1];
        int b = data[i * nrChannels + 2];

        image(i) = (r + g + b) / (MAX_CHAR * 3.0);
    });

    // RAII :<
    stbi_image_free(data);

    return image;
}

void saveImage(const Image& image, std::string path, bool negative) {
    auto sizeX = image.getSizeX();
    auto sizeY = image.getSizeY();
    int nrChannels = 4;
    auto n = sizeX * sizeY;

    std::vector<unsigned char> data(n * nrChannels);
    if (!negative) {
        for (std::size_t i = 0; i < n; ++i) {
            unsigned int value = image(i) * MAX_CHAR;
            data[i * nrChannels]     = value;
            data[i * nrChannels + 1] = value;
            data[i * nrChannels + 2] = value;
            data[i * nrChannels + 3] = MAX_CHAR;
        }
    } else {
        for (std::size_t i = 0; i < n; ++i) {
            unsigned int p_value, n_value;
            p_value = image(i) * MAX_CHAR;
            n_value = abs(image(i)) * MAX_CHAR;
            data[i * nrChannels] = (image(i) >= 0 ? p_value : 0);
            data[i * nrChannels + 1] = (image(i) >= 0 ? 0 : n_value);
            data[i * nrChannels + 2] = 0;
            data[i * nrChannels + 3] = MAX_CHAR;
        }
    }

    std::string newPath = path.substr(0, path.find_last_of(".")) + ".png";
    stbi_flip_vertically_on_write(true);
    if(!stbi_write_png(newPath.c_str(), sizeX, sizeY, nrChannels, data.data(), sizeX * nrChannels)) {
        std::cerr << "Failed to write image at path: " << newPath << std::endl;
        return;
    }
}

// Odd sized square kernel convulation
Image convolve(const Image& input, const Kernel& kernel, data_t norm_factor) {
    auto sizeX = input.getSizeX();
    auto sizeY = input.getSizeY();
    Image output(sizeX, sizeY);
    output.process2d([&](size_t i, size_t j) {
        data_t sum = 0;
        kernel.process2d([&](size_t x, size_t y) {
            size_t n = kernel.getSize();
            y = n - y - 1;
            auto i_ = i + x - n / 2;
            auto j_ = j + y - n / 2;
            if (!output.isValidIndex(i_, j_)) return;
            sum += input(i_, j_) * kernel(x, y);
        });
        output(i, j) = sum / norm_factor;
    });

    return output;
}

std::pair<Image, Image> computeGradient(const Image& image) {
    Kernel gX(3), gY(3);
    gX = { 1,  0, -1,
           2,  0, -2,
           1,  0, -1};
    gY = { 1,  2,  1,
           0,  0,  0,
          -1, -2, -1};
    data_t norm_factor = 4;

    std::pair<Image, Image> result;
    auto& gradMagnitude = result.first;
    auto& gradAngle = result.second;

    auto gradX = convolve(image, gX, norm_factor);
    auto gradY = convolve(image, gY, norm_factor);

    image.process2d([&](size_t i, size_t j) {
        auto gx = gradX(i, j);
        auto gy = gradY(i, j);
        gradMagnitude(i, j) = std::sqrt(gx * gx + gy * gy);
        gradAngle(i, j) = std::atan2(gy, gx);
    });

    return result;
}

Image nonMinimalSuppression(const Image& gradX, const Image& gradY) {

}