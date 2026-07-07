#include<cstring>
#include<cassert>
#include <set>
#include <iostream>

#include "hrlz/Fingerprint.hpp"

Fingerprint Fingerprint::min_hash_shingle(const DynArray<char>& file, const DynArray<ArrayHash<char>>& hashers) {
  assert(file.size() >= hashers[0].array_size());
  Fingerprint fingerprint(hashers.size());
  for(std::size_t i = 0; i < hashers.size(); ++i) {
    fingerprint._fingerprint[i] = hashers[i](file.begin());
  }
  for(std::size_t i = 1; i < file.size() - hashers[0].array_size(); ++i) {
    auto ptr = file.get() + i;
    for(std::size_t j = 0; j < hashers.size(); ++j) {
      fingerprint._fingerprint[j] = std::min(fingerprint._fingerprint[j], hashers[j](ptr));
    }
  }
  return fingerprint;
}

Fingerprint Fingerprint::bottom_k_hash(const DynArray<char>& file, const TwoArrayHash<uint32_t, char>& hash, int k) {
    int key_whole = hash.array1_size() << 2; //Mult by 4
    int key_remainder = hash.array2_size();
    int key_size = key_whole + key_remainder;
    assert(file.size() >= static_cast<long unsigned int>(key_size));
    Fingerprint fingerprint(k);
    std::set<size_t> bottom_k; //It is ordered
    size_t n = file.size();
    const char *ptr = file.begin();
    for (size_t i = 0; i <= n - key_size; i++) {
        uint32_t hash_value = hash(reinterpret_cast<const uint32_t *>(ptr + i), ptr + key_whole);
        if (bottom_k.size() < k) {
            bottom_k.insert(hash_value);
        } else {
            auto it_largest_val = bottom_k.cend();
            --it_largest_val;   //largest value
            if (*it_largest_val > hash_value) {
                bottom_k.erase(it_largest_val);
                bottom_k.insert(hash_value);
            }
        }
    }
    if (bottom_k.size() < k) {
        std::cout << "Problems in bottom k!" << std::endl;
        std::cerr << "PANIC! Less than k hash values for sequence!!!" << std::endl;
        std::cerr << "Copying largest value in bottom_k" << std::endl;
        uint32_t largest_val = *bottom_k.rbegin();
        for (int i = bottom_k.size(); i < k; i++) {
            fingerprint._fingerprint[i] = largest_val;
        }
    }
    auto it = bottom_k.cbegin();
    for (int i = 0; i < k; i++) {
        fingerprint._fingerprint[i] = *it++;
    }
    return fingerprint;
}

Fingerprint Fingerprint::min_hash_shingle(const DynArray<char>& file, const DynArray<TwoArrayHash<uint32_t, char>>& hashers) {
  std::size_t buf_bytes = hashers[0].array1_size() * 4;
  assert(file.size() >= buf_bytes + hashers[0].array2_size());
  DynArray<uint32_t> buf(hashers[0].array1_size());
  Fingerprint fingerprint(hashers.size());
  memcpy(reinterpret_cast<void*>(buf.get()), reinterpret_cast<const void*>(file.get()), buf_bytes);
  for(std::size_t i = 0; i < hashers.size(); ++i) {
    fingerprint._fingerprint[i] = hashers[i](buf.get(), file.get() + buf_bytes);
  }
  for(std::size_t i = 1; i < file.size() - (buf_bytes+hashers[0].array2_size()); ++i) {
    auto ptr = file.get() + i;
    memcpy(reinterpret_cast<void*>(buf.get()), reinterpret_cast<const void*>(ptr), buf_bytes);
    for(std::size_t j = 0; j < hashers.size(); ++j) {
      auto hash = hashers[j](buf.get(), ptr + buf_bytes);
      fingerprint._fingerprint[j] = std::min(fingerprint._fingerprint[j], hash);
    }
  }
  return fingerprint;
}

bool Fingerprint::operator==(const Fingerprint& rhs) {
  assert(_fingerprint.size() == rhs.size());
  auto eq = true;
  for(std::size_t i = 0; i < _fingerprint.size(); ++i) {
    eq &= _fingerprint[i] == rhs._fingerprint[i];
  }
  return eq;
}
bool Fingerprint::operator!=(const Fingerprint& rhs) {
  return !(*this == rhs);
}
bool Fingerprint::operator<(const Fingerprint& rhs) {
  assert(_fingerprint.size() == rhs.size());
  for(std::size_t i = 0; i < _fingerprint.size(); ++i) {
    if(_fingerprint[i] < rhs._fingerprint[i]) {
      return true;
    }
    if(_fingerprint[i] > rhs._fingerprint[i]) {
      return false;
    }
  }
  return false;
}

/**
 * PRECONDITION: The values of the fingerprint are ordered from smallest to largest.
 * @param rhs The other fingerprint.
 * @return the size of the intersection between the two fingerprints.
 */
size_t Fingerprint::ordered_intersection(const Fingerprint& rhs) {
    size_t minsize = std::min(_fingerprint.size(), rhs.size());
    size_t i = 0, j = 0;
    size_t total = 0;
    while (i < minsize && j < minsize) {
        if (_fingerprint[i] == rhs._fingerprint[j]) {
            total++;
            i++;
            j++;
        } else if (_fingerprint[i] < rhs._fingerprint[j]) {
            i++;
        } else {
            j++;
        }
    }
    return total;
}

std::size_t Fingerprint::size() const noexcept {
  return _fingerprint.size();
}
