#include <string>
#include <vector>

#ifndef NOTE_H
#define NOTE_H


class Note {
    public:
        static std::vector<Note *> NoteList;
        static Note* create_note(std::string title, std::string message, std::string date="",
                int id=0, bool new_message=true);
        ~Note();

        // Set log level to debug
        static void enable_debugging();

        //Set log level to info
        static void disable_debugging();

        // Load profile
        static void load_profile(std::string path="");

        // Encrypt profile
        static void encrypt_profile();

        // Arg parsing
        static void process_args(int argc, char **argv);

        // retrieve Note
        static Note* get_note(int id);

        // edit note
        static void edit_note(int id);

        // show note
        static void show_note(int id);

        //get_message
        static std::string get_tmp_message(std::string message="");

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

        //Encryption-related functions
        static bool check_if_profile_encrypted();
        static std::string decrypt_profile();
        static void write_encrypted_profile(std::string profile);
        template <class T> static std::string get_gpg_pass(T prompt);

        // Learning templates - ignore
        template<class Tee> static void learn_templates(Tee y); 

        // Get $UID
        static std::string get_uid();

        // Create app temp directory
        static void create_app_tmp_dir(std::string path); 

        /**
         * Run cmd and return exit code and stdout output via reference
         * See:
         * https://www.jeremymorgan.com/tutorials/c-programming/how-to-capture-the-output-of-a-linux-command-in-c/
         */
        static void run_cmd(std::string cmd, int& exit_code, std::string&
                stdout_);

        // Create temp dir and set s_temp_dir
        static void create_temp_dir();


    private:
        // Generic info 
        static const std::string NOTE_TAKER_INFO;

        // File to save notes in 
        static std::string NOTES_FILE; // this is not a constant, should be lower case

        // Version number
        static const float VERSION;

        // Default message in temp file when adding note
        static const std::string ADD_NOTE_MSG;

        // GPG decryption command
        static const std::string GPG_DECRYPTION_CMD;

        // GPG encryption command
        static const std::string GPG_ENCRYPTION_CMD;

        // Command to get $UID
        static const std::string GET_UID; 
      
        // Command to check if /run/user/$(id -u) is present
        static const std::string CHECK_USER_TMPFS; 

        // File to store passwords
        static const std::string PASSWORDS_FILE;

        // Field signifies whether profile is encrypted
        static bool s_profile_encrypted;

        // Temporary directory
        static std::string s_temp_dir;

        // GPG header
        static const std::string GPG_HEADER;

        // save messages as JSON
        static void save_notes();

        // Private consutructor so we can only allocate on heap
        Note(std::string title, std::string message, std::string date,
                int id, bool new_message);

        // Message specific fields
        int id;
        std::string title;
        std::string message;
        std::string date;
};

#endif
