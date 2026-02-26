#pragma once
#include<filesystem>
#include<common/DynArray.hpp>
#include<hrlz/Arborescence.hpp>
#include<istream>
#include<rlz/common.h>

struct Factor {
  filelength_type pos;
  filelength_type len;
  filelength_type cumulative_len;
};

class CompressedFile {
  DynArray<Factor> _file;
public:
  CompressedFile() = default;
  CompressedFile(DynArray<Factor>&& content): _file(std::move(content)) {}
  const Factor& find(std::size_t pos) const noexcept;
};

struct FileHeader {
  std::size_t file_offset;
  std::size_t parent;
  std::size_t factors;
};

class HRLZFile {
public:
  HRLZFile(std::size_t root, DynArray<char>&& root_content, DynArray<FileHeader>&& headers, DynArray<std::filesystem::path>&& files, DynArray<CompressedFile>&& compressed_files):
    _root(root), _root_content(std::move(root_content)), _headers(std::move(headers)), _files(std::move(files)), _compressed_files(std::move(compressed_files)) {}
  char find(const std::size_t file, const std::size_t pos) const noexcept;
  const std::filesystem::path& file_path(const std::size_t file) const noexcept;
private:
  std::size_t _root;
  DynArray<char> _root_content;
  DynArray<FileHeader> _headers;
  DynArray<std::filesystem::path> _files;
  DynArray<CompressedFile> _compressed_files;

  const FileHeader& file_header(const std::size_t file) const noexcept;
  char find(const std::size_t file, FileHeader header, std::size_t pos) const noexcept;
};


HRLZFile construct_hrlzfile(std::istream& stream);
