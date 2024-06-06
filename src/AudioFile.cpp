
#include "AudioFile.h"

bool
bitcmp
(
    std::vector<char> *bytes,
    const char *string
) {
    if (!bytes || !string) {
        return false;
    }

    // get the length of the string
    size_t length = std::strlen(string);
    if (bytes->size() != length) {
        return false;
    }

    // compare the bytes in the vector with the string
    for (size_t i = 0; i < length; i++) {
        if ((*bytes)[i] != string[i]) {
            return false;
        }
    }

    return true; // all bytes match
}

int
bytes_to_int
(
    std::vector<char> *bytes
) {
    if (bytes->size() < 1 || bytes->size() > 4) {
        throw std::invalid_argument("AudioFile::btoi: >4 bytes provided.");
    }

    int result = 0;
    for (size_t i = 0; i < bytes->size(); i++) {
        result |= (static_cast<unsigned char>((*bytes)[i]) << (i * 8));
    }

    return result;
}

//=============================================================================
// Byte Extractor - read n bytes from an input stream
//=============================================================================

ByteExtractor::ByteExtractor
(
    fs::path file_path
) : std::ifstream(file_path, std::ios::in | std::ios::binary) {
    if (!is_open()) {
        throw std::ios_base::failure("Byte Extractor: error opening file.");
    }
}

// Function to get a single byte
char 
ByteExtractor::read_byte
(
    void
) {
    char byte;
    if (this->read(&byte, 1)) {
        return byte;
    }
    else {
        throw std::ios_base::failure("Byte Extractor: Error reading byte");
    }
}

// Function to get multiple bytes
std::vector<char> 
ByteExtractor::read_bytes
(
    int bytes
) {
    std::vector<char> buffer(bytes);
    if (this->read(buffer.data(), bytes)) {
        return buffer;
    }
    else {
        throw std::ios_base::failure("Byte Extractor: error reading bytes");
    }
}

bool
ByteExtractor::read_string
(
    const char *string
) {
    int string_length = std::strlen(string);

    char readbuf[64];  // assuming the string length won't exceed 64

    if (string_length > sizeof(readbuf)) {
        throw std::invalid_argument (
            "ByteExtractor:read_string: string length exceeds max (64).\n"
        );
    }

    this->read(readbuf, string_length);

    // ensure the read operation was successful
    if (this->gcount() != string_length) {
        return false;
    }

    return std::memcmp(string, readbuf, string_length) == 0;
}

int
ByteExtractor::read_int
(
    int num_bytes
) {
    char bytes[4];
    this->read(bytes, num_bytes);
    if (this->gcount() != num_bytes) {
        return -1;
    }
    
    int result = 0;
    for (int i = 0; i < num_bytes; i++) {
        result |= (static_cast<unsigned char>(bytes[i]) << (i * 8));
    }
    return result;
}



//=============================================================================
// Audio File base class
//=============================================================================

// default constructor
AudioFile::AudioFile
(
    void
) {
    this->sample_rate = 0;
    this->num_channels = 0;

    file = nullptr;
}

// open file from constructor
AudioFile::AudioFile
(
    fs::path file_path
) {
    // set defaults
    this->sample_rate = 0;
    this->num_channels = 0;

    // open stream
    int opened = this->open(file_path);
    this->file_path = file_path;
}

// destructor frees ByteExtractor
AudioFile::~AudioFile
(
    void
) {
    if (this->file) {
        if (this->file->is_open()) {
            this->file->close();
        }
        delete file;
    }
}

// open the ifstream
// opens with ios::in | ios::binary
int
AudioFile::open
(
    fs::path file_path
) {
    this->file = new ByteExtractor(file_path);
    if (file->is_open()) {
        this->file_path = file_path;
    }
    return this->file->is_open();
}

// close the ifstream if its open
int
AudioFile::close
(
    void
) {
    if (this->file && this->file->is_open()) {
        this->file->close();
    }
    return !(this->file->is_open());
}

//=============================================================================
// MP3 Parser
//=============================================================================

