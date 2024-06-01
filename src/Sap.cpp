#include <iostream>
#include <stdio.h>
#include <chrono>
#include <vector>

// Project Inclusions
#include "Database.h"
#include "ThreadSafeQueue.h"
#include "Scanner.h"

// FLTK Inclusions
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Box.H>

// callback function for the search input
void search_cb(Fl_Widget *widget, void *data) {
    Fl_Input *input = static_cast<Fl_Input *>(widget);
    const char *search_query = input->value();
    // Add search handler here
}

class SearchInput : public Fl_Input {
public:
    SearchInput (int X, int Y, int W, int H, const char *L = 0) : Fl_Input(X, Y, W, H, L) {}

    int handle (int event) override {
        int ret = Fl_Input::handle(event);
        if (event == FL_KEYDOWN || event == FL_RELEASE) {
            do_callback();
        }
        return ret;
    }
};

// Callback function for button click events
void file_button_callback (Fl_Widget *widget, void *data) {
    Fl_Button *button = (Fl_Button *)widget;
    // Display the file name when the button is clicked
    printf("File selected: %s\n", button->label());
}


int main(int argc, char **argv) {
    
    // open the database
    sqlite3* db = nullptr;
    if (sqlite3_open_v2("audio_files.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL) != SQLITE_OK) { 
        panicf("Cannot open database.\n");
    }
    db_initialize(db);

    // scan the files
    fprintf(stderr, "Scanning Files...\n");
    const std::string dir_path = "D:/Samples/";
    int db_size_before = db_get_num_rows (db, "audio_files");
    auto start = std::chrono::high_resolution_clock::now();
    scan_directory(db, dir_path);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    int db_size_after = db_get_num_rows (db, "audio_files");
    
    // report results
    fprintf(stderr, "Files Scanned: %d\n", db_size_after - db_size_before);
    fprintf(stderr, "Scan duration: %f\n", duration.count() / 1000);
    fprintf(stderr, "Scan Performance: %f Files / Second\n", float(db_size_after - db_size_before) / (duration.count() / 1000));

    fprintf(stderr, "\rStarting in 3...");
    Sleep(1000);
    fprintf(stderr, "\rStarting in 2...");
    Sleep(1000);
    fprintf(stderr, "\rStarting in 1...");
    Sleep(1000);
    fprintf(stderr, "\r\n");


    return 0;

    Fl_Window *window = new Fl_Window(600, 600, "Search Bar Example");

    SearchInput *search_input = new SearchInput(50, 20, 300, 30, "");
    search_input->textsize(22);
    search_input->callback(search_cb);

    Fl_Scroll *scroll = new Fl_Scroll(50, 50, 380, 280);
    scroll->type(Fl_Scroll::VERTICAL_ALWAYS);
    scroll->begin();

    // Sample file names
    std::vector<std::string> file_names = {
        "file1.txt", "file2.txt", "file3.txt", "file4.txt", "file5.txt",
        "file6.txt", "file7.txt", "file8.txt", "file9.txt", "file10.txt",
        "VeryLongFileNameExample - It's Long! Who could fit such a name?"
    };

    int y_offset = 10;
    for (const auto &file_name : file_names) {
        Fl_Button *button = new Fl_Button(50, y_offset, 340, 25, file_name.c_str());
        button->box(FL_FLAT_BOX);
        button->labelsize(20);
        button->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        button->callback(file_button_callback);
        y_offset += 20; // Adjust spacing between items
    }
    scroll->scroll_to(scroll->xposition(), -1);

    scroll->end();
    window->resizable(scroll);
 
    window->end();
    window->show(argc, argv);

    return Fl::run();
}