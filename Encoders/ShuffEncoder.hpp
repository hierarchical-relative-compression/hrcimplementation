//
// Created by maximo on 10-01-26.
//

#ifndef SHUFFENCODER_HPP
#define SHUFFENCODER_HPP

#include "Encoder.hpp"

class ShuffEncoder : public Encoder {
public:
    ShuffEncoder();
    virtual ~ShuffEncoder() = default;
    /**
     * Decode an encoded file at the path specified by in_path, and store it at the provided file.
     * The path MUST NOT HAVE AN ENCODING EXTENSION.
     * @param in_path path to the encoded target, WITHOUT AN ENCODING EXTENSION.
     * @param out_path path where the decoded target will be stored
     */
    virtual void decode_executable(const std::filesystem::path& in_path, const std::filesystem::path& out_path) const;

    virtual void encode_executable(const std::filesystem::path& in_path, const std::filesystem::path& out_path) const;

    void encode_C_stream(FILE* in_stream, const std::filesystem::path& out_path) const;
    void decode_C_stream(const std::filesystem::path& in_path, FILE* out_stream) const;

    void encode_C_stream(FILE* in_stream, FILE* out_stream) const override;
    void decode_C_stream(FILE* in_path, FILE* out_stream) const override;

    /**
     *
     */
};

#endif //SHUFFENCODER_HPP
