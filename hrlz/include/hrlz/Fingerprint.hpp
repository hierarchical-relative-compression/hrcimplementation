#pragma once
#include "common/DynArray.hpp"
#include "hrlz/ArrayHash.hpp"

class Fingerprint {
public:
  Fingerprint(): _fingerprint(0) {}
  Fingerprint(std::size_t size): _fingerprint(size) {}

  static Fingerprint min_hash_shingle(const DynArray<char>& file, const DynArray<ArrayHash<char>>& hashers);

  static Fingerprint min_hash_shingle(const DynArray<char>& file, const DynArray<TwoArrayHash<uint32_t, char>>& hashers);

  static Fingerprint bottom_k_hash(const DynArray<char>& file, const TwoArrayHash<uint32_t, char>& hash, int k);

  bool operator==(const Fingerprint& rhs);
  bool operator!=(const Fingerprint& rhs);
  bool operator<(const Fingerprint& rhs);

  size_t ordered_intersection(const Fingerprint& rhs);

  std::size_t size() const noexcept;
private:
  DynArray<uint32_t> _fingerprint;
};
