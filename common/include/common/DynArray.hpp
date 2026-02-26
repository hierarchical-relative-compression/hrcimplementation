#pragma once
#include<utility>
#include<memory>

template <class T>
class DynArray {
public:
  DynArray() = default;
  DynArray(const std::size_t size): _ptr(size ? new T[size] : nullptr), _size(size) {}
  DynArray(DynArray&& moved) noexcept: _ptr(std::move(moved._ptr)), _size(moved._size) {
    moved._size = 0;
  }
  auto& operator=(DynArray&& moved) noexcept {
    if(this != &moved) {
      _ptr = std::move(moved._ptr);
      _size = moved._size;
      moved._size = 0;
    }
    return *this;
  }
  auto size() const noexcept {
    return this->_size;
  }
  auto* get() noexcept {
    return _ptr.get();
  }
  auto const* get() const noexcept {
    return _ptr.get();
  }
  auto* begin() noexcept {
    return _ptr.get();
  }
  auto const* begin() const noexcept {
    return _ptr.get();
  }
  auto const* end() const noexcept {
    return _ptr.get() + _size;
  }
  auto* end() noexcept {
    return _ptr.get() + _size;
  }

  auto const& operator[](size_t i) const noexcept {
    return _ptr[i];
  }

  auto& operator[](size_t i) noexcept {
    return _ptr[i];
  }

  /**
   * WARNING: Only use if the data stored is actually
   * smaller than the buffer size.
  */
  void set_size(size_t data_size) noexcept {
    _size = data_size;
  }

private:
  std::unique_ptr<T[]> _ptr = nullptr;
  size_t _size = 0;
};
