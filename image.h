#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <vector>
#include <string>
#include <filesystem>
#include "debug.h"

#define MAX_CHAR 256

class Image {
public:
    using data_t = double;
    using size_t = unsigned long long;
    using ssize_t = long long;
    using vector = std::vector<data_t>;

    Image() = default;

    Image(size_t __width, size_t __height, size_t __nr_channels) :
        _width(__width), _height(__height), _nr_channels(__nr_channels) {
            _data.resize(_width * _height * _nr_channels);
            std::fill(_data.begin(), _data.end(), 0);
    }

    size_t width() const {
        return _width;
    }

    size_t height() const {
        return _height;
    }

    size_t nr_channels() const {
        return _nr_channels;
    }

    const vector& data() const {
        return _data;
    }

    void set_data(size_t __width, size_t __height, size_t __nr_channels, vector&& __data = {}) {
        _width = __width;
        _height = __height;
        _nr_channels = __nr_channels;
        _data = std::forward<vector>(__data);
    }

    bool is_valid_index(ssize_t i, ssize_t j) const {
        return i >= 0 && j >= 0 && i < _width && j < _height;
    }

    bool is_border_index(ssize_t i, ssize_t j) const {
        return is_valid_index(i, j) && 
               (i == 0 || j == 0 || (i == _width - 1) || (j == _height - 1));
    }

    data_t& operator[] (size_t i) {
        return _data[i];
    }

    const data_t& operator[] (size_t i) const {
        return _data[i];
    }

    data_t& operator() (size_t i, size_t j, size_t channel = 0) {
        return _data[get_1d_index(i, j, channel)];
    }

    const data_t& operator() (size_t i, size_t j, size_t channel = 0) const {
        return _data[get_1d_index(i, j, channel)];
    }

    template<typename Functor>
    void loop_1d(Functor&& f) const {
        for (size_t i = 0; i < _data.size(); ++i) {
            f(i);
        }
    }

    template<typename Functor>
    void loop_2d(Functor&& f) const {
        for (size_t i = 0; i < _width; ++i) {
            for (size_t j = 0; j < _height; ++j) {
                f(i, j);
            }
        }
    }
protected:
    vector& data() {
        return _data;
    }

private:
    size_t get_1d_index(size_t i, size_t j, size_t channel = 0) const {
        return _nr_channels * ((j * _width) + i) + channel;
    }

    size_t _width, _height, _nr_channels;
    vector _data;

    // Debug debug;
};

class Kernel : public Image {
public:
    Kernel(Image::size_t __size) : Image(__size, __size, 1), _size(__size) {}

    Kernel& operator= (Image::vector _data) {
        this->data() = _data;
        return *this;
    }

    Image::size_t size() const {
        return _size;
    }
private:
    Image::size_t _size;
};

Image load_image(std::string path) {
    int width, height, nr_channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nr_channels, 0);

    if (data == nullptr) {
        std::cerr << "Failed to load image at path: " + path << '\n';
    }

    Image image(width, height, nr_channels);
    image.loop_1d([&](Image::size_t i) {
        image[i] = (Image::data_t)data[i] / MAX_CHAR;
    });

    // RAII :<
    stbi_image_free(data);
    return image;
}

void save_image(const Image& image, std::string path) {
    std::vector<unsigned char> data(image.data().size());
    image.loop_1d([&](Image::size_t i) {
        data[i] = static_cast<unsigned char>(image[i] * (MAX_CHAR - 1));
    });

    std::string extension = std::filesystem::path(path).extension().string();
    stbi_flip_vertically_on_write(true);
    if (extension == ".png") {
        if(!stbi_write_png(path.c_str(), image.width(), image.height(), image.nr_channels(), 
                        data.data(), image.width() * image.nr_channels())) {
            std::cerr << "Failed to write image at path: " << path << '\n';
        }
    } else if (extension == ".jpg") {
        if(!stbi_write_jpg(path.c_str(), image.width(), image.height(), image.nr_channels(), 
                        data.data(), 100)) {
            std::cerr << "Failed to write image at path: " << path << '\n';
        }
    }
    else {
        std::cerr << extension << " files not supported." << '\n';
    }
}

