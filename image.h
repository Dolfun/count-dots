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

class Image;
Image load_image(std::string path);
void save_image(const Image& image, std::string path);

class Image {
public:
    using data_t = double;
    using size_t = std::size_t;
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

    void set_data(size_t __width, size_t __height, size_t __nr_channels, vector&& __data) {
        _width = __width;
        _height = __height;
        _nr_channels = _nr_channels;
        _data = std::forward<vector>(__data);
    }

    bool is_valid_index(size_t i, size_t j) const {
        return i >= 0 && j >= 0 && i < _width && j < _height;
    }

    bool is_border_index(size_t i, size_t j) const {
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
private:
    size_t get_1d_index(size_t i, size_t j, size_t channel = 0) const {
        return _nr_channels * ((j * _width) + i) + channel;
    }

    size_t _width, _height, _nr_channels;
    vector _data;

    Debug debug;
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
        data[i] = static_cast<unsigned char>(image[i] * MAX_CHAR);
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

#endif