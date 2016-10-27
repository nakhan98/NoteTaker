#include <string>
#include <vector>

#ifndef NOTE_H
#define NOTE_H

const std::string NOTE_TAKER_HELP = "This is the notetaker help!";

class Note {
    public:
        static std::vector<Note *> NoteList;
        Note(std::string title, std::string message, int id=0,
                bool new_message=true);
        ~Note();

        // Arg parsing
        static void process_args(int argc, char **argv);

        //get_message
        static std::string get_tmp_message();
        //
        // Create temporary file to save messages in
        static void create_tmp_file(std::string tmp_file);

        // get id for new message
        int get_id();

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

        // Setters
        std::string get_title();
        std::string get_message();

        static void load_notes();

    private:
        // File to save notes in 
        static const std::string NOTES_FILE;

        // Version number
        static const float VERSION;

        // save messages as JSON
        static void save_notes();

        // Message specific fields
        int id;
        std::string title;
        std::string message;
        std::string date;
};

#endif