Image convolve_2d(const Image& image, const Kernel& kernel, Image::data_t normalizing_factor = 1, bool flip_y = true) {
    Image output{image.width(), image.height(), image.nr_channels()};

    output.loop_2d([&](Image::size_t i, Image::size_t j) {
        for (Image::size_t k = 0; k < output.nr_channels(); ++k) {
            Image::data_t kernel_sum = 0;
            kernel.loop_2d([&](Image::ssize_t x, Image::ssize_t y) {
                Image::ssize_t n = kernel.size();
                auto _y = y;
                if (flip_y) y = n - y - 1;
                Image::ssize_t _i = i + x - n / 2;
                Image::ssize_t _j = j + y - n / 2;
                if (!output.is_valid_index(_i, _j)) return;
                kernel_sum += image(_i, _j, k) * kernel(x, _y);
            });
            output(i, j, k) = kernel_sum / normalizing_factor;
        }
    });

    return output;
}

Image apply_non_maximum_suppression(const Image& gradient_magnitude, const Image& gradient_angle, bool interpolate = false) {
    Image output{gradient_magnitude.width(), gradient_magnitude.height(), gradient_magnitude.nr_channels()};
    const Image::data_t pi = std::acos(-1);

    if (interpolate) {

    } else {
        auto is_between = [pi] (Image::data_t angle, Image::data_t lower, Image::data_t upper) {
            if (angle < 0) angle += pi;
            if (lower < 0) lower += pi;
            if (upper < 0) upper += pi;
            return angle >= lower && angle <= upper;
        };

        output.loop_2d([&](Image::size_t i, Image::size_t j) {
            if (output.is_border_index(i, j)) return;
            auto angle = gradient_angle(i, j);
            Image::data_t g_next, g_prev;
            if (is_between(angle, -pi / 8, pi / 8)) {
                g_next = gradient_magnitude(i + 1, j);
                g_prev = gradient_magnitude(i - 1, j);
            } else if (is_between(angle, pi / 8, 3 * pi / 8)) {
                g_next = gradient_magnitude(i + 1, j + 1);
                g_prev = gradient_magnitude(i - 1, j - 1);
            } else if (is_between(angle, 3 * pi / 8, 5 * pi / 8)) {
                g_next = gradient_magnitude(i, j + 1);
                g_prev = gradient_magnitude(i, j - 1);
            } else {
                g_next = gradient_magnitude(i - 1, j + 1);
                g_prev = gradient_magnitude(i + 1, j - 1);
            }
            auto g_curr = gradient_magnitude(i, j);
            if (g_curr >= g_next && g_curr >= g_prev) output(i, j) = g_curr;
        });
    }
    
    return output;
}

void draw_line(Image& image, 
               Image::ssize_t x0, Image::ssize_t y0,
               Image::ssize_t x1, Image::ssize_t y1,
               const Image::vector& color) {

    auto dx = std::abs(static_cast<Image::ssize_t>(x1 - x0));
    auto sx = x0 < x1 ? 1 : -1;
    auto dy = -std::abs(static_cast<Image::ssize_t>(y1 - y0));
    auto sy = y0 < y1 ? 1 : -1;
    auto error = dx + dy;

    while (true) {
        for (Image::size_t k = 0; k < image.nr_channels(); ++k) {
            if (!image.is_valid_index(x0, y0)) break;
            image(x0, y0, k) = color[k];
        }
        if (x0 == x1 && y0 == y1) break;
        auto e2 = 2 * error;
        if (e2 >= dy) {
            if (x0 == x1) break;
            error += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            if (y0 == y1) break;
            error += dx;
            y0 += sy;
        }
    }
}

#endif