#include <iostream>
#include <fstream>
#include <random>
#include "image.h"
#include "debug.h"

Image process_image(const Image& image, Profiler& profiler);

int main() {
    Profiler profiler;
    profiler.start();

    Image image;
    profiler.profile("loading image", [&] {
        image = load_image("input/1.jpg");
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
        greyscale_image.loop_2d([&] (int i, int j) {
            greyscale_image(i, j) = (image(i, j, 0) + image(i, j, 1) + image(i, j, 2)) / 3;
        });
    });

    Image smooth_image{width, height, 1};
    profiler.profile("gaussian blur", [&] {
        Kernel gaussian_filter(5);
        gaussian_filter = {
            2, 4, 5, 4, 2,
            4, 9, 12, 9, 4,
            5, 12, 15, 12, 5,
            4, 9, 12, 9, 4,
            2, 4, 5, 4, 2,
        };
        smooth_image = convolve_2d(greyscale_image, gaussian_filter, 159);
        smooth_image = greyscale_image;
    });

    Image binary_image;
    profiler.profile("thresholding", [&] {
        float th = compute_threshold(smooth_image, 1000);
        std::cout << "threshold: " << th << '\n';
        binary_image = apply_thresholding(smooth_image, th);
        save_image(binary_image, "output/1_bin.png");
    });

    endpoints_t raw_x_endpoints, raw_y_endpoints;
    profiler.profile("raw endpoints", [&] {
        int max_overlaps = 10;

        for (int i = 1; i < width - 1; ++i) {
            for (int j = 1; j < width - 1; ++j) {
                bool flag = true;
                int curr_overlaps = 0;
                draw_line(i, 0, j, height - 1, [&] (int x, int y) {
                    if (binary_image(x, y) == 1.0) ++curr_overlaps;
                    if (binary_image(x, y) == 0.0) curr_overlaps = 0;
                    if (curr_overlaps > max_overlaps) {
                        flag = false;
                        return -1;
                    }
                    return 0;
                });
                if (flag) {
                    raw_x_endpoints.emplace_back(i, j);
                }
            }
        }

        for (int i = 1; i < height - 1; ++i) {
            for (int j = 1; j < height - 1; ++j) {
                bool flag = true;
                int curr_overlaps = 0;
                draw_line(0, i, width - 1, j, [&] (int x, int y) {
                    if (binary_image(x, y) == 1.0) ++curr_overlaps;
                    if (binary_image(x, y) == 0.0) curr_overlaps = 0;
                    if (curr_overlaps > max_overlaps) {
                        flag = false;
                        return -1;
                    }
                    return 0;
                });
                if (flag) {
                    raw_y_endpoints.emplace_back(i, j);
                }
            }
        }
    });

    std::vector<float> differences;
    endpoints_t x_endpoints, y_endpoints;
    endpoints_t interpolated_x_endpoints, interpolated_y_endpoints;
    endpoints_t extrapolated_x_endpoints, extrapolated_y_endpoints;
    float max_flt = std::max(width, height);
    float grid_width = max_flt;
    profiler.profile("processing endpoints", [&] {
        x_endpoints = process_endpoints(raw_x_endpoints, differences);
        y_endpoints = process_endpoints(raw_y_endpoints, differences);

        std::sort(differences.begin(), differences.end());
        differences.push_back(max_flt);
        float max_difference = 10;
        int curr_count = 1;
        float curr_sum = differences[0];
        for (int i = 1; i < differences.size(); ++i) {
            if (differences[i] - differences[i - 1] <= max_difference) {
                ++curr_count;
                curr_sum += differences[i];
            } else {
                grid_width = std::min(grid_width, curr_sum / curr_count);

                curr_count = 1;
                curr_sum = differences[i];
            }
        }
    });

    profiler.profile("interpolating", [&] {
        for (int i = 0, n = x_endpoints.size(); i < n - 1; ++i) {
            float x1_l = x_endpoints[i].first, x1_u = x_endpoints[i].second;
            float x2_l = x_endpoints[i + 1].first, x2_u = x_endpoints[i + 1].second;
            int nr_lines = std::round((x2_l - x1_l) / grid_width) - 1;
            for (int j = 1; j <= nr_lines; ++j) {
                float t = (float)j / (nr_lines + 1);
                float m1 = (height - 1) / (x1_u - x1_l);
                float m2 = (height - 1) / (x2_u - x2_l);
                float x1_m = (x1_l + x1_u) / 2;
                float x2_m = (x2_l + x2_u) / 2;
                float mt = std::lerp(m1, m2, t);
                float xt_m = std::lerp(x1_m, x2_m, t);
                float xt_l = xt_m - (height - 1) / (2 * mt);
                float xt_u = xt_m + (height - 1) / (2 * mt);
                interpolated_x_endpoints.push_back({(int)std::round(xt_l), (int)std::round(xt_u)});
            }
        }

        for (int i = 0, n = y_endpoints.size(); i < n - 1; ++i) {
            float y1_l = y_endpoints[i].first, y1_u = y_endpoints[i].second;
            float y2_l = y_endpoints[i + 1].first, y2_u = y_endpoints[i + 1].second;
            int nr_lines = std::round((y2_l - y1_l) / grid_width) - 1;
            for (int j = 1; j <= nr_lines; ++j) {
                float t = (float)j / (nr_lines + 1);
                float m1 = (y1_u - y1_l) / (width - 1);
                float m2 = (y2_u - y2_l) / (width - 1);
                float y1_m = (y1_l + y1_u) / 2;
                float y2_m = (y2_l + y2_u) / 2;
                float mt = std::lerp(m1, m2, t);
                float yt_m = std::lerp(y1_m, y2_m, t);
                float yt_l = yt_m - mt * (width - 1) / 2;
                float yt_u = yt_m + mt * (width - 1) / 2;
                interpolated_y_endpoints.push_back({(int)std::round(yt_l), (int)std::round(yt_u)});
            }
        }
    });

    profiler.profile("extrapolating", [&] {
        for (int i = 1;; ++i) {
            float x_l = x_endpoints.front().first, x_u = x_endpoints.front().second;
            float m = (height - 1) / (x_u - x_l);
            float x0 = (x_l + x_u) / 2 - grid_width * i;
            float xt_l = x0 - (height - 1) / (2 * m);
            float xt_u = x0 + (height - 1) / (2 * m);
            if (xt_l < 0 && xt_u < 0) break;
            extrapolated_x_endpoints.push_back({(int)std::round(xt_l), (int)std::round(xt_u)});
        }

        for (int i = 1;; ++i) {
            float x_l = x_endpoints.back().first, x_u = x_endpoints.back().second;
            float m = (height - 1) / (x_u - x_l);
            float x0 = (x_l + x_u) / 2 + grid_width * i;
            float xt_l = x0 - (height - 1) / (2 * m);
            float xt_u = x0 + (height - 1) / (2 * m);
            if (xt_l > width - 1 && xt_u > width - 1) break;
            extrapolated_x_endpoints.push_back({(int)std::round(xt_l), (int)std::round(xt_u)});
        }

        for (int i = 1;; ++i) {
            float y_l = y_endpoints.front().first, y_u = y_endpoints.front().second;
            float m = (y_u - y_l) / (width - 1);
            float y0 = (y_l + y_u) / 2 - grid_width * i;
            float yt_l = y0 - m * (width - 1) / 2;
            float yt_u = y0 + m * (width - 1) / 2;
            if (yt_l < 0 && yt_u < 0) break;
            extrapolated_y_endpoints.push_back({(int)std::round(yt_l), (int)std::round(yt_u)});
        }

        for (int i = 1;; ++i) {
            float y_l = y_endpoints.back().first, y_u = y_endpoints.back().second;
            float m = (y_u - y_l) / (width - 1);
            float y0 = (y_l + y_u) / 2 + grid_width * i;
            float yt_l = y0 - m * (width - 1) / 2;
            float yt_u = y0 + m * (width - 1) / 2;
            if (yt_l > height - 1 && yt_u > height - 1) break;
            extrapolated_y_endpoints.push_back({(int)std::round(yt_l), (int)std::round(yt_u)});
        }
    });

    Image main_grid_image{width, height, 3};
    profiler.profile("result", [&] () {
        std::vector<std::pair<vec2, vec2>> final_endpoints;
        x_endpoints.insert(x_endpoints.end(), interpolated_x_endpoints.begin(), interpolated_x_endpoints.end());
        x_endpoints.insert(x_endpoints.end(), extrapolated_x_endpoints.begin(), extrapolated_x_endpoints.end());
        y_endpoints.insert(y_endpoints.end(), interpolated_y_endpoints.begin(), interpolated_y_endpoints.end());
        y_endpoints.insert(y_endpoints.end(), extrapolated_y_endpoints.begin(), extrapolated_y_endpoints.end());
        for (auto& p : x_endpoints) {
            final_endpoints.emplace_back(vec2(p.first, 0), vec2(p.second, height - 1));
        }
        for (auto& p : y_endpoints) {
            final_endpoints.emplace_back(vec2(0, p.first), vec2(width - 1, p.second));
        }
        for (auto [p1, p2] : final_endpoints) {
            draw_line(p1.x, p1.y, p2.x, p2.y, [&] (int i, int j) {
                if (!main_grid_image.is_valid_index(i, j)) return 0;
                main_grid_image(i, j, 1) = 1.0;
                return 0;
            });
        }
    });

    Image output(main_grid_image);
    return output;
}