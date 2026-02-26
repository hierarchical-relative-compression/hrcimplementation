//
// Created by mpelo on 7/10/24.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_IDOCOMPINTERFACE_H
#define HIERARCHICALRELATIVELEMPELZIV_IDOCOMPINTERFACE_H

#include <filesystem>

std::size_t iDoComp_compress(const std::filesystem::path& ref, const std::filesystem::path& ref_sa,
                     const std::filesystem::path& target, const std::filesystem::path& output_path);
int iDoComp_decompress(const std::filesystem::path& ref, const std::string& target_filename,
                       const std::filesystem::path& compressed_target, const std::filesystem::path& output_dir);

#endif //HIERARCHICALRELATIVELEMPELZIV_IDOCOMPINTERFACE_H
