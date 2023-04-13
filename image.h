#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED

#include <vector>

// Greyscale image class
class Image {
public:
    using data_t = double;
    using size_t = std::size_t;
    using vector = std::vector<data_t>;

    Image() = default;

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

    bool isBorderIndex(size_t i, size_t j) const {
        return (i == 0) || (j == 0) || (i == imageX - 1) || (j == imageY - 1);
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

#endif