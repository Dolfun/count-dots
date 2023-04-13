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

    Image(size_t _sizeX, size_t _sizeY) : sizeX(_sizeX), sizeY(_sizeY) {
        data.resize(sizeX * sizeY);
    }

    size_t getSizeX() const {
        return sizeX;
    }

    size_t getSizeY() const {
        return sizeY;
    }

    size_t get1dIndex(size_t i, size_t j) const {
        return j * sizeX + i;
    }

    bool isValidIndex(size_t i, size_t j, size_t padding = 0) const {
        return (i > padding) && (j > padding) && (i < sizeX - padding) && (j < sizeY - padding);
    }

    bool isBorderIndex(size_t i, size_t j) const {
        return (i == 0) || (j == 0) || (i == sizeX - 1) || (j == sizeY - 1);
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
        for (size_t i = 0; i < sizeX; ++i) {
            for (size_t j = 0; j < sizeY; ++j) {
                f(i, j);
            }
        }
    }

protected:
    vector& getData() {
        return data;
    }

private:
    size_t sizeX, sizeY;
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