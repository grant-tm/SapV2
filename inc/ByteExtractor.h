#ifndef Byte_Extractor_h
#define Byte_Extractor_h

#include <stdio.h>
#include <filesystem>
#include <vector>
#include <fstream>
#include <bitset>
#include <string.h>
#include <omp.h>
#include <iostream>
#include <math.h>
#include <windows.h>
#include <cstring>
#include <stdexcept>
#include "SystemUtilities.h"

namespace fs = std::filesystem;

//=============================================================================
// Byte Extractor - read n bytes from an input stream
//=============================================================================

class ByteExtractor {
public:

	ByteExtractor(fs::path);
	~ByteExtractor(void);

	void open(const fs::path &filePath);
	void close(void);
	bool is_open(void);

	int goto_byte (int);
	int seek_bytes (int);

	char read_byte (void);
	void read_bytes (std::vector<char> *container, int bytes);
	int read_int (int);
	std::string read_string (int length);
	bool cmp_read (const char *);

private:
	HANDLE file_handle;
	HANDLE map_handle;

	char *buffer;
	size_t buffer_size;

	size_t current_idx;
};

#endif // Byte_Extractor_h