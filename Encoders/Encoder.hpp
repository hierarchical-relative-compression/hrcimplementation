//
// Created by mpelo on 1/1/26.
//

#ifndef HIERARCHICALRELATIVELEMPELZIV_ENCODER_HPP
#define HIERARCHICALRELATIVELEMPELZIV_ENCODER_HPP

#include <string>
#include <filesystem>

class Encoder {
public:
    enum class STREAM_VERSION { CPP, C };
    const std::string extension;
    STREAM_VERSION version;

    Encoder(const std::string& ext, STREAM_VERSION version);
    virtual ~Encoder() = default;
    /**
     * Decode an encoded file at the path specified by in_path, and store it at the provided file.
     * The path MUST NOT HAVE AN ENCODING EXTENSION.
     * @param in_path path to the encoded target, WITHOUT AN ENCODING EXTENSION.
     * @param out_path path where the decoded target will be stored
     */
    virtual void decode_executable(const std::filesystem::path& in_path, const std::filesystem::path& out_path) const = 0;

    virtual void encode_executable(const std::filesystem::path& in_path, const std::filesystem::path& out_path) const = 0;

    virtual void encode_C_stream(FILE* in_stream, FILE* out_stream) const = 0;

    virtual void decode_C_stream(FILE* in_path, FILE* out_stream) const = 0;
};

#endif //HIERARCHICALRELATIVELEMPELZIV_ENCODER_HPP
