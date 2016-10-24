#include <string>
#include <vector>

#ifndef NOTE_H
#define NOTE_H

const std::string NOTE_TAKER_HELP = "This is the notetaker help!";

class Note {
    public:
        static std::vector<Note *> NoteList;
        Note(std::string title, std::string message, bool new_message=true);
        ~Note();

		//get_message
		static std::string get_tmp_message();

        // Setters
        std::string get_title();
        std::string get_message();

        void load_notes();

    private:
        // File to save notes in 
        static const std::string NOTES_FILE;

        // save messages as JSON
        static void save_notes();

        // Message specific fields
        std::string title;
        std::string message;
        std::string date;
};

#endif
