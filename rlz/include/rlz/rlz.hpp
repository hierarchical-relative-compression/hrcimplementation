#pragma once
#include"common.h"
#include"factor.hpp"
#include "common/DynArray.hpp"
#include "RefDataStructure.hpp"
#include <filesystem>
#include <cstddef>

class RLZ {
private:
    void parseDNA(std::istream& file);
protected:
  RLZ(DynArray<data_type>&& ref_seq): _ref_seq(std::move(ref_seq)) {
      std::cout << "Using old constructor!!!" << std::endl;
  };
  RLZ(const std::filesystem::path& ref_seq);
  RLZ(std::istream& in);
  DynArray<data_type> _ref_seq;
public:
    data_type *last_ch;
};

class RLZCompress: public RLZ, public RefDataStructure {
public:
    RLZCompress(const std::filesystem::path& path, bool decomp);
    RLZCompress(std::istream& in, bool decomp);
    ~RLZCompress();

    void parseRef(std::istream& in) override;
    void compress_target(const std::filesystem::path& in, std::ostream& out) const override;
    void decompress_target(std::istream& in, std::ostream& out) const override;

  static RLZCompress from(const std::filesystem::path& ref_seq_path,  const std::filesystem::path& ref_sa_path);
  static std::unique_ptr<RLZCompress> from_dynamic_fasta(const std::filesystem::path& ref_seq_path, const std::filesystem::path& ref_sa_path);
  static std::unique_ptr<RLZCompress> from_dynamic(const std::filesystem::path& ref_seq_path,  const std::filesystem::path& ref_sa_path);
  RLZCompress(DynArray<data_type>&& ref_seq, DynArray<unsigned int>&& ref_sa);
  std::size_t compress(const char *file_to_parse, FactorWriter& writer);
  std::size_t compress_fasta(const char *file_to_parse, FactorWriter& writer) const;
  filelength_type ref_len() const;

  DynArray<unsigned int> _ref_sa;
  const std::string& get_id();
  void set_id(const std::string& new_id);

  void decompress(const std::filesystem::path& target_unizpped);
private:
  void computeLZFactorAt(data_type* seq, filelength_type i, filelength_type &pos, filelength_type &len) const;
  void constructBST(const DynArray<unsigned int>& lcp, filelength_type i, filelength_type left, filelength_type right);
  DynArray<std::pair<unsigned int, unsigned int>> _bst_lcp;
  std::string id {};
};
