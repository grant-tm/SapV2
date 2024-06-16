
#include "AudioFile.h"

//=============================================================================
// Audio File base class
//=============================================================================

// default constructor
AudioFile::AudioFile (void) {
    this->sample_rate = 0;
    this->num_channels = 0;

    file = nullptr;
}

// open file from constructor
AudioFile::AudioFile (fs::path file_path) {
    // set defaults
    this->sample_rate = 0;
    this->num_channels = 0;

    // open stream
    int opened = this->open(file_path);
    this->file_path = file_path;
}

// destructor frees ByteExtractor
AudioFile::~AudioFile (void) {
    if (this->file) {
        if (this->file->is_open()) {
            this->file->close();
        }
        delete file;
    }
}

// open the ifstream
// opens with ios::in | ios::binary
int AudioFile::open (fs::path file_path) {
    this->file = new ByteExtractor(file_path);
    if (file->is_open()) {
        this->file_path = file_path;
    }
    return this->file->is_open();
}

// close the ifstream if its open
int AudioFile::close (void) { 
    if (this->file->is_open()) {
        this->file->close();
    }
    return 1;
}

void AudioFile::print_file_path (void) {
    std::cerr << this->file_path << std::endl;
}

//=============================================================================
// MP3 Parser
//=============================================================================

void MP3::parse (void) {
    if (!this->file || !this->file->is_open()) {
        return;
    }
}

//=============================================================================
// WAV Parser
//=============================================================================

void WAV::parse (void) {
    
    // prerequisites
    if (!this->file) {
        errlog("WAV::parse: Uninitialized ifstream.\n");
        return;
    }
    if (!this->file->is_open()) {
        errlog("WAV::parse: No file opened.\n.");
        return;
    }

    this->file->goto_byte(0);

    // parse riff section
    if (!file->cmp_read("RIFF")) {
        print_file_path();
        errlog("WAV::parse: Invalid RIFF description header.\n");
        return;
    }
    
    int size = file->read_int(4);
    
    // parse wave description section
    if (!file->cmp_read("WAVE")) {
        print_file_path();
        errlog("WAV::parse: Invalid WAV description header.\n");
        return;
    }
    
    // parse data section
    int attempts = 0;
    int flag = 0;
    while (++attempts < 10) {
        std::string chunk_id = file->read_string(4);
        if (chunk_id == std::string("fmt ")) {
            break;
        }
        int chunk_size = file->read_int(4);
        file->seek_bytes(chunk_size);    
    }
    
    int chunk_size  = file->read_int(4);
    int wave_format = file->read_int(2);
    int n_channels  = file->read_int(2);
    int sample_rate = file->read_int(4);
    int data_rate   = file->read_int(4);
    int block_align = file->read_int(2);
    int bit_depth   = file->read_int(2);

    if (chunk_size > 16) {
        file->seek_bytes(chunk_size - 16);
    }

    attempts = 0;
    while (++attempts < 10) {
        std::string chunk_id = file->read_string(4);
        if (chunk_id == std::string("data")) {
            break;
        }
        int chunk_size = file->read_int(4);
        file->seek_bytes(chunk_size);
    }

    int data_len = file->read_int(4);

    int bps = (bit_depth / 8) / n_channels;
    if (bps <= 0 || bps > 4) { 
        print_file_path();
        errlog("WAV::parse: failed to calculate bytes per sample\n");
        return;
    }

    for (int i = 0; i < data_len-bps; i += bps) {
        uint32_t acc = 0;
        for (int j = 0; j < bps; j++) {
            uint32_t read = static_cast<uint32_t>(file->read_byte());
            acc |= read << (j * 8);
        }
        float result;
        std::memcpy(&result, &acc, sizeof(result));
        samples.push_back(result);
    }

    /*fprintf(stderr, "Success\n");*/
    //fprintf(stderr, "Mono/Stereo: %d\n", n_channels);
    //fprintf(stderr, "Sample Rate: %d\n", sample_rate);
    //fprintf(stderr, "Bit Depth:   %d\n", bit_depth);
    //fprintf(stderr, "Total Size:  %d bytes\n", size + 8);
    //fprintf(stderr, "Data Length: %d bytes\n", data_len);
    //fprintf(stderr, "# samples:   %d\n", samples.size());
}

//=============================================================================
// FLAC Parser
//=============================================================================

void FLAC::parse (void) {
    if (!this->file || !this->file->is_open()) {
        return;
    }

    fprintf(stderr, "I am a FLAC!\n");

}

