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

// FLTK Inclusions
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Box.H>

Database db("audio_files.db");

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
            
            /*if (++num >= 10) {
                break;
            }*/
            
            /*const char *label = record.file_name.c_str();
            if (!label) {
                label = "Bruh";
            }*/
            const char *label = "bruh";

            fprintf(stderr, "%s\n", label);
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
        Fl_Box *invisible_box = new Fl_Box(0, 0, w() - 20, content_height); // Invisible box to set the scrollable area
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
    std::vector<struct FileRecord> files = db.search_by_name(input->value());
    search_results->set_files(files);
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

int main(int argc, char **argv) {
    
    
    fs::path path("D:/Samples/Drums/Fills/Cymatics - Terror Fill 010.wav");
    WAV wav(path);
    wav.parse();
    wav.close();
    //return EXIT_SUCCESS;
    

    // scan the files
    fprintf(stderr, "Scanning Files...\n");
    const std::string dir_path = "D:/Samples/";
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

    //fprintf(stderr, "\rStarting in 3...");
    //Sleep(1000);
    //fprintf(stderr, "\rStarting in 2...");
    //Sleep(1000);
    //fprintf(stderr, "\rStarting in 1...");
    //Sleep(1000);
    //fprintf(stderr, "\r\n");

    Fl_Window *window = new Fl_Window(600, 600, "Search Bar Example");

    SearchInput *search_input = new SearchInput(50, 20, 300, 30, "");
    search_results = new SearchResults(50, 50, 380, 280, "");
    
    window->resizable(search_results);
    window->end();
    window->show(argc, argv);

    return Fl::run();
}