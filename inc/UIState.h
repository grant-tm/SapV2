
#ifndef UISTATE_H
#define UISTATE_H

#include <string>
#include <vector>

#include "MKBDIO.h"
#include "ThreadSafeQueue.h"
#include "FileRecord.h"

class UIState {
public:
    char frame = 's'; // s search, r results
    
    ThreadSafeQueue<char> *control_queue;

    std::string search_buffer = "";
    std::string search_query = "";
    int search_cursor = 0;
    bool search_exec = false;

    std::vector<struct FileRecord> files;
    int file_scroll = 0;

    void process_inputs (void);
    void input_dispatch (char input);
    inline bool is_command (char input);

    void command_handler (char command);

    void search_control_handler (char input);
    void update_search_buffer (char input);
    
    // void result_control_handler(char c);

    // int set_results_scroll(int scroll);
    // int add_results_scroll(int offset);
};

#endif