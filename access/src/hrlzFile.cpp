#include<access/hrlzFile.hpp>
#include<algorithm>

HRLZFile construct_hrlzfile(std::istream& stream) {
  std::size_t file_count, root;
  stream.read((char*) &file_count, sizeof(file_count));
  stream.read((char*) &root, sizeof(root));

  DynArray<std::filesystem::path> files(file_count);
  DynArray<WeightedEdge> edges(file_count-1);
  DynArray<FileHeader> headers(file_count);
  DynArray<CompressedFile> compressed_files(file_count);
  std::vector<char> path;

  for(std::size_t i = 0; i < files.size(); ++i) {
    char c;
    while(true) {
      stream.get(c);
      path.push_back(c);
      if(c == 0)
        break;
    }
    files[i] = std::filesystem::path(path.begin());
    path.clear();
  }

  std::size_t root_file_size;
  stream.read((char*) &root_file_size, sizeof(root_file_size));

  DynArray<char> root_content(root_file_size);
  std::copy_n(std::istreambuf_iterator<char>(stream), root_file_size, root_content.begin());

  for(std::size_t i = 0; i < file_count - 1; ++i) {
    auto& edge = edges[i];
    stream.read((char*) &edge.from, sizeof(edge.from));
    stream.read((char*) &edge.to, sizeof(edge.to));
    stream.read((char*) &edge.weight, sizeof(edge.weight));
    headers[edge.to] = { 0, edge.from, edge.weight };
  }

  for(std::size_t i = 0; i < file_count - 1; ++i) {
    const auto& edge = edges[i];
    DynArray<Factor> content(edge.weight);
    for(std::size_t f = 0; f < edge.weight; ++f) {
      stream.read((char*) &content[f].pos, sizeof(content[f].pos));
      stream.read((char*) &content[f].len, sizeof(content[f].len));
      content[f].cumulative_len = content[f].len;
      if(f > 0) {
        content[f].cumulative_len += content[f-1].cumulative_len;
      }
    }
    compressed_files[edge.to] = CompressedFile(std::move(content));
  }
  return {
    root,
    std::move(root_content),
    std::move(headers),
    std::move(files),
    std::move(compressed_files)
  };
}

char HRLZFile::find(const std::size_t file, const std::size_t pos) const noexcept {
  return find(file, file_header(file), pos);
}

const std::filesystem::path& HRLZFile::file_path(const std::size_t file) const noexcept {
  return _files[file];
}

const FileHeader& HRLZFile::file_header(const std::size_t file) const noexcept {
  return _headers[file];
}

char HRLZFile::find(const std::size_t file, FileHeader header, std::size_t pos) const noexcept {
  if(file == _root) {
    return _root_content[pos];
  }
  auto ref_factor = _compressed_files[file].find(pos);
  if(ref_factor.len == 0) {
    return (char) pos;
  }
  return find(header.parent, file_header(header.parent), ref_factor.len + pos - ref_factor.cumulative_len + ref_factor.pos);
}

const Factor& CompressedFile::find(std::size_t pos) const noexcept {
  auto comp = [&](const auto& it) {
    return pos >= it.cumulative_len;
  };
  auto it = std::partition_point(_file.begin(), _file.end(), comp);
  return *it;
}
