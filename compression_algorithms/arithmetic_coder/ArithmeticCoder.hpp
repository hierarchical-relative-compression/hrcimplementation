#pragma once
#include <string>
#include <filesystem>

#include "Statistics.hpp"

class ArithmeticCoder
{
public:
    /**
     * Compress a string to a file. May move the contents of the original string.
     * @param str The string to compress
     * @param out the file to compress
     * @return The statistics of the compression
     */
	virtual Statistics encode(std::string& str, const std::filesystem::path& out, bool start, bool end) = 0;
    /**
     * Compress the contents of the stringstream to a file. May modify the stream.
     * @param ss the stringstream
     * @param n the length of the stream
     * @param out the output file
     * @return the statistics of the compression
     */
    virtual Statistics encode(std::istream& ss, size_t n, const std::filesystem::path& out, bool start, bool end) = 0;
	virtual void decode(const std::filesystem::path& path_in, std::stringstream& out, bool start, bool end) = 0;
};
