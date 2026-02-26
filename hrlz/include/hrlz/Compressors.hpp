/*
 * Compressors.hpp
 *
 *  Created on: Feb 8, 2024
 *      Author: mpelo
 */

#ifndef SOURCE_DIRECTORY__HRLZ_INCLUDE_HRLZ_COMPRESSORS_HPP_
#define SOURCE_DIRECTORY__HRLZ_INCLUDE_HRLZ_COMPRESSORS_HPP_

#pragma once
#include <memory>
#include "rlz/rlz.hpp"
#include <vector>
#include "hrlz/Arborescence.hpp"
#include "RefDataStructure.hpp"
#include "Encoder.hpp"

namespace Compressors {
    static const bool verbose = false;

    static const bool output_raw = false;

	enum Compressor_Types {
		RLZ, GDC2, HIRGCMAX, SCCG, IDOCOMPMAX
	};

    enum Encoding_Types {
        ARITH, PPM, HUFF
    };

    extern Encoding_Types encoding_type;
	extern Compressor_Types compressor_type;

	class Compressor {
    protected:
		Encoder *encoder;
    public:
		Compressor(Encoder *enc);
        virtual ~Compressor();
        std::filesystem::path output_dir;

		virtual size_t compress_pair_delete(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse, const std::filesystem::path& ref_sa = "") {
            (void)ref;
            (void)file_to_parse;
            (void)ref_sa;
            return 0;};
		virtual size_t compress_pair_keep(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse, const std::filesystem::path& ref_sa = "") {(void)ref;
            (void)file_to_parse;
            (void)ref_sa;
            return 0;};


        /**
         * Returns the name of the compressed filename corresponding to the given reference and target filename.
         * @param ref_filename Filename of the reference
         * @param target_filename Filename of the target
         * @return filename of the relative compression of the target w.r.t. the reference
         */
        virtual std::string get_name(const std::string& ref_filename, const std::string& target_filename);
        virtual std::filesystem::path get_output_file(const std::string& ref_filename, const std::string& target_filename,
                                                      const std::filesystem::path& output_dir);

        /**
         * Compresses a full hierarchy to disk.
         * @param files an array of all the file paths to the original files.
         * @param arbor the arboricity that represents the hierarchy.
         * @param root_id the index of the root of the hierarchy in @files
         * @return the size of the compressed targets, WITHOUT THE ROOT.
         */
        virtual size_t compress_dataset(const DynArray<std::filesystem::path>& files,
                                        const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const size_t root_id,
                                        const std::filesystem::path& output_dir);

        /**
         * Compresses several targets given a reference file. Note that this function also applies encoding.
         * @param ref path to the reference file
         * @param targets raw array of paths to the targets
         * @param n_targets number of targets to compress
         * @param output_dir directory where the compressed targets will be stored.
         * @return the size in bytes of the compressed targets.
         */
        virtual size_t compress_one_to_many(const std::filesystem::path& ref, const std::filesystem::path **targets,
                                            size_t n_targets, const std::filesystem::path& output_dir);
        /**
         * Returns the individual sizes of the relative compression of the targets w.r.t the reference.
         * Note that this function also applies encoding.
         * @param ref path to the reference file
         * @param targets raw array of paths to the targets
         * @param n_targets number of targets to compress
         * @return a vector containing the size in bytes of each of the compressed targets.
         */
        virtual std::vector<size_t> compress_one_to_many_delete(const std::filesystem::path& ref, const std::filesystem::path **targets, size_t n_targets);

        /**
         * DECOMPRESSION METHODS
         */

        /**
         * Decompresses a target that was compressed with ref. The filename of the target is specified as it was
         * *BEFORE compression*
         * @param ref path to the reference
         * @param target path to the target, with the filename being the original target filename
         * @param output path to the output file, including the desired output filename
         */
        virtual void decompress_pair(const std::filesystem::path& ref, const std::filesystem::path& target, const std::filesystem::path& output);

        /**
         * Decompresses a hierarchical-compressed dataset stored at input_dir, following the metadata present at
         * the same directory, and stores the output in output_dir.
         * @param input_dir directory of the compressed dataset
         * @param output_dir directory to store the output decompressed dataset
         */
        virtual void decompress_dataset(const std::filesystem::path& input_dir, const std::filesystem::path& output_dir);
        /**
         * Loads a reference file, decompresses the given targets stored on the disk, and
         * stores the decompressed targets in the specified folder. Note that this function includes
         * the decoding step.
         * @param in_ref input path to the reference
         * @param targets array of pointers to the paths to the targets WITHOUT ENCODING EXTENSIONS
         * @param tar_original_names array of pointers to the filenames of the decompressed targets
         * @param n_targets number of targets in the "targets" array
         * @param out_dir directory to store the decompressed targets.
         */
        void decompress_one_to_many(const std::filesystem::path& in_ref, const std::filesystem::path *targets,
                                    const std::string **tar_original_names, size_t n_targets,
                                    const std::filesystem::path& out_dir);
        /**
         * Decompresses an encoded target and outputs it in the specified directory.
         * @param input_dir Directory of the encoded target
         * @param seq_ix Index of the sequence to decompress in the dataset
         * @param output_dir Directory where the output will be stored
         */
        void decompress_sequence(const std::filesystem::path& input_dir, size_t seq_ix, const std::filesystem::path& output_dir);
        /**
         * Decdes and decompresses one target stored in disk, given a RefDataStructure that contains the reference.
         * Note that this step also includes decoding.
         * @param ref RefDataStructure containing the reference to decompress with.
         * @param target path to the target to decompress in disk WITHOUT ENCODING EXTENSIONS
         * @param out output stream containing the decoded and decompressed target.
         * @param temp_decode temporary directory for the decoder
         */
        virtual void decompress_one(const RefDataStructure& ref, const std::filesystem::path& target, std::ostream& out,
                            const std::filesystem::path& temp_decode_dir);

        virtual std::unique_ptr<RefDataStructure> getRefDataStructure(const std::filesystem::path& ref, bool decomp) {
            (void)ref; (void)decomp;
            throw std::runtime_error("getRefDataStructure Not Implemented for this compressor");
        }
        virtual std::unique_ptr<RefDataStructure> getRefDataStructure(std::istream& ref, bool decomp) {
            (void)ref; (void)decomp;
            throw std::runtime_error("getRefDataStructure not implemented for this compressor");
        }

        bool print_stats = false;
	};

	/**
	 * Get an instance of the selected compressor
	*/
	std::unique_ptr<Compressor> getCompressor();
    /**
     * Clean up any global resources used by the compressors
     */
    void cleanup();

	/**
	 * UTILS
	*/

	/**
     * Make a C-style string, non-const copy of the n strings given.
     * The pointers are to the c-style strings are returned in the output parameter pointers.
     * The memory allocated is all contiguous in the unique pointer returned.
    */
    std::unique_ptr<char[]> make_nonconst_copy(std::string strings[], char **(&pointers), int n);

	std::unique_ptr<char[]> make_nonconst_copy(std::string strings[], char *(&pointers)[], int n);

	/**
	 * Print in one line the arguments of a program
	*/
	void print_args(int argc, char *argv[]);
}

#endif /* SOURCE_DIRECTORY__HRLZ_INCLUDE_HRLZ_COMPRESSORS_HPP_ */
