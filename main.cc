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
Image convolve(const Image& input, const Kernel& kernel, data_t factor = 1);
std::pair<Image, Image> computeGradient(const Image& image);
Image nonMinimalSuppression(const Image& gm, const Image& ga);
Image doubleThresholding(const Image& image, double lowerThreshold, double upperThreshold);
Image hysteresis(const Image& image);

int main() {
    auto image = loadImage("input/1.jpg");
    auto sizeX = image.getSizeX();
    auto sizeY = image.getSizeY();
    saveImage(image, "output/1.png");

    auto [gradMag, gradAngle] = computeGradient(image);

    Image gradX(sizeX, sizeY), gradY(sizeX, sizeY);
    image.process2d([&](size_t i, size_t j) {
        gradX(i, j) = gradMag(i, j) * std::cos(gradAngle(i, j));
        gradY(i, j) = gradMag(i, j) * std::sin(gradAngle(i, j));
    });
    saveImage(gradX, "output/1_x.png", true);
    saveImage(gradY, "output/1_y.png", true);

    saveImage(gradMag, "output/1_g.png");

    Image nms = nonMinimalSuppression(gradMag, gradAngle);
    saveImage(nms, "output/1_nms.png");

    Image doubleThreshold = doubleThresholding(nms, 0.1, 0.2);
    saveImage(doubleThreshold, "output/1_dt.png");

    Image hyst = hysteresis(doubleThreshold);
    saveImage(hyst, "output/1_hyst.png");

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
Image convolve(const Image& input, const Kernel& kernel, data_t factor) {
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
        output(i, j) = sum / factor;
    });

    return output;
}

std::pair<Image, Image> computeGradient(const Image& image) {
    auto sizeX = image.getSizeX();
    auto sizeY = image.getSizeY();
    Kernel gX(3), gY(3);
    gX = { 1,  0, -1,
           2,  0, -2,
           1,  0, -1};
    gY = { 1,  2,  1,
           0,  0,  0,
          -1, -2, -1};
    data_t factor = 4;

    Image gradMag(sizeX, sizeY), gradAngle(sizeX, sizeY);

    auto gradX = convolve(image, gX, factor);
    auto gradY = convolve(image, gY, factor);

    image.process2d([&](size_t i, size_t j) {
        auto gx = gradX(i, j);
        auto gy = gradY(i, j);
        gradMag(i, j) = std::sqrt(gx * gx + gy * gy);
        gradAngle(i, j) = std::atan2(gy, gx);
    });

    return {gradMag, gradAngle};
}

Image nonMinimalSuppression(const Image& gradMag, const Image& gradAngle) {
    auto sizeX = gradMag.getSizeX();
    auto sizeY = gradMag.getSizeY();
    Image image(sizeX, sizeY);

    image.process2d([&](size_t i, size_t j) {
        if (!image.isValidIndex(i, j, 2)) {
            image(i, j) = 0;
            return;
        }
        data_t m = gradMag(i, j), m1, m2;
        data_t angle = gradAngle(i, j);
        if (angle < 0.0) angle += pi;
        auto slope = std::tan(angle);

        if (angle <= pi / 8 || angle >= 7 * pi / 8) {
            m1 = gradMag(i + 1, j);
            m2 = gradMag(i - 1, j);
        } else if (angle < 3 * pi / 8) {
            m1 = gradMag(i + 1, j + 1);
            m2 = gradMag(i - 1, j - 1);
        } else if (angle < 5 * pi / 2) {
            m1 = gradMag(i, j + 1);
            m2 = gradMag(i, j - 1);
        } else {
            m1 = gradMag(i + 1, j - 1);
            m2 = gradMag(i - 1, j + 1);
        }
        
        image(i, j) = ((m1 > m || m2 > m) ? 0 : m);
    });

    return image;
}

Image doubleThresholding(const Image& image, double lowerThreshold, double upperThreshold) {
    data_t maxValue = 0.0;
    image.process2d([&](size_t i, size_t j) {
        maxValue = std::max(maxValue, image(i, j));
    });
    auto sizeX = image.getSizeX();
    auto sizeY = image.getSizeY();
    Image output(sizeX, sizeY);
    data_t upperValue = 1.0;
    data_t lowerValue = lowerThreshold / upperThreshold;
    output.process2d([&](size_t i, size_t j) {
        if (image(i, j) >= maxValue * upperThreshold) output(i, j) = upperValue;
        else if (image(i, j) >= maxValue * lowerThreshold) output(i, j) = lowerValue;
        else output(i, j) = 0.0;
    });

    return output;
}

Image hysteresis(const Image& image) {
    auto sizeX = image.getSizeX();
    auto sizeY = image.getSizeY();
    Image output(sizeX, sizeY), visited(sizeX, sizeY);

    output.process2d([&](size_t i, size_t j) {
        if (visited(i, j) > 0.0) return;
        visited(i, j) = 1.0;
        if (image(i, j) == 0.0) return;
        else if (image(i, j) == 1.0) output(i, j) = 1.0;
        else {
            bool flag = false;
            std::vector<vec2> points;
            points.push_back({i, j});
            for (auto& p : points) {
                if (visited(p.x, p.y) > 0.0) continue;
                visited(p.x, p.y) = 1.0;
                for (size_t x = -1; x <= 1; ++x) {
                    for (size_t y = -1; y <= 1; ++y) {
                        size_t _i = i + x, _j = j + y;
                        if (!output.isValidIndex(_i, _j) || visited(_i, _j) > 0.0) continue;
                        if (image(_i, _j) == 1.0) flag = true;
                        else if (image(_i, _j) > 0.0) points.push_back({_i, _j});
                    }
                }
            }
            if (flag) {
                for (auto& p : points) {
                    output(p.x, p.y) = 1.0;
                }
            }
        }
    });

    return output;
}