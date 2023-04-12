#include <iostream>
#include <vector>
#include <string>
#include <functional>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Only greyscale images
class Image {
public:
    using data_t = double;
    using size_t = std::size_t;

    Image(size_t _imageX, size_t _imageY) : imageX(_imageX), imageY(_imageY) {
        data.resize(imageX * imageY);
    }

    size_t getImageX() const {
        return imageX;
    }

    size_t getImageY() const {
        return imageY;
    }

    size_t get1dIndex(size_t i, size_t j) const {
        return j * imageX + i;
    }

    data_t& operator() (size_t i) {
        return data[i];
    }

    const data_t& operator() (size_t i) const {
        return data[i];
    }

    data_t& operator() (size_t i, size_t j) {
        return data[get1dIndex(i, j)];
    }

    const data_t& operator() (size_t i, size_t j) const {
        return data[get1dIndex(i, j)];
    }

    void process1d(const std::function<void(size_t)>& process) {
        for (size_t i = 0; i < data.size(); ++i) {
            process(i);
        }
    }

    void process2d(const std::function<void(size_t, size_t)>& process) {
        for (size_t i = 0; i < imageX; ++i) {
            for (size_t j = 0; j < imageY; ++j) {
                process(i, j);
            }
        }
    }

private:
    size_t imageX, imageY;
    std::vector<data_t> data;
};


Image loadImage(std::string path) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data == nullptr) {
        std::cerr << "Failed to load image at path: " + path << std::endl;
    }

    Image image(width, height);
    image.process1d([&](Image::size_t i) {
        int r = data[i * nrChannels];
        int g = data[i * nrChannels + 1];
        int b = data[i * nrChannels + 2];

        image(i) = (r + g + b) / (255.0 * 3.0);
    });

    // RAII :<
    stbi_image_free(data);

    return image;
}

void saveImage(const Image& image, std::string path) {
    auto imageX = image.getImageX();
    auto imageY = image.getImageY();
    int nrChannels = 4;
    auto n = imageX * imageY;

    std::vector<unsigned char> data(n * nrChannels);
    for (std::size_t i = 0; i < n; ++i) {
        unsigned int value = image(i) * 255;
        data[i * nrChannels]     = value;
        data[i * nrChannels + 1] = value;
        data[i * nrChannels + 2] = value;
        data[i * nrChannels + 3] = 255;
    }

    std::string newPath = path.substr(0, path.find_last_of(".")) + ".png";
    if(!stbi_write_png(newPath.c_str(), imageX, imageY, nrChannels, data.data(), imageX * nrChannels)) {
        std::cerr << "Failed to write image at path: " << newPath << std::endl;
        return;
    }
}


int main() {
    auto image = loadImage("input/1.jpg");
    saveImage(image, "output/1.png");

    return 0;
} 