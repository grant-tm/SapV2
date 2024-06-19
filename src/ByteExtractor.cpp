#include "ByteExtractor.h"

//-----------------------------------------------------------------------------
// ByteExtractor (fs::path)
// ----------------------------------------------------------------------------
// Create a ByteExtractor and open the file specified by an fs::path
//-----------------------------------------------------------------------------
ByteExtractor::ByteExtractor (fs::path file_path) {
    open(file_path);
}

//-----------------------------------------------------------------------------
// ~ByteExtractor
// ----------------------------------------------------------------------------
// Default destructor. Unmaps any open memory mapped files.
//-----------------------------------------------------------------------------
ByteExtractor::~ByteExtractor (void) {
    close();
}

//-----------------------------------------------------------------------------
// open
// ----------------------------------------------------------------------------
// Map the binary of a file specified by an fs::path to the buffer.
//-----------------------------------------------------------------------------
void ByteExtractor::open (const fs::path &file_path) {
    close(); // Ensure any previously opened file is closed

    file_handle = CreateFileW(
        file_path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (file_handle == INVALID_HANDLE_VALUE) {
        panicf("Byte Extractor: error opening file.");
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle, &file_size)) {
        CloseHandle(file_handle);
        file_handle = INVALID_HANDLE_VALUE;
        panicf("Byte Extractor: error getting file size.");
    }

    buffer_size = static_cast<size_t>(file_size.QuadPart);

    map_handle = CreateFileMapping(
        file_handle,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
    );

    if (map_handle == NULL) {
        CloseHandle(file_handle);
        file_handle = INVALID_HANDLE_VALUE;
        panicf("Byte Extractor: error creating file mapping.");
    }

    buffer = static_cast<unsigned char *>(MapViewOfFile(
        map_handle,
        FILE_MAP_READ,
        0,
        0,
        0
    ));

    if (buffer == NULL) {
        CloseHandle(map_handle);
        CloseHandle(file_handle);
        file_handle = INVALID_HANDLE_VALUE;
        map_handle = NULL;
        panicf("Byte Extractor: error mapping view of file.");
    }

    current_idx = 0;
}

//-----------------------------------------------------------------------------
// close
// ----------------------------------------------------------------------------
// Unmap any open memory mapped files and clear the file and map handles.
//-----------------------------------------------------------------------------
void ByteExtractor::close() {
    if (buffer) {
        UnmapViewOfFile(buffer);
        buffer = nullptr;
    }
    if (map_handle != NULL) {
        CloseHandle(map_handle);
        map_handle = NULL;
    }
    if (file_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(file_handle);
        file_handle = INVALID_HANDLE_VALUE;
    }
    buffer_size = 0;
    current_idx = 0;
}

//-----------------------------------------------------------------------------
// is_open
// ----------------------------------------------------------------------------
// Returns whether or not a file is open
//-----------------------------------------------------------------------------
bool ByteExtractor::is_open (void) {
    return file_handle != INVALID_HANDLE_VALUE;
}

//-----------------------------------------------------------------------------
// seek_bytes
// ----------------------------------------------------------------------------
// Move the cursor head n bytes (positive or negative) in the buffer.
//-----------------------------------------------------------------------------
int ByteExtractor::seek_bytes (int bytes) {
    int new_idx = max(current_idx + bytes, 0);
    current_idx = min(current_idx + bytes, buffer_size - 1);
    return current_idx;
}

//-----------------------------------------------------------------------------
// get_cursor
// ----------------------------------------------------------------------------
// Get the index of the cursor within the ByteExtractor buffer.
//-----------------------------------------------------------------------------
size_t ByteExtractor::get_cursor (void) {
    return current_idx;
}

//-----------------------------------------------------------------------------
// goto_byte
// ----------------------------------------------------------------------------
// Move the cursor to a byte within the buffer.
//-----------------------------------------------------------------------------
int ByteExtractor::goto_byte (int byte) {
    byte = max(byte, 0);
    current_idx = min(static_cast<size_t>(byte), buffer_size - 1);
    return current_idx;
}

//-----------------------------------------------------------------------------
// read_byte
// ----------------------------------------------------------------------------
// Read 1 byte as a char at the cursor head. Advances the cursor 1 byte.
//-----------------------------------------------------------------------------
unsigned char ByteExtractor::read_byte (void) {
    if (current_idx >= buffer_size || current_idx < 0) {
        return NULL;
    }
    return buffer[current_idx++];
}

//-----------------------------------------------------------------------------
// read_bytes
// ----------------------------------------------------------------------------
// Fill a provided char vector with n bytes at the cursor head. Advances the
// cursor n bytes.
//-----------------------------------------------------------------------------
void ByteExtractor::read_bytes (std::vector<unsigned char> *container, int bytes) {

    if (container == nullptr) {
        current_idx = min(current_idx + bytes, buffer_size);
        return;
    }

    if (current_idx + bytes > buffer_size) {
        bytes = buffer_size - current_idx;
    }

    container->assign(buffer + current_idx, buffer + current_idx + bytes);
    current_idx += bytes;
}

//-----------------------------------------------------------------------------
// cmp_read
// ----------------------------------------------------------------------------
// Check if the next memory locations under the cursor head match a provided
// string.
//-----------------------------------------------------------------------------
bool ByteExtractor::cmp_read(const char *string) {
    int strlen = std::strlen(string);

    if (current_idx + strlen > buffer_size) {
        return false;
    }

    bool result = std::memcmp(buffer + current_idx, string, strlen) == 0;
    if (result) {
        current_idx += strlen;
    }
    return result;
}

//-----------------------------------------------------------------------------
// read_string
// ----------------------------------------------------------------------------
// Read n bytes, interpretted as a c++ string. Advances the cursor n bytes.
//-----------------------------------------------------------------------------
std::string ByteExtractor::read_string (int length) {
    if (length < 0 || current_idx + length > buffer_size) {
        return "";
    }

    std::string result(buffer + current_idx, buffer + current_idx + length);
    current_idx += length;

    return result;
}

//-----------------------------------------------------------------------------
// read_int
// ----------------------------------------------------------------------------
// Read n bytes interpretted as a single integer value.
//-----------------------------------------------------------------------------
int ByteExtractor::read_int (int num_bytes) {
    if (current_idx + num_bytes > buffer_size) {
        return -1;
    }

    int result = 0;
    for (int i = 0; i < num_bytes; i++) {
        result |= buffer[current_idx + i] << (i*8);
    }
    current_idx += num_bytes;

    return result;
}

//-----------------------------------------------------------------------------
// get_byte
// ----------------------------------------------------------------------------
// Random access byte in ByteExtractor's internal buffer.
//-----------------------------------------------------------------------------
unsigned char ByteExtractor::get_byte (size_t idx) {
    if (idx < 0 || idx > buffer_size) {
        return NULL;
    }
    return buffer[idx];
}

//-----------------------------------------------------------------------------
// get_loc
// ----------------------------------------------------------------------------
// Get the mem location of a byte within the ByteExtractor's internal buffer.
//-----------------------------------------------------------------------------
const unsigned char *ByteExtractor::get_loc (void) {
    if (current_idx < 0 || current_idx > buffer_size) {
        return nullptr;
    }

    return buffer + current_idx;
}

//-----------------------------------------------------------------------------
// get_byte_loc
// ----------------------------------------------------------------------------
// Get the mem location of a byte within the ByteExtractor's internal buffer.
//-----------------------------------------------------------------------------
const unsigned char *ByteExtractor::get_byte_loc (size_t idx) {
    if (idx < 0 || idx > buffer_size) {
        return nullptr;
    }
    
    return buffer + idx;
}