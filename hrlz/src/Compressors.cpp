// Author: Máximo Pérez López

// This file will be the interface to the selected compressor.
#include "hrlz/Compressors.hpp"
#include "hrlz/CompressorIDoCompMax.hpp"
#include "hrlz/file.hpp"
#include "hrlz/CompressorHiRGCMax.hpp"
#include "hrlz/CompressorRLZ.hpp"
#include "Encoder.hpp"
#include <cstddef>
#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <queue>
#include "HuffmanFile.hpp"
#include <omp.h>
#include <ShuffEncoder.hpp>

#include "common/SteadyTimer.hpp"

namespace Compressors
{
    //Define the global variables of Compressors.hpp
    Compressor_Types compressor_type = RLZ;
    Encoding_Types encoding_type = HUFF;

    const char *gdc2_exec_path = "../compression_algorithms/gdc2/Gdc2/gdc2";

    Compressor::Compressor(Encoder *enc) : encoder(enc) {}
    Compressor::~Compressor() {}

    std::string Compressor::get_name(const std::string& ref_filename, const std::string& tar_filename) {
        return ref_filename + "_" + tar_filename;
    }

    std::filesystem::path Compressor::get_output_file(const std::string &ref_filename,
                                                      const std::string &target_filename,
                                                      const std::filesystem::path &output_dir) {
        return output_dir / get_name(ref_filename, target_filename);
    }

    void Compressor::decompress_pair(const std::filesystem::path& ref, const std::filesystem::path& target, const std::filesystem::path& output) {
        (void)ref;
        (void)target;
        (void)output;
        throw std::runtime_error("Decompress pair not implemented for " + compressor_type);
    }

    size_t Compressor::compress_dataset(const DynArray<std::filesystem::path> &files,
                                      const std::unique_ptr<std::vector<std::size_t>[]> &arbor, const size_t root_id,
                                      const std::filesystem::path& output_dir) {
        size_t weight = 0;
        std::queue<size_t> queue;
        std::vector<size_t> *adj = arbor.get();
        queue.push(root_id);
        while (!queue.empty()) {
            size_t ref = queue.front();
            queue.pop();
            const std::vector<size_t>& targets = adj[ref];
            size_t n_targets = targets.size();
            if (n_targets == 0)
                continue;
            const std::filesystem::path **target_paths = new const std::filesystem::path*[n_targets];
            for (size_t i = 0; i < n_targets; i++) {
                //We need to make a deep copy here, so that we don't delete the paths, and we can reuse them again
                //(either in parallel or in sequential calls)
                target_paths[i] = &files[targets[i]];
                queue.push(targets[i]);
            }
            weight += compress_one_to_many(files[ref], target_paths, n_targets, output_dir);
            delete[] target_paths;
        }
        //Copy root
        if (std::filesystem::exists(output_dir / files[root_id].filename())) {
            std::cerr << "Warning: root already exists at " << output_dir << std::endl;
        } else {
            std::filesystem::copy_file(files[root_id], output_dir / files[root_id].filename());
        }
        //Now, all the files are in place. We just need to write the metadata
        write_external_metadata(files, arbor, output_dir, root_id);
        return weight;
    }

    /**
     * A stringstream that exposes its get and put areas for reading. Adapted from StackOverflow
     */
    class BufferStringStream : public std::stringstream, public std::basic_stringbuf<char> {
    public:
        BufferStringStream() {
            static_cast<std::basic_ios<char>&>(*this).rdbuf(this);
        }
        char *c_str_get() {
            *pptr() = '\0';
            return eback();
        }
        char *c_str_put() {
            *pptr() = '\0';
            return pbase();
        }
    };

    size_t Compressor::compress_one_to_many(const std::filesystem::path &ref, const std::filesystem::path **targets,
                                            size_t n_targets, const std::filesystem::path &output_dir) {
        std::unique_ptr<RefDataStructure> refds = getRefDataStructure(ref, false);
        std::filesystem::path temp = targets[0]->parent_path();
        std::string ref_filename = ref.filename().string();
        size_t weight = 0;
        for (size_t i = 0; i < n_targets; i++) {
            //First, compress
            std::filesystem::path compressed = get_output_file(ref_filename, targets[i]->filename().string(),
                                                               std::filesystem::canonical(output_dir));
            std::ofstream out_tar(compressed, refds->out_tar_mode);
            if (!out_tar)
                throw std::runtime_error("Can't open " + compressed.string() + " to compress");
            refds->compress_target(*(targets[i]), out_tar);
            out_tar.close();
            //decode
            encoder->encode_executable(compressed, compressed); //Will add the extension
            std::filesystem::remove(compressed);
            //get size
            std::filesystem::path output_ext = compressed.parent_path() / (compressed.filename().string() + encoder->extension);
            if (!std::filesystem::exists(output_ext))
                throw std::runtime_error("Can't find the output " + output_ext.string());
            weight += filesystem::file_size(output_ext);

        }
        return weight;
    }

    std::vector<size_t> Compressor::compress_one_to_many_delete(const std::filesystem::path& ref,
                                                                const std::filesystem::path **targets, size_t n_targets) {
        std::filesystem::path out_dir = std::filesystem::temp_directory_path();
        compress_one_to_many(ref, targets, n_targets, out_dir);
        std::string ref_filename = ref.filename().string();
        std::vector<size_t> weights(n_targets);
        for (size_t i = 0; i < n_targets; i++) {
            std::filesystem::path output = out_dir / (get_name(ref_filename, targets[i]->filename().string()) + encoder->extension);
            weights[i] = std::filesystem::file_size(output);
            std::filesystem::remove(output);
        }
        return weights;
    }

    void Compressor::decompress_dataset(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir) {
        std::unique_ptr<ExternalMetadata> met = read_external_metadata(input_dir);
        const size_t root_id = met->root_id;
        const std::string *filenames = met->filenames.get();
        const std::string& root_filename = filenames[root_id];
        const std::vector<size_t> *adj = met->arbor.get();
        //Copy root (we need it in the compressed dataset)
        if (!std::filesystem::exists(output_dir / root_filename))
            std::filesystem::copy(input_dir / root_filename, output_dir / root_filename);
        std::queue<size_t> queue; //BFS
        queue.push(root_id);
        while (!queue.empty()) {
            size_t ref = queue.front();
            queue.pop();
            const std::vector<size_t>& targets = adj[ref];
            size_t n_targets = targets.size();
            if (n_targets == 0)
                continue;
            const std::string& ref_uncompressed_name = filenames[ref];
            //One-to-many compression
            std::filesystem::path *arr_paths = new std::filesystem::path[n_targets];
            const std::string **arr_names = new const std::string *[n_targets];
            for (size_t i = 0; i < n_targets; i++) {
                queue.push(targets[i]);
                arr_paths[i] = get_output_file(filenames[ref], filenames[targets[i]], input_dir); //The targets are in input_dir
                arr_names[i] = &filenames[targets[i]];
            }
            decompress_one_to_many(output_dir / ref_uncompressed_name, arr_paths, arr_names, n_targets, output_dir);
            delete[] arr_paths;
            delete[] arr_names;
        }
    }

    void Compressor::decompress_sequence(const std::filesystem::path& input_dir, size_t seq_ix,
                                         const std::filesystem::path& output_dir) {
        std::unique_ptr<FullTreeMetadata> met = read_metadata_full_tree(input_dir);
        size_t root_id = met->root_id;
        const std::string& root_filename = met->filenames[root_id];
        if (seq_ix == root_id) {
            if (!std::filesystem::exists(input_dir / root_filename))
                throw std::runtime_error("Couldn't find root of the dataset at " + (input_dir / root_filename).string() + "!!!");
            std::filesystem::copy_file(input_dir / root_filename, output_dir / root_filename);
            return;
        }
        const size_t *parents = met->parents.get();
        const std::string *filenames = met->filenames.get();
        std::vector<size_t> path;
        path.push_back(seq_ix);
        size_t ref_id = parents[seq_ix];
        while (ref_id != root_id) {
            path.push_back(ref_id);
            ref_id = parents[ref_id];
        }
        size_t i = path.size() - 1;
        size_t target_id = path[i];
        //First, decompress when the root is the reference
        std::stringstream output_decompression;
        std::unique_ptr<RefDataStructure> refds;
        ProfilerSteady::profile_thread("load_ref_" + std::to_string(omp_get_thread_num()), [&] {
            refds = getRefDataStructure(input_dir / root_filename, true);
        }, false);
        decompress_one(*refds, get_output_file(root_filename, filenames[target_id], input_dir), output_decompression,
                       output_dir);
        //Decompress the rest. Note that, at the start, the reference is in the stringstream "output_decompression",
        //and that's why we perform a swap
        std::stringstream input_decompression;
        while (i > 0) {
            i--;
            input_decompression.str(std::string());
            input_decompression.clear();
            std::swap(output_decompression, input_decompression);
            ProfilerSteady::profile_thread("load_ref_" + std::to_string(omp_get_thread_num()), [&] {
                refds = getRefDataStructure(input_decompression, true); //Destroys the previous data structure
            }, false);
            decompress_one(*refds, get_output_file(filenames[path[i+1]], filenames[path[i]], input_dir),
                           output_decompression, output_dir);
        }
        //The output is in output_decompression. Serialize in binary to keep the exact same stream
        std::ofstream output_file(output_dir / filenames[seq_ix], std::ios_base::out | std::ios_base::binary);
        output_file << output_decompression.rdbuf();
    }


    void Compressor::decompress_one_to_many(const std::filesystem::path& in_ref, const std::filesystem::path *targets,
                                            const std::string **tar_original_names, size_t n_targets,
                                            const std::filesystem::path& out_dir) {
        std::unique_ptr<RefDataStructure> refds = getRefDataStructure(in_ref, true);
        std::filesystem::path temp = targets[0].parent_path();
        for (size_t i = 0; i < n_targets; i++) {
            std::ofstream output_target(out_dir / *(tar_original_names[i]), refds->out_decomp_tar_mode);
            decompress_one(*refds, targets[i], output_target, targets[i].parent_path());
        }
    }

    /**
     * From StackOverflow: a streambuf that's initialized with a raw array, without copying it
     * https://stackoverflow.com/questions/7781898/get-an-istream-from-a-char
     */
    struct membuf : std::streambuf
    {
        char *beg_ptr;
        char *end_ptr;
        membuf(char* begin, char* end) {
            beg_ptr = begin;
            end_ptr = end;
            this->setg(begin, begin, end);
        }

        pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override {
            if (which == std::ios_base::out) {
                std::cerr << "No writing pointer in input membuf!!!";
                return 0UL;
            }
            switch (dir) {
                case std::ios_base::beg:
                    this->setg(beg_ptr, beg_ptr + off, end_ptr);
                    break;
                case std::ios_base::cur:
                    this->gbump(off);
                    break;
                case std::ios_base::end:
                    this->setg(beg_ptr, end_ptr + off, end_ptr);
                    break;
            }
            return gptr() - beg_ptr;
        }

        pos_type seekpos(pos_type off, std::ios_base::openmode which) override {
            if (which == std::ios_base::out) {
                std::cerr << "No writing pointer in input membuf!!!";
                return 0UL;
            }
            this->setg(beg_ptr, beg_ptr + off, end_ptr);
            return off;
        }
    };

    void Compressor::decompress_one(const RefDataStructure& ref, const std::filesystem::path& target, std::ostream& out,
                                    const std::filesystem::path& temp_decode_dir) {
        //First, decode
        std::filesystem::path decoded;
        ProfilerSteady::profile_thread("decode_target_" + std::to_string(omp_get_thread_num()), [&] {
            decoded = temp_decode_dir / (target.filename().string() + "_decoded");
            encoder->decode_executable(target, decoded);
        }, false);
        //Then, decompress to stream
        ProfilerSteady::profile_thread("decompress_target_" + std::to_string(omp_get_thread_num()), [&] {
            std::ifstream input_decoded(decoded, ref.in_decomp_tar_mode);
            ref.decompress_target(input_decoded, out);
        }, false);
        //Remove the only-decoded file
        std::filesystem::remove(decoded);
    }

    std::unique_ptr<Compressor> getCompressor() {
        switch (compressor_type) {
            case RLZ: {
                return std::make_unique<CompressorRLZ>();
            }
            case HIRGCMAX: {
                return std::make_unique<CompressorHiRGCMax>();
            }
            case IDOCOMPMAX: {
                return std::make_unique<CompressorIDoCompMax>();
            }
            default: {
                std::cerr << "Compressor not implemented" << std::endl;
                std::exit(1);
            }
        }
    }

    void cleanup() {}

    /**
     * UTILS
    */

    /**
     * Make a C-style string, non-const copy of the n strings given.
     * The pointers are to the c-style strings are returned in the output parameter pointers.
     * The memory allocated is all contiguous in the unique pointer returned.
     * char *(&pointers)[]
    */
    std::unique_ptr<char[]> make_nonconst_copy(std::string strings[], char **(&pointers), int n) {
        //Get the combined size
        int size = 0;
        for (int i = 0; i < n; i++) {
            size += strings[i].size() + 1; //+1 for the last null character
        }
        std::unique_ptr<char []> memory = std::make_unique<char[]>(size);
        int offset = 0;
        for (int i = 0; i < n; i++) {
            pointers[i] = &memory[offset];  //Output pointer points now to the correct slice of the memory
            strings[i].copy(pointers[i], strings[i].size()); //Copy the string to this slice of memory
            offset += strings[i].size();    //The character after the last character of the string
            memory[offset] = '\0';          //We write a zero
            offset++;                       //The next pointer is on the character after the zero
        }
        return memory;
    }

    std::unique_ptr<char []> make_nonconst_copy(std::string strings[], char *(&pointers)[], int n) {
        //Get the combined size
        int size = 0;
        for (int i = 0; i < n; i++) {
            size += strings[i].size() + 1; //+1 for the last null character
        }
        std::unique_ptr<char []> memory = std::make_unique<char[]>(size);
        int offset = 0;
        for (int i = 0; i < n; i++) {
            pointers[i] = &memory[offset];  //Output pointer points now to the correct slice of the memory
            strings[i].copy(pointers[i], strings[i].size()); //Copy the string to this slice of memory
            offset += strings[i].size();    //The character after the last character of the string
            memory[offset] = '\0';          //We write a zero
            offset++;                       //The next pointer is on the character after the zero
        }
        return memory;
    }

	/**
	 * Print in one line the arguments of a program
	*/
    void print_args(int argc, char *argv[]) {
		std::cout << argv[0];
		for (int i = 1; i < argc; i++) {
			std::cout << " " << argv[i];
		}
	}
}
