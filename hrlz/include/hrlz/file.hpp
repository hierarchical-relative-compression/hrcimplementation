#pragma once 
#include<memory>
#include<filesystem>
#include<vector>
#include<ostream>
#include<istream>

#include "common/DynArray.hpp"
#include "hrlz/Arborescence.hpp"

namespace filesystem {
  std::size_t file_size(const std::filesystem::path& path);
	extern std::filesystem::path output_dir;

	void init_output_folder(const std::filesystem::path& output_folder);
  std::string get_output_filename_gdc2(const std::string& filename_ref, const std::string& filename_target);
  std::string get_filename_gdc2(const std::string& filename_ref, const std::string& filename_target);
}

struct FileMetadata {
  std::size_t sequence_count, root_index, root_size, max_sequence_size, filepath_array_start;
};

struct SequenceMetadata {
  std::size_t parent, index_in_file, factors, factors_pos, filepath_pos;
};

struct ExternalMetadata {
  std::size_t n_sequences;
  std::size_t root_id;
  std::string original_directory;
  std::unique_ptr<std::string[]> filenames;
  std::unique_ptr<std::vector<std::size_t>[]> arbor;
};

struct FullTreeMetadata {
    size_t n_sequences;
    size_t root_id;
    std::string original_directory;
    std::unique_ptr<std::string[]> filenames;
    std::unique_ptr<size_t[]> parents;                  //The only difference from the original metadata
    std::unique_ptr<std::vector<std::size_t>[]> arbor;
};

DynArray<char> read_file(const std::filesystem::path& path, std::size_t right_padding);
DynArray<char> read_file(const std::filesystem::path& path);
DynArray<char> read_compacted_DNA(const std::filesystem::path& path);
DynArray<char> read_DNA_from_FASTA(const std::filesystem::path& path);

DynArray<std::filesystem::path> read_input_file(const std::filesystem::path& path);

DynArray<SequenceMetadata> compress_to_stream(std::ostream& stream, const DynArray<std::filesystem::path>& files, const std::vector<Edge>& edges, std::size_t root, const DynArray<std::filesystem::path>& sa);

void decompress_from_stream(std::istream& stream);

void decompress_sequence_from_stream(std::istream& stream, const std::size_t);

void print_arbor(std::ostream& ostream, size_t n, const std::unique_ptr<std::vector<size_t>[]>& arbor);
void write_external_metadata(const DynArray<std::filesystem::path>& files, const std::unique_ptr<std::vector<std::size_t>[]>& arbor,
                             const std::filesystem::path& comp_dir, std::size_t root_id, const std::string& metadata_name="metadata");

//void write_chromosome_metadata(const DynArray<std::filesystem::path>& files, const std::unique_ptr<std::vector<size_t>[]>& arbor,
//                               const std::filesystem::path& comp_dir, size_t root_id, size_t n_chromosomes);

std::unique_ptr<ExternalMetadata> read_external_metadata(const std::filesystem::path& output_dir, const std::string& metadata_name="metadata");
std::unique_ptr<FullTreeMetadata> read_metadata_full_tree(const std::filesystem::path& input_dir, const std::string& metadata_name="metadata");
//std::unique_ptr<ChromosomeMetadata> read_chromosome_metadata(const std::filesystem::path& output_dir);