void MP3::parse 
(
    void
) {
    if (!this->file || !this->file->is_open()) {
        return;
    }
}

//=============================================================================
// WAV Parser
//=============================================================================

float b16_to_float(int16_t sample) {
    return static_cast<float>(sample) / 32768.0f; // 32768 = 2^15
}

// Function to convert a 24-bit integer to a float
float b24_to_float(const uint8_t *bytes) {
    int32_t sample = 0;
    std::memcpy(&sample, bytes, 3);
    if (sample & 0x800000) {
        sample |= 0xFF000000;
    }
    return static_cast<float>(sample) / 8388608.0f; // 8388608 = 2^23
}

// Function to convert a 32-bit integer to a float
float b32_to_float(int32_t sample) {
    return static_cast<float>(sample) / 2147483648.0f; // 2147483648 = 2^31
}

void WAV::parse
(
    void
) {
    // check prerequisites
    if (!this->file) {
        errlog("WAV::parse: Uninitialized ifstream.\n");
        return;
    }
    if (!this->file->is_open()) {
        errlog("WAV::parse: No file opened.\n.");
        return;
    }

    fprintf(stderr, "Parsing: %s\n", this->file_path.string().c_str());

    // parse riff section
    if (!file->read_string("RIFF")) {
        errlog("WAV::parse: Invalid RIFF description header.\n");
        return;
    }
    
    int size = file->read_int(4);
    
    // parse wave description section
    if (!file->read_string("WAVE")) {
        errlog("WAV::parse: Invalid WAV description header.\n");
        return;
    }
    
    // parse data section
    int attempts = 0;
    while (!file->read_string("fmt ")) {
        int chunk_size = file->read_int(4);
        errlog("reading another chunk of size: %d\n", chunk_size);
        if (++attempts > 10 || chunk_size < 0) {
            errlog("WAV:parse: Failed to locate format chunk.\n");
            return;
        }
        if (chunk_size > 0) {
            file->read_bytes(chunk_size);
        }    
    }
    
    int chunk_size  = file->read_int(4);
    
    int wave_format = file->read_int(2);
    int n_channels  = file->read_int(2);
    int sample_rate = file->read_int(4);
    int data_rate   = file->read_int(4);
    int block_align = file->read_int(2);
    int bit_depth   = file->read_int(2);

    if (chunk_size > 16) {
        file->read_bytes(chunk_size - 16);
    }
    
    // parse data section
    attempts = 0;
    while (!file->read_string("data")) {
        int chunk_size = file->read_int(4);
        if (++attempts > 10 || chunk_size < 0) {
            errlog("WAV:parse: Failed to locate data chunk.\n");
            return;
        }
        if (chunk_size > 0) {
            file->read_bytes(chunk_size);
        }
    }

    int data_len = file->read_int(4);
    std::vector<char> data = file->read_bytes(data_len);

    int bps;
    (n_channels == 2) ? bps = bit_depth >> 3 : bps = bit_depth >> 2;

    for (int i = 0; i < data_len-bps+1; i += bps) {
        uint32_t acc = 0;
        for (int j = 0; j < bps; j++) {
            acc |= (static_cast<uint32_t>(data[i+j]) << (j * 8));
        }
        float result;
        std::memcpy(&result, &acc, sizeof(result));
        samples.push_back(result);
    }

    fprintf(stderr, "Success\n");
   /* fprintf(stderr, "Mono/Stereo: %d\n", n_channels);
    fprintf(stderr, "Sample Rate: %d\n", sample_rate);
    fprintf(stderr, "Bit Depth:   %d\n", bit_depth);
    fprintf(stderr, "Total Size:  %d bytes\n", size + 8);
    fprintf(stderr, "Data Length: %d bytes\n", data_len);
    fprintf(stderr, "# samples:   %d\n", samples.size());*/
}

//=============================================================================
// FLAC Parser
//=============================================================================

void FLAC::parse
(
    void
) {
    if (!this->file || !this->file->is_open()) {
        return;
    }

    fprintf(stderr, "I am a FLAC!\n");

}

