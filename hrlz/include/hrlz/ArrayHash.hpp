#pragma once
//Author: Simon Rumle Tarnow
//Commented by Máximo Pérez López

#include<type_traits>
#include<cstdint>
#include<random>
#include<chrono>

#include "common/DynArray.hpp"
#include "utils/GlobalRandom.hpp"



template<typename KeyType>
uint64_t PairMultSum(const KeyType* ar, const uint64_t* multipliers, std::size_t array_size) {
  uint64_t sum = 0;
  for(std::size_t i = 0; i + 1 < array_size; i+= 2) {
    sum += (ar[i] + multipliers[i+1])*(ar[i+1] + multipliers[i]);
  }
  return sum;
}


template<typename KeyType>
class ArrayHash {
  static_assert(std::is_integral<KeyType>::value, "Integral required.");
  static_assert(sizeof(KeyType) * 8 <= 32, "Integral has to be less than or equal to 32 bits in size.");
private:
  DynArray<uint64_t> _multipliers;
  uint64_t _add;
public:
  ArrayHash(): _multipliers(0), _add(0) {}
  ArrayHash(const std::size_t array_size): _multipliers(array_size), _add(0) {
    unsigned seed = GlobalRandom::get(); //before: std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine gen(seed);
    std::uniform_int_distribution<uint64_t> dist;
    for(std::size_t i = 0; i < _multipliers.size(); ++i) {
      _multipliers[i] =  dist(gen);
    }
    _add = dist(gen);
  }
  std::size_t array_size() const noexcept {
    return _multipliers.size();
  }
  uint32_t operator()(const KeyType* ar) const noexcept {
    uint64_t sum = PairMultSum(ar, _multipliers.get(), array_size());
    if (array_size() % 2) {
      sum += ar[array_size()-1] * _multipliers[array_size()-1];
    }
    return (sum + _add) >> (64 - 32);
  }
};
/**
 * Max: TwoArrayHash makes it possible to have a hash function that works over e.g. 32-bit integers, and apply it to a
 * whole array of chars (f.ex. k=31 chars), whose length may not be a multiple of 4. The intended use is that array1_size is the quotient
 * of the length of the array by the nº of bytes of the hash function, while array2_size is the remainder of that
 * division (the last characters that don't fill a full key).
 * @tparam KeyType1
 * @tparam KeyType2
 */
template<typename KeyType1, typename KeyType2>
class TwoArrayHash {
  static_assert(std::is_integral<KeyType1>::value, "Integral required.");
  static_assert(sizeof(KeyType1) * 8 <= 32, "Integral has to be less than or equal to 32 bits in size.");
  static_assert(std::is_integral<KeyType2>::value, "Integral required.");
  static_assert(sizeof(KeyType2) * 8 <= 32, "Integral has to be less than or equal to 32 bits in size.");
private:
  DynArray<uint64_t> _multipliers;
  std::size_t size1;
  uint64_t _add;
public:
  TwoArrayHash(): _multipliers(0), _add(0) {}
  TwoArrayHash(const std::size_t array1_size, const std::size_t array2_size): _multipliers(array1_size + array2_size), size1(array1_size), _add(0) {
    unsigned seed = GlobalRandom::get(); //std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine gen(seed);
    std::uniform_int_distribution<uint64_t> dist;
    for(std::size_t i = 0; i < _multipliers.size(); ++i) {
      _multipliers[i] =  dist(gen);
    }
    _add = dist(gen);
  }
  std::size_t array1_size() const noexcept {
    return size1;
  }
  std::size_t array2_size() const noexcept {
    return _multipliers.size() - size1;
  }
  uint32_t operator()(const KeyType1* ar1, const KeyType2* ar2) const noexcept {
    uint64_t sum = PairMultSum(ar1, _multipliers.get(), array1_size());
    sum += PairMultSum(ar2, _multipliers.get() + size1, array2_size());
    if (array1_size() % 2) {
      auto last = array1_size() - 1;
      sum += ar1[last] * _multipliers[last];
    }
    if (array2_size() % 2) {
      auto last = array1_size() + array2_size() - 1;
      sum += ar2[last] * _multipliers[last];
    }
    return (sum + _add) >> (64 - 32);
  }
};
