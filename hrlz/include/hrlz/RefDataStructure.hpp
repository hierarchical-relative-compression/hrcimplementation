//
// Created by mpelo on 1/1/26.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_REFDATASTRUCTURE_HPP
#define HIERARCHICALRELATIVELEMPELZIV_REFDATASTRUCTURE_HPP

#include <filesystem>
#include <fstream>

class RefDataStructure {
public:
    typedef std::ios_base::openmode streammode;
    streammode in_ref_mode { std::ios_base::binary | std::ios_base::in };
    streammode in_tar_mode { std::ios_base::binary | std::ios_base::in };
    streammode out_tar_mode { std::ios_base::binary | std::ios_base::out };
    streammode in_decomp_tar_mode { std::ios_base::binary | std::ios_base::in };
    streammode out_decomp_tar_mode { std::ios_base::binary | std::ios_base::out };

    RefDataStructure(const std::filesystem::path& path) {(void)path;}
    RefDataStructure(std::istream& in) {(void)in;}
    virtual ~RefDataStructure() = default;

    virtual void parseRef(std::istream& in) = 0;
    virtual void compress_target(const std::filesystem::path& in, std::ostream& out) const = 0;
    virtual void decompress_target(std::istream& in, std::ostream& out) const = 0;
};
#endif //HIERARCHICALRELATIVELEMPELZIV_REFDATASTRUCTURE_HPP
