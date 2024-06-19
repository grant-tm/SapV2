#include <iostream>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <vector>

// Project Inclusions
#include "UIState.h"
#include "Database.h"
#include "FileRecord.h"
#include "ThreadSafeQueue.h"
#include "Scanner.h"
#include "AudioFile.h"
#include "FourierTX.h"

#include <unordered_map>
#include <omp.h>
#include <windows.h>

// FLTK Inclusions
//#include <GuiObjects.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Box.H>

Database db("audio_files.db");

std::unordered_map<fs::path, struct FileRecord> all_files;
std::vector<struct FileRecord> files_in_scope;

std::string wchar_to_utf8(const std::wstring &wide_string) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide_string.c_str(), (int)wide_string.size(), NULL, 0, NULL, NULL);
    std::string utf8_string(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_string.c_str(), (int)wide_string.size(), &utf8_string[0], size_needed, NULL, NULL);
    return utf8_string;
}

//=============================================================================
// FLTK Search Results Window
//=============================================================================

// Callback function for button click events
void file_button_callback (Fl_Widget *widget, void *data) {
    Fl_Button *button = (Fl_Button *)widget;
    // Display the file name when the button is clicked
    printf("File selected: %s\n", button->label());
}

class SearchResults : public Fl_Scroll {
public:
    SearchResults (int xpos, int ypos, int xlen, int ylen, const char *label)
        : Fl_Scroll(xpos, ypos, xlen, ylen, label) {
        this->type(Fl_Scroll::VERTICAL_ALWAYS);
    }

    void set_files (std::vector<struct FileRecord> records) {

        this->clear();

        this->begin();

        //fprintf(stderr, "num files found: %d\n", records.size());

        int y_offset = 10;
        int num = 0;

        for (auto &record : records) {
            if (++num >= 100) {
                break;
            }

            /*const char *label = wchar_to_utf8(record.file_name.c_str()).c_str();
            if (!label) {
                label = "blank_label";
            }*/
            const char *label = "test_label";
            
            Fl_Button *button = new Fl_Button(50, y_offset, 340, 25, label);
            button->box(FL_FLAT_BOX);
            button->labelsize(20);
            button->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
            button->callback(file_button_callback);

            y_offset += 20; // Adjust spacing between items
        }

        this->end();
        this->resize_content(y_offset);
        this->redraw();
    }
private:
    void resize_content(int height) {
        int content_height = height > h() ? height : h();
        this->init_sizes(); // Ensure sizes are properly initialized
        Fl_Box *invisible_box = new Fl_Box(0, 0, w() - 20, content_height);
        invisible_box->hide();
        add(invisible_box);
    }
};
SearchResults *search_results = nullptr;

//=============================================================================
// FLTK Search Bar
//=============================================================================

// callback function for the search input
static void search_callback (Fl_Widget *widget, void *data) {
    Fl_Input *input = static_cast<Fl_Input *>(widget);
    fprintf(stderr, "%s\n", input->value());
    std::vector<struct FileRecord> files;
    db.search_by_name(&files_in_scope, input->value());
    for (size_t i = 0; i < files.size(); i++) {
        //files[i].file_name = L"bruh";
        std::wcout << files[i].file_name << std::endl;
    }
    //search_results->set_files(files);
}

class SearchInput : public Fl_Input {
public:
    SearchInput (int xpos, int ypos, int xlen, int ylen, const char *label)
        : Fl_Input(xpos, ypos, xlen, ylen, label) {
        this->textsize(22);
        this->callback(search_callback);
    }

    int handle (int event) override {
        int ret = Fl_Input::handle(event);
        if (event == FL_KEYDOWN || event == FL_RELEASE) {
            do_callback();
        }
        return ret;
    }
};

//=============================================================================
// MAIN
//=============================================================================

// Parser testing/benchmarking
/*
int main(int argc, char **argv) {

    auto start = std::chrono::high_resolution_clock::now();
    //WAV wav("D:/Samples/Instruments/Synths/One Shots/003_Synth_Hit_C_-_LOFICHILL_Zenhiser.wav");
    WAV wav("D:/Samples/Remixes/Remix Packs/Chainsmokers/The Chainsmokers - Takeaway.wav");
    //WAV wav("D:/Samples/Instruments/Synths/Loops/RU_TE_90_theremin_glissando_B_to_G_reverb_B.wav");
    
    wav.parse();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    fprintf(stderr, "File Parse Duration (ms): %f\n", duration.count());

    // Check for NaN values in input
    for (const auto &value : *(wav.get_samples())) {
        //std::cerr << value << std::endl;
        if (std::isnan(value)) {
            std::cerr << "input contains NaN values." << std::endl;
            return -1;
        }
    }
    std::cerr << "Input does not contain any NaN values." << std::endl;

    FourierTX ftx;
    
    int window_size = 128;
    int num_iters = 0;
    float total_time = 0;

    start = std::chrono::high_resolution_clock::now();
    ftx.chroma_features(wav.get_samples(), window_size);
    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    fprintf(stderr, "\nSingle-Processor FFT Duration (ms): %f\n", duration.count());

    return EXIT_SUCCESS;
}
*/

// SAP
int main(int argc, char **argv) {
    // scan the files
    fprintf(stderr, "Scanning Files...\n");
    const std::string dir_path = "D:/Samples/Instruments/Guitar";
    int db_size_before = db.num_rows("audio_files");
    auto start = std::chrono::high_resolution_clock::now();
    scan_directory(&db, dir_path);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    int db_size_after = db.num_rows("audio_files");

    // report scan performance results
    int num_scanned = db_size_after - db_size_before;
    fprintf(stderr, "Files Scanned: %d\n", num_scanned);
    float seconds_elapsed = float(duration.count()) / float(1000.0);
    fprintf(stderr, "Scan duration: %f\n", seconds_elapsed);
    float performance = float(num_scanned) / seconds_elapsed;
    fprintf(stderr, "Scan Performance: %f Files / Second\n", performance);

    Fl_Window *window = new Fl_Window(600, 600, "Search Bar Example");

    SearchInput *search_input = new SearchInput(50, 20, 300, 30, "");
    search_results = new SearchResults(50, 50, 380, 280, "");

    window->resizable(search_results);
    window->end();
    window->show(argc, argv);

    return Fl::run();
}

/*
std::filesystem::path utf16BlobToPath(const std::vector<uint8_t> &blob) {
    const uint16_t *data = reinterpret_cast<const uint16_t *>(blob.data());
    std::u16string u16str(data, data + blob.size() / 2);
    return std::filesystem::path(u16str);
}

int main() {
    sqlite3 *db;
    sqlite3_open("example.db", &db);

    // Insert an example path
    std::filesystem::path filePath = "/example/path/to/file.txt";
    std::u16string pathString = filePath.u16string();

    sqlite3_exec(db, "DELETE FROM paths_table;", nullptr, nullptr, nullptr); // Clear table for the example
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "INSERT INTO paths_table (path) VALUES (?);", -1, &stmt, nullptr);
    sqlite3_bind_text16(stmt, 1, pathString.c_str(), pathString.size() * sizeof(char16_t), SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Use LIKE to search for paths
    const char16_t *searchPattern = u"%file%";
    sqlite3_prepare_v2(db, "SELECT path FROM paths_table WHERE path LIKE ?;", -1, &stmt, nullptr);
    sqlite3_bind_text16(stmt, 1, searchPattern, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const void *text = sqlite3_column_text16(stmt, 0);
        int text_size = sqlite3_column_bytes16(stmt, 0);
        std::u16string pathString(reinterpret_cast<const char16_t *>(text), text_size / sizeof(char16_t));

        std::filesystem::path filePath = std::filesystem::path(pathString);
        std::wcout << L"Found path: " << filePath << std::endl;
    }
    else {
        std::wcout << "Failed to locate file" << std::endl;
    }
    sqlite3_finalize(stmt);

    sqlite3_close(db);
    return 0;
}
*/