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

	// constructors / destructors
	ByteExtractor(fs::path);
	~ByteExtractor(void);

	// file management
	void open(const fs::path &filePath);
	void close(void);
	bool is_open(void);

	// move or read cursor
	int goto_byte (int);
	int seek_bytes (int);
	size_t get_cursor (void);

	// read at cursor
	unsigned char read_byte (void);
	void read_bytes (std::vector<unsigned char> *container, int bytes);
	int read_int (int);
	std::string read_string (int length);
	bool cmp_read (const char *);

	// random access read
	unsigned char get_byte (size_t);

	// get memory locations
	const unsigned char *get_loc (void);
	const unsigned char *get_byte_loc (size_t);

private:
	HANDLE file_handle;
	HANDLE map_handle;

	const unsigned char *buffer;
	size_t buffer_size;
	size_t current_idx;
};

#endif // Byte_Extractor_h