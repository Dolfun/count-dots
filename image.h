#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <filesystem>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include "debug.h"

#define pi std::acos(-1)

#define MAX_CHAR 256

class Image {
public:
    using vector = std::vector<float>;

    Image() = default;

    Image(int __width, int __height, int __nr_channels) :
        _width(__width), _height(__height), _nr_channels(__nr_channels) {
            _data.resize(_width * _height * _nr_channels);
            std::fill(_data.begin(), _data.end(), 0);
    }

    int width() const {
        return _width;
    }

    int height() const {
        return _height;
    }

    int nr_channels() const {
        return _nr_channels;
    }

    const vector& data() const {
        return _data;
    }

    void set_data(int __width, int __height, int __nr_channels, vector&& __data = {}) {
        _width = __width;
        _height = __height;
        _nr_channels = __nr_channels;
        _data = std::forward<vector>(__data);
    }

    int get_1d_index(int i, int j, int channel = 0) const {
        return _nr_channels * ((j * _width) + i) + channel;
    }

    bool is_valid_index(int i, int j, int padding = 0) const {
        return i >= padding && j >= padding && i < _width - padding && j < _height - padding;
    }

    bool is_border_index(int i, int j) const {
        return is_valid_index(i, j) && 
               (i == 0 || j == 0 || (i == _width - 1) || (j == _height - 1));
    }

    float& operator[] (int i) {
        return _data[i];
    }

    const float& operator[] (int i) const {
        return _data[i];
    }

    float& operator() (int i, int j, int channel = 0) {
        return _data[get_1d_index(i, j, channel)];
    }

    const float& operator() (int i, int j, int channel = 0) const {
        return _data[get_1d_index(i, j, channel)];
    }

    template<typename Functor>
    void loop_1d(Functor&& f) const {
        for (int i = 0; i < _data.size(); ++i) {
            std::forward<Functor&&>(f)(i);
        }
    }

    template<typename Functor>
    void loop_2d(Functor&& f) const {
        for (int i = 0; i < _width; ++i) {
            for (int j = 0; j < _height; ++j) {
                std::forward<Functor&&>(f)(i, j);
            }
        }
    }
protected:
    vector& data() {
        return _data;
    }

private:
    int _width, _height, _nr_channels;
    vector _data;

    // Debug debug;
};

class Kernel : public Image {
public:
    Kernel(int __size) : Image(__size, __size, 1), _size(__size) {}

    Kernel& operator= (Image::vector _data) {
        this->data() = _data;
        return *this;
    }

    int size() const {
        return _size;
    }
private:
    int _size;
};

struct vec2 {
    vec2() = default;
    
    vec2(int _x, int _y) : x(_x), y(_y) {}

    int x, y;
};

Image load_image(std::string path) {
    int width, height, nr_channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nr_channels, 0);

    if (data == nullptr) {
        std::cerr << "Failed to load image at path: " + path << '\n';
    }

    Image image(width, height, nr_channels);
    image.loop_1d([&](int i) {
        image[i] = (float)data[i] / (MAX_CHAR - 1);
    });

    // RAII :<
    stbi_image_free(data);
    return image;
}

void save_image(const Image& image, std::string path) {
    std::vector<unsigned char> data(image.data().size());
    image.loop_1d([&](int i) {
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

Image convolve_2d(const Image& image, const Kernel& kernel, float normalizing_factor = 1, bool flip_y = true) {
    Image output{image.width(), image.height(), image.nr_channels()};

    output.loop_2d([&](int i, int j) {
        for (int k = 0; k < output.nr_channels(); ++k) {
            float kernel_sum = 0;
            kernel.loop_2d([&](int x, int y) {
                int n = kernel.size();
                auto _y = y;
                if (flip_y) y = n - y - 1;
                int _i = i + x - n / 2;
                int _j = j + y - n / 2;
                if (!output.is_valid_index(_i, _j)) return;
                kernel_sum += image(_i, _j, k) * kernel(x, _y);
            });
            output(i, j, k) = kernel_sum / normalizing_factor;
        }
    });

    return output;
}

float compute_threshold(const Image& image, int nr_bins = 256) {
    float nr_pixels = image.width() * image.height();
    std::vector<int> histogram(nr_bins, 0);
    image.loop_1d([&](int i) {
        ++histogram[static_cast<int>(image[i] * (nr_bins - 1))];
    });

    std::vector<float> sum_p(nr_bins), sum_pi(nr_bins);
    sum_p[0] = histogram[0] / nr_pixels;
    sum_pi[0] = 0;

    for (int i = 1; i < nr_bins; ++i) {
        sum_p[i] = sum_p[i - 1] + histogram[i] / nr_pixels;
        sum_pi[i] = sum_pi[i - 1] + i * histogram[i] / nr_pixels;
    }
    auto mu_t = sum_pi.back();

    float max_inter_class_variance = 0;
    int level;
    for (int i = 0; i < nr_bins; ++i) {
        auto w_0 = sum_p[i];
        auto w_1 = 1.0 - w_0;
        auto mu_0 = sum_pi[i] / w_0;
        auto mu_1 = (mu_t - sum_pi[i]) / w_1;

        auto inter_class_variance = w_0 * w_1 * (mu_0 - mu_1) * (mu_0 - mu_1);
        if (inter_class_variance > max_inter_class_variance) {
            max_inter_class_variance = inter_class_variance;
            level = i;
        }
    }

    float threshold = static_cast<float>(level) / (nr_bins - 1);
    return threshold;
}

Image apply_thresholding(const Image& image, float threshold) {
    Image output{image.width(), image.height(), 1};
    output.loop_1d([&](int i) {
        if (image[i] >= threshold) output[i] = 1.0;
    });
    return output;
}

template<typename T>
std::pair<T, T> solve_linear_equations(T a1, T b1, T c1, T a2, T b2, T c2) {
    T det = a1 * b2 - b1 * a2;
    T x = (b1 * c2 - c1 * b2) / det;
    T y = (c1 * a2 - a1 * c2) / det;
    return {x, y};
}

std::pair<float, float> compute_best_fit_line(std::vector<vec2> points) {
    float s_x, s_y, s_xy, s_x2, n;
    s_x = s_y = s_xy = s_x2 = 0.0;
    n = points.size();
    for (auto& p : points) {
        s_x += p.x;
        s_y += p.y;
        s_xy += p.x * p.y;
        s_x2 += p.x * p.x;
    }

    return solve_linear_equations(s_x2, s_x, -s_xy, s_x, n, -s_y);
}

template <typename Functor>
void draw_line(int x0, int y0,
               int x1, int y1,
               Functor&& f) {

    auto dx = std::abs(static_cast<int>(x1 - x0));
    auto sx = x0 < x1 ? 1 : -1;
    auto dy = -std::abs(static_cast<int>(y1 - y0));
    auto sy = y0 < y1 ? 1 : -1;
    auto error = dx + dy;

    while (true) {
        if (std::forward<Functor&&>(f)(x0, y0)) break;

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

using endpoints_t = std::vector<std::pair<int, int>>;
void sort_endpoints(endpoints_t& endpoints) {
    std::sort(endpoints.begin(), endpoints.end(), [] (const auto& p1, const auto& p2) {
        return p1.first < p2.first;
    });
}

endpoints_t process_endpoints(endpoints_t endpoints, std::vector<float>& differences) {
    sort_endpoints(endpoints);
    endpoints.emplace_back(INT_MAX, INT_MAX);

    std::map<int, int> upper_endpoints;
    std::vector<int> lower_endpoints;
    int sum_lower = endpoints[0].first, sum_upper = endpoints[0].second;
    int count = 1;
    for (int i = 1; i < endpoints.size(); ++i) {
        if (endpoints[i].first - endpoints[i - 1].first <= 1) {
            sum_lower += endpoints[i].first;
            sum_upper += endpoints[i].second;
            ++count;
        } else {
            int lower_endpoint = sum_lower / count, upper_endpoint = sum_upper / count;
            lower_endpoints.push_back(lower_endpoint);
            upper_endpoints[lower_endpoint] = upper_endpoint;

            sum_lower = endpoints[i].first, sum_upper = endpoints[i].second;
            count = 1;
        }
    }

    endpoints_t new_endpoints;
    for (auto& x : lower_endpoints) {
        new_endpoints.emplace_back(x, upper_endpoints[x]);
    }
    for (int i = 1; i < new_endpoints.size(); ++i) {
        float e1 = (new_endpoints[i - 1].first + new_endpoints[i - 1].second) / 2.0;
        float e2 = (new_endpoints[i].first + new_endpoints[i].second) / 2.0;
        differences.push_back(e2 - e1);
    }
    return new_endpoints;
}

#endif