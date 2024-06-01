#include "..\inc\UIState.h"

void UIState::process_inputs (void) {
    while (!control_queue->empty()) {
        char input_container;
        control_queue->wait_pop(input_container);
        this->input_dispatch(input_container);
    }
}

inline bool UIState::is_command (char input) {
    int character = static_cast<int>(input);
    if (character == KBD_CTRL_S || character == KBD_CTRL_R || 
        character == KBD_CTRL_T || character == KBD_CTRL_Q) {
        return true;
    } else {
        return false;
    }
}

void UIState::input_dispatch (char input) {
    
    if(this->is_command(input)){
        command_handler(input);
        return;
    }
    
    switch (this->frame) {
        case 's':
            search_control_handler(input);
            return;
        case 'r':
            //result_control_handler(input);
            return;
        case 't':
            //tags_control_handler(input);
            return;
        case 'q':
            //quit();
            return;
        default:
            return;
    }
}

void UIState::command_handler (char commmand) {
    switch(commmand) {
        case KBD_CTRL_S: // Ctrl-S
            this->frame = 's';
            return;
        case KBD_CTRL_R: // Ctrl-R
            this->frame = 'r';
            return;
        case KBD_CTRL_T: // Ctrl-T 
            this->frame = 't';
            return;
        case KBD_CTRL_Q: // Ctrl-Q
            this->frame = 'q';
            return;
        default:
            return;
    }
}

//==============================================================================
// Handle inputs in SEARCH frame
//==============================================================================

void UIState::search_control_handler (char c) {
    if (c == '\n') {
        this->frame = 'r';
    } else {
        update_search_buffer(c);
    }
}

// add a character to the serach buffer
void UIState::update_search_buffer (char c) {    
    // backspace
    if (c == '\b') {
        if(search_cursor > 0) {
            search_buffer.erase(--search_cursor);
        }
    }

    // Del Key: delete character in front of cursor
    // TODO: Cursor scroll
    else if (c == 127) {
        // if(size_t(search_cursor) < search_buffer.length()-1) {
        //     search_buffer.erase(search_cursor + 1);
        // }
    }

    // printable characters
    else if (std::isprint(c)) {
        search_buffer.reserve(search_buffer.length() + 1);
        search_buffer.push_back(c);
        search_cursor++;
    }

    this->search_exec = true;
}

//==============================================================================
// Handle inputs in RESULT frame
//==============================================================================

// void UIState::result_control_handler (char c) {
//     return;
// }

// int UIState::set_results_scroll (int scroll) {
//     if(scroll < 0) {
//         file_scroll = 0;
//     }
//     if(scroll > files.size()) {
//         file_scroll = files.size();
//     }
//     return file_scroll;
// }

// int UIState::add_results_scroll (int offset) {
//     set_results_scroll(file_scroll + offset);
// }