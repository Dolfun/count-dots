#include <iostream>
#include <vector>
#include <string>
#include <functional>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MAX_CHAR 255

// Greyscale image class
class Image {
public:
    using data_t = double;
    using size_t = std::size_t;
    using vector = std::vector<data_t>;

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

    bool isValidIndex(size_t i, size_t j) const {
        return (i > 0) && (j > 0) && (i < imageX) && (j < imageY);
    }

    const vector getData() const {
        return data;
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

    template<typename Functor>
    void process1d(Functor&& f) const {
        for (size_t i = 0; i < data.size(); ++i) {
            f(i);
        }
    }

    template<typename Functor>
    void process2d(Functor&& f) const {
        for (size_t i = 0; i < imageX; ++i) {
            for (size_t j = 0; j < imageY; ++j) {
                f(i, j);
            }
        }
    }

protected:
    vector& getData() {
        return data;
    }

private:
    size_t imageX, imageY;
    vector data;
};

using size_t = Image::size_t;
using data_t = Image::data_t;

class Kernel : public Image {
public:
    Kernel(size_t _size) : Image(_size, _size), size(_size) {}

    void operator= (const vector& input) {
        getData() = input;
    }

    size_t getSize() const {
        return size;
    }
private:
    size_t size;
};



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

void saveImage(const Image& image, std::string path, bool negative = false) {
    auto imageX = image.getImageX();
    auto imageY = image.getImageY();
    int nrChannels = 4;
    auto n = imageX * imageY;

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
    if(!stbi_write_png(newPath.c_str(), imageX, imageY, nrChannels, data.data(), imageX * nrChannels)) {
        std::cerr << "Failed to write image at path: " << newPath << std::endl;
        return;
    }
}

// Odd sized square kernel convulation
Image convolve(const Image& input, const Kernel& kernel, data_t norm_factor = 1) {
    auto imageX = input.getImageX();
    auto imageY = input.getImageY();
    Image output(imageX, imageY);
    output.process2d([&](size_t i, size_t j) {
        data_t sum = 0;
        size_t n = kernel.getImageX();
        kernel.process2d([&](size_t x, size_t y) {
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

int main() {
    auto image = loadImage("input/1.jpg");
    auto imageX = image.getImageX();
    auto imageY = image.getImageY();

    Kernel gX(3), gY(3);
    gX = { 1,  0, -1,
           2,  0, -2,
           1,  0, -1};
    gY = { 1,  2,  1,
           0,  0,  0,
          -1, -2, -1};
    data_t norm_factor = 4;
    auto gradX = convolve(image, gX, norm_factor);
    auto gradY = convolve(image, gY, norm_factor);

    Image gradient(image);
    gradient.process2d([&](size_t i, size_t j) {
        data_t x = gradX(i, j);
        data_t y = gradY(i, j);
        gradient(i, j) = std::sqrt(x * x + y * y);
    });

    saveImage(image, "output/1.png");
    saveImage(gradX, "output/1_x.png", true);
    saveImage(gradY, "output/1_y.png", true);
    saveImage(gradient, "output/1_g.png");
    return 0;
} 