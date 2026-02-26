#ifndef SOURCE_DIRECTORY__HRLZ_INCLUDE_HRLZ_COMPRESSORGDC2_HPP_
#define SOURCE_DIRECTORY__HRLZ_INCLUDE_HRLZ_COMPRESSORGDC2_HPP_

#include <HuffmanFile.hpp>

#include "hrlz/Compressors.hpp"

namespace Compressors {

    class CompressorGDC2 : public Compressor {
		public:
		CompressorGDC2() : Compressor(new HuffmanEncoder()) {}
    	~CompressorGDC2() {
			delete encoder;
		}

		/**
		 * Methods inherited from Compressor
		*/
		size_t compress_pair_delete(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse,
                                    const std::filesystem::path& ref_sa) override;
		size_t compress_pair_keep(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse,
                                  const std::filesystem::path& ref_sa) override;
		size_t compress_dataset(const DynArray<std::filesystem::path>& files,
                                const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const std::size_t root_id,
                                const DynArray<std::filesystem::path>& sa_files);
		void decompress_dataset(const std::filesystem::path& input_directory);

		/**
		 * Other methods used in the implementation
		 * (they need not be in the header, but it makes things easier)
		*/
	
        void gdc2_delete_file(const std::string& path);

        void compress_sequence_and_keep(const std::filesystem::path& ref, std::string **targets, std::size_t n);
		
		//Functions to compress a hierarchy, but by calling gdc2 on every internal node of the hierarchy.
        void compress_hrlz_gdc2_only_children(const DynArray<std::filesystem::path>& file_paths,
            const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const int root_id);
        void decompress_hrlz_gdc2_only_children(const std::filesystem::path& input_directory);
        void decompress_sequence(const std::string& ref_name, const std::filesystem::path& input_directory);

		//Functions to compress all the hierarchy with a single gdc2 call, but with the input files in the order
		//predicted by hrlz
        void compress_hrlz_gdc2_full(const DynArray<std::filesystem::path>& file_paths,
			const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const std::size_t root_id);
        void decompress_hrlz_gdc2_full(const std::filesystem::path& input_directory);

        //Removing SA from the call
        size_t compress_pair_delete(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse);
        size_t compress_pair_keep(const std::filesystem::path& ref, const std::filesystem::path& file_to_parse);
        size_t compress_hrlz(const DynArray<std::filesystem::path>& files,
                             const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const std::size_t root_id);
	};
}

#endif /* SOURCE_DIRECTORY__HRLZ_INCLUDE_HRLZ_COMPRESSORGDC2_HPP_ */