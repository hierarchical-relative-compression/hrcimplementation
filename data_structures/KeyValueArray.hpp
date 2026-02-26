//
// Created by maximo on 06-03-25.
//

#include <cstddef>
#include <cstdlib>

#ifndef KEYVALUEARRAY_HPP
#define KEYVALUEARRAY_HPP

template <typename KeyInt, typename ValInt>
struct KeyValEntry {
    KeyInt key;
    ValInt value;
};

template <typename KeyInt, typename ValInt>
class KeyValueArray {
private:
    int keyBytes { 4 };
    int valBytes { 4 };
    int entrySize { 8 };
    size_t n { 0 };
    char *array { nullptr };
    KeyInt keyMask { -1ULL };
    KeyInt keyMaskInv { 0 };
    ValInt valMask { -1ULL };
    ValInt valMaskInv { 0 };
public:
    KeyValueArray() {}
    KeyValueArray(int keyBytes, int valBytes, size_t n)
    : keyBytes { keyBytes }
    , valBytes { valBytes }
    , entrySize { keyBytes + valBytes }
    , n { n }
    //The last addition is to make an assignment to the last value not write to unallocated memory
    , array { static_cast<char*>(std::malloc(n * entrySize + (sizeof(ValInt) - valBytes))) } {
        //Compute the byte mask for reading keys and values
        if (keyBytes == sizeof(KeyInt)) {
            keyMask = -1;
        } else {
            keyMask = (1ULL << (keyBytes << 3)) - 1;
        }
        if (valBytes == sizeof(ValInt)) {
            valMask = -1;
        } else {
            valMask = (1ULL << (valBytes << 3)) - 1;
        }
        keyMaskInv = ~keyMask;
        valMaskInv = ~valMask;
    }

    ~KeyValueArray() {
        if (array != nullptr) {
            free(array);
            array = nullptr;
        }
    }
    //Other constructors
    KeyValueArray(const KeyValueArray& other) = delete; //No copy constructor
    KeyValueArray& operator=(const KeyValueArray& other) = delete; //No copy assignment constructor

    KeyValueArray(KeyValueArray&& other) noexcept //Move constructor
        : keyBytes { other.keyBytes }
        , valBytes { other.valBytes }
        , entrySize { other.entrySize }
        , n { other.n }
        , array { other.array }
        , keyMask { other.keyMask }
        , keyMaskInv { other.keyMaskInv }
        , valMask { other.valMask }
        , valMaskInv { other.valMaskInv }
    {
        other.array = nullptr;
    }
    KeyValueArray& operator=(KeyValueArray&& other) noexcept { //Move assignment constructor
        if (&other == this)
            return *this;
        keyBytes = other.keyBytes;
        valBytes = other.valBytes;
        entrySize = other.entrySize;
        n = other.n;
        array = other.array;
        keyMask = other.keyMask;
        keyMaskInv = other.keyMaskInv;
        valMask = other.valMask;
        valMaskInv = other.valMaskInv;
        other.array = nullptr;
        return *this;
    }

    void boundCheck(size_t index) const {
        if (index > n) {
            std::string error = "Index out of bounds error at KeyValueArray: capacity is ";
            error += n + " but accessed " + index;
            throw std::runtime_error(error.c_str());
        }
    }

    KeyValEntry<KeyInt, ValInt> get(size_t index) const {
        boundCheck(index);
        size_t p = entrySize * index;
        return KeyValEntry<KeyInt, ValInt> { *reinterpret_cast<KeyInt *>(array + p) & keyMask,
                                       *reinterpret_cast<ValInt *>(array + p + keyBytes) & valMask};
    }

    KeyInt getKey(size_t index) const {
        boundCheck(index);
        return *(reinterpret_cast<KeyInt *>(array + entrySize * index)) & keyMask;
    }

    ValInt getValue(size_t index) const {
        boundCheck(index);
        return *(reinterpret_cast<ValInt *>(array + entrySize * index + keyBytes)) & valMask;
    }

    void set(size_t index, KeyInt key, ValInt value) {
        boundCheck(index);
        KeyInt *p = reinterpret_cast<KeyInt *>(array + index * entrySize);
        *p = (*p & keyMaskInv) | key;
        ValInt *v = reinterpret_cast<ValInt *>(reinterpret_cast<char*>(p) + keyBytes);
        *v = (*v & valMaskInv) | value;
    }

    void setKey(size_t index, KeyInt key) {
        boundCheck(index);
        KeyInt *p = reinterpret_cast<KeyInt *>(array + index * entrySize);
        *p = (*p & keyMaskInv) | key;
    }

    void setValue(size_t index, ValInt value) {
        boundCheck(index);
        ValInt *p = reinterpret_cast<ValInt *>(array + index * entrySize + keyBytes);
        *p = (*p & valMaskInv) | value;
    }
};

#endif //KEYVALUEARRAY_HPP
