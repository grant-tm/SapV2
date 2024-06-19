
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

std::vector<float> *AudioFile::get_samples (void) {
    return &samples;
}

//=============================================================================
// MP3 Parser
//=============================================================================

 

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
        while (chunk_size % 4 != 0) {
            chunk_size++;
        }
        file->seek_bytes(chunk_size);    
    }
    if (attempts >= 10) {
        errlog("Failed to locate format chunk\n");
        return;
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
        while (chunk_size % 4 != 0) {
            chunk_size++;
        }
        file->seek_bytes(chunk_size);
    }
    if (attempts >= 10) {
        errlog("Failed to locate data chunk\n");
        return;
    }

    int data_len = file->read_int(4);

    int bps = (bit_depth / 8) / n_channels;
    if (bps <= 0 || bps > 4) { 
        print_file_path();
        errlog("WAV::parse: failed to calculate bytes per sample\n");
        return;
    }

    /*
    for (int i = 0; i < data_len - bps; i += bps) {
        uint32_t acc = 0;
        for (int j = 0; j < bps; j++) {
            char byte = file->read_byte();
            acc |= static_cast<uint32_t>(byte) << (j * 8);
        }
    
        int16_t *r16 = reinterpret_cast<int16_t *>(&acc);
        int32_t *r32 = reinterpret_cast<int32_t *>(&acc);
        int32_t s32 = static_cast<int32_t>(acc);
    
        float result;
        switch (bit_depth) {
        case 16:
            result = static_cast<float>(*r16) / 32768.0f; 
            break;
        case 24:
            acc &= 0xFFFFFF;        // mask to 24 bits
            if (acc & 0x800000) {   // if the sign bit is set
                acc |= 0xFF000000;  // sign-extend to 32 bits
            }
            result = static_cast<float>(s32) / 8388608.0f;
            break;
        case 32:
            // Assuming 32-bit signed PCM
            result = static_cast<float>(*r32) / 2147483648.0f;
            break;
        default:
            std::cerr << "Unsupported bit depth: " << bit_depth << "\n";
            return;
        }
    
        samples.push_back(result);
    }*/
   
    /*
    samples.reserve(data_len / bps);
    for (int i = 0; i < data_len; i += bps) {
        uint32_t acc = 0;
        for (int j = 0; j < bps; ++j) {
            acc |= static_cast<uint32_t>(static_cast<unsigned char>(file->get_byte(i+j))) << (j * 8);
        }

        float result;
        switch (bit_depth) {
        case 16:
            result = static_cast<float>(static_cast<int16_t>(acc)) / 32768.0f;
            break;
        case 24:
            acc &= 0xFFFFFF; // mask to 24 bits
            if (acc & 0x800000) { // if the sign bit is set
                acc |= 0xFF000000; // sign-extend to 32 bits
            }
            result = static_cast<float>(static_cast<int32_t>(acc)) / 8388608.0f;
            break;
        case 32:
            result = static_cast<float>(static_cast<int32_t>(acc)) / 2147483648.0f;
            break;
        default:
            std::cerr << "Unsupported bit depth: " << bit_depth << "\n";
            return;
        }

        samples.push_back(result);
    }
    */

    const unsigned char *addr = file->get_loc();
    samples.reserve(data_len / bps);
    for (int i = 0; i < data_len; i += bps) {
        uint32_t acc = 0;
        switch (bps) {
        case 1:
            acc = static_cast<uint32_t>(addr[i]);
            break;
        case 2:
            acc = *reinterpret_cast<const uint16_t *>(addr + i);
            break;
        case 3:
            acc = *reinterpret_cast<const uint16_t *>(addr + i);
            acc |= static_cast<uint32_t>(addr[i + 2]) << 16;
            break;
        case 4:
            acc = *reinterpret_cast<const uint32_t *>(addr + i);
            break;
        default:
            std::cerr << "Unsupported bytes per sample: " << bps << "\n";
            return;
        }

        float result;
        switch (bit_depth) {
        case 8:
            result = static_cast<float>(static_cast<int8_t>(acc)) / 128.0f;
            break;
        case 16:
            result = static_cast<float>(static_cast<int16_t>(acc)) / 32768.0f;
            break;
        case 24:
            acc &= 0xFFFFFF; // mask to 24 bits
            if (acc & 0x800000) { // if the sign bit is set
                acc |= 0xFF000000; // sign-extend to 32 bits
            }
            result = static_cast<float>(static_cast<int32_t>(acc)) / 8388608.0f;
            break;
        case 32:
            result = static_cast<float>(static_cast<int32_t>(acc)) / 2147483648.0f;
            break;
        default:
            std::cerr << "Unsupported bit depth: " << bit_depth << "\n";
            return;
        }

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

