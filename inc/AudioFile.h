#ifndef Audio_File_h
#define Audio_File_h

#include <stdio.h>
#include <filesystem>
#include <vector>
#include <fstream>
#include <bitset>
#include <string.h>
#include <omp.h>

#include "SystemUtilities.h"

namespace fs = std::filesystem;

bool bitcmp (std::vector<char> *, const char *);
int bytes_to_int(std::vector<char> *);

//=============================================================================
// Byte Extractor - read n bytes from an input stream
//=============================================================================
class ByteExtractor : public std::ifstream {
public:
	ByteExtractor(fs::path);
	char read_byte (void);
	std::vector<char> read_bytes (int);
	int read_int (int);
	bool read_string(const char *);
};

//=============================================================================
// Audio File base class
//=============================================================================
class AudioFile {
public:

	AudioFile();
	AudioFile(fs::path);
	~AudioFile();

	// open_file calls virtual function member extract()
	int open (fs::path);
	int close (void);

	// getters
	int get_sample_rate (void);
	int get_num_samples (void);
	int get_num_channels (void);

	// each audio file should implement this function
	virtual void parse (void) = 0;

protected:
	int sample_rate;
	int num_channels;
	std::vector<float> samples;
	fs::path file_path;
	
	ByteExtractor *file;

	
};

//=============================================================================
// MP3
//=============================================================================
class MP3 : public AudioFile {
public:
	MP3() : AudioFile() {};
	MP3(fs::path path) : AudioFile(path) {};
	void parse (void);
};

//=============================================================================
// WAV
//=============================================================================
class WAV : public AudioFile {
public:
	WAV() : AudioFile() {};
	WAV(fs::path path) : AudioFile(path) {};
	void parse (void);
};
//=============================================================================
// FLAC
//=============================================================================
class FLAC : public AudioFile {
public:
	FLAC() : AudioFile() {};
	FLAC(fs::path path) : AudioFile(path) {};
	void parse (void);
};

#endif // Audio_File_h