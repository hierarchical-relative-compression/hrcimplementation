#pragma once

#include <vector>
#include <cstddef>
#include <fstream>

using byte_t = unsigned char;

class BitBuffer
{
public:
	int capacity;
	BitBuffer(int capacity);
    BitBuffer();

	size_t size();

	void write(bool bit);
	void write(int bit);
	BitBuffer& operator<< (const bool bit);
	BitBuffer& operator<< (const int bit);
	void writeByte(byte_t byte);

	bool read();
	BitBuffer& operator>> (bool & bit);
	byte_t readByte();

	std::vector<bool> readAllBits();
	std::vector<byte_t> readAllBytes(bool padding = 0);

    void storeBitBuffer(std::ostream& file);
    void loadBitBuffer(std::istream& file);

private:
	size_t m_size;
	std::vector<bool> m_bits {};
	size_t m_readIndex, m_writeIndex;
};
