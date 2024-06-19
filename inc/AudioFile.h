#ifndef Audio_File_h
#define Audio_File_h

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
#include "ByteExtractor.h"

namespace fs = std::filesystem;

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

	void print_file_path (void);

	std::vector<float> *get_samples (void);

protected:
	fs::path file_path;
	ByteExtractor *file;
	
	int sample_rate;
	int num_channels;

	std::vector<float> samples;
};

//=============================================================================
// MP3
//=============================================================================
class MP3 : public AudioFile {
public:
	MP3() : AudioFile() {};
	MP3(fs::path path) : AudioFile(path) {};
	void parse (void) override;
};

//=============================================================================
// WAV
//=============================================================================
class WAV : public AudioFile {
public:
	WAV() : AudioFile() {};
	WAV(fs::path path) : AudioFile(path) {};
	void parse (void) override;
};
//=============================================================================
// FLAC
//=============================================================================
class FLAC : public AudioFile {
public:
	FLAC() : AudioFile() {};
	FLAC(fs::path path) : AudioFile(path) {};
	void parse (void) override;
};

#endif // Audio_File_h