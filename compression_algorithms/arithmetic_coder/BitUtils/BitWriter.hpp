#include <fstream>
#include <string>
#include <vector>
#include "BitBuffer.hpp"
#include "BitStat.hpp"

#ifndef HIERARCHICALRELATIVELEMPELZIV_BITWRITER_HPP
#define HIERARCHICALRELATIVELEMPELZIV_BITWRITER_HPP

class BitWriter
{
public:
	BitWriter(std::string path);
    BitWriter();
    void open(const std::string& path);

	void write(bool bit);
	void write(int bit);
	BitWriter& operator<< (bool bit);
	BitWriter& operator<< (int bit);

	void writeN(bool bit, int n);
	void writeN(int bit, int n);

	void flush();

	void beginByte(int byte);
	std::vector<BitStat> getStats();

	virtual ~BitWriter();

    void storeBitWriter(std::ostream& file);
    void loadBitWriter(std::istream& file);

private:
	const int BIT_BUFFER_CAPACITY = 8;
    std::string path;
	std::ofstream m_fileStream;
	BitBuffer m_bitBuffer {};
	int m_currentByte = -1;
	std::vector<BitStat> m_stats;
};

#endif