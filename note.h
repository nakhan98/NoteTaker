#include <string>
#include <vector>

#ifndef NOTE_H
#define NOTE_H


class Note {
    public:
        static std::vector<Note *> NoteList;
        Note(std::string title, std::string message, std::string date="",
                int id=0, bool new_message=true);
        ~Note();

        // Arg parsing
        static void process_args(int argc, char **argv);

        // retrieve Note
        static Note* get_note(int id);

        // edit note
        static void edit_note(int id);

        //get_message
        static std::string get_tmp_message(std::string message="");
        //
        // Create temporary file to save messages in
        static void create_tmp_file(std::string tmp_file, std::string message);

        // get id for new message
        int get_id();

        // destroy all notes in memory
        static void destroy_all_notes();

        //Get default editor
        static std::string get_default_editor();

        // Open tmp_file in editor
        static void open_file_in_editor(std::string editor,
                                        std::string file);

        // Read tmp file
        static std::string read_tmp_file(std::string tmp_file); 

        // Delete tmp file
        static void delete_tmp_file(std::string tmp_file);

        //print all notes
        static void print_all_notes();

        //Get console width
        static int get_console_width();

        //Calculate title width
        static int calc_title_width(int console_width);
        
        //create row format
        static const char * create_note_row(int title_width);

        //get current date
        static std::string get_current_date();

        // Setters
        std::string get_title();
        std::string get_message();

        static void load_notes();

    private:
        // Generic info 
        static const std::string NOTE_TAKER_INFO;

        // File to save notes in 
        static const std::string NOTES_FILE;

        // Version number
        static const float VERSION;

        // Default message in temp file when adding note
        static const std::string ADD_NOTE_MSG;

        // save messages as JSON
        static void save_notes();

        // Message specific fields
        int id;
        std::string title;
        std::string message;
        std::string date;
};

#endif
