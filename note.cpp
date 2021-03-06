#include <iostream>
#include <ctime>
#include <vector>
#include <algorithm> 
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdexcept>
#include <termios.h>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/core.hpp>

#include "note.h"

using namespace std;

const string Note::NOTE_TAKER_INFO = 
    "NoteTaker\n"
    "---------\n"
    "Simple note taking app in C++/Boost\n";

const string Note::ADD_NOTE_MSG = "Enter your message here...";

const float Note::VERSION = 0.1;

const string Note::GPG_HEADER = "-----BEGIN PGP MESSAGE-----";

const string Note::GPG_DECRYPTION_CMD = "gpg --output - --passphrase-file %s %s";

const string Note::GPG_ENCRYPTION_CMD = "gpg --output %s --yes --symmetric "
    "--armour --passphrase-file %s %s";

const std::string Note::CHECK_USER_TMPFS = "ls -l /run/user/$(id -u)";

const std::string Note::GET_UID = "id -u";

const std::string Note::PASSWORD_FILE = ".x768bbbgi";

bool Note::s_profile_encrypted = false;

std::string Note::s_temp_dir;

// Define location to save file, default in home dir
//  For testing
string Note::NOTES_FILE = "notes.json";
// const string Note::NOTES_FILE = string(getenv("HOME")) + "/" + 
// ".notes.json";

// http://stackoverflow.com/a/20256365
vector<Note *> Note::NoteList;

// Set log level to info
void Note::disable_debugging() {
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::info
    );
}

// Set log level to debug
void Note::enable_debugging() {
    namespace logging = boost::log;
    logging::core::get()->set_filter
    (
        logging::trivial::severity >= logging::trivial::debug
    );
}

// Arg parsing
void Note::process_args(int argc, char **argv) {
    /* See:
     * http://www.boost.org/doc/libs/1_58_0/doc/html/program_options/tutorial.html
     * https://chuckaknight.wordpress.com/2013/03/24/boost-command-line-argument-processing/
     * https://www.nu42.com/2015/05/cpp-command-line-arguments-with-boost.html
     */
    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Show brief usage message")
        ("version", "Show version number")
        ("list,l", "List all notes")
        ("add,a", "Add a note")
        ("edit,e", po::value< vector<int> >(), "Edit a note")
        ("delete,d", po::value< vector<int> >(),"Delete a note")
        ("show,s", po::value< vector<int> >(), "Show a note")
        ("profile,p", po::value< vector<string> >(), "Specify a profile")
        ("encrypt", "Encrypt profile")
        ("debug", "Turn debugging on")
    ;
    po::variables_map args;
    po::store(
        po::parse_command_line(argc, argv, desc),
        args
    );
    po::notify(args);    

    // Set log level
    if (args.count("debug"))
        enable_debugging();
    else
        disable_debugging();

    // Create and specify temp dir
    create_temp_dir();

    // Set profile path
    if (args.count("profile")) {
        vector<string> input = args["profile"].as< vector<string> >();
        BOOST_LOG_TRIVIAL(debug) << "process_args: profile path has been " <<
            "specified: " << input[0];
        load_profile(input[0]);

    }
    else {
        BOOST_LOG_TRIVIAL(debug) << "process_args: using default profile - " <<
            NOTES_FILE;
    }

    if (args.count("help"))
        cout << NOTE_TAKER_INFO << endl << desc << endl;
    else if (args.count("version"))
        cout << "NoteTaker version " << VERSION << endl;
    else if (args.count("list"))
        print_all_notes();
    else if (args.count("add")) {
        string title, message;
        cout << "Enter a title for your message: ";
        getline(cin, title);
        message = Note::get_tmp_message();
        create_note(title, message);
    }
    else if (args.count("edit")) {
        load_notes();
        vector<int> input = args["edit"].as< vector<int> >();
        edit_note(input[0]);
    }
    else if (args.count("delete")) {
        load_notes();
        vector<int> input = args["delete"].as< vector<int> >();
        delete_note(input[0]);
        save_notes();
    }
    else if (args.count("show")) {
        load_notes();
        vector<int> input = args["show"].as< vector<int> >();
        show_note(input[0]);
    }

    if (args.count("encrypt")) {
        BOOST_LOG_TRIVIAL(debug) << "process_args: encrypt profile";
        encrypt_profile();
    }

    // if no relevant args, list notes
    if (!(args.count("add") || args.count("edit") || args.count("delete") ||
                args.count("show") || args.count("list") || args.count("help")
                || args.count("version") || args.count("encrypt"))) {
        BOOST_LOG_TRIVIAL(debug) << "process_args: listing notes";
        print_all_notes();
    }

    // Cleanup
    do_cleanup();
}

/**
 * Create temp dir and set s_temp_dir
 *
 * If /run/user/$UID does not exist use /tmp/ for temporary files (which may
 * be insecure in multi-user system when dealing with encrypted profiles).
 * See - https://www.freedesktop.org/software/systemd/man/pam_systemd.html
 */
void Note::create_temp_dir() {
    using boost::format;
    int exit_code;
    string output;
    string tmp_dir;
    run_cmd(CHECK_USER_TMPFS, exit_code, output);

    if (exit_code != 0) {
        cout << "Warning: Running app in insecure mode. " << 
            "Dot not rely upon for strong encryption!" << endl;
        tmp_dir = "/tmp/NoteTaker/";
        create_app_tmp_dir(tmp_dir);
    }
    else {
        // Get $UID
        string uid = get_uid();

        // Path for tempfile 
        tmp_dir = "/run/user/" + uid + "/NoteTaker/";
        create_app_tmp_dir(tmp_dir);
    }
    BOOST_LOG_TRIVIAL(debug) << "create_temp_dir: setting s_temp_dir to " <<
        tmp_dir;
    s_temp_dir = tmp_dir;
}

// Ask for password
// Taken from http://stackoverflow.com/a/6899073/7142682
template <class T>
string Note::get_gpg_pass(T prompt) {
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    cout << prompt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    string password;
    getline(cin, password);

    BOOST_LOG_TRIVIAL(debug) << "get_gpg_pass: entered password is: " <<
        password;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return password;
}

string Note::get_passwd_file_path() {
    return s_temp_dir + "/" + PASSWORD_FILE;
}

void Note::get_and_save_passwd_to_file(string prompt) {
    string passwd = get_gpg_pass(prompt);
    string passwd_file_path = get_passwd_file_path();
    ofstream outfile(passwd_file_path.c_str());
    outfile << passwd << endl;
    outfile.close();
}

void Note::do_cleanup() {
    BOOST_LOG_TRIVIAL(debug) << "do_cleanup:...";
    delete_passwd_file();
}

void Note::delete_passwd_file() {
    string passwd_file = s_temp_dir + "/" + PASSWORD_FILE;
    if (boost::filesystem::exists(passwd_file)) {
        BOOST_LOG_TRIVIAL(debug) << "delete_passwd_file: Deleting PASSWORD_FILE: " << passwd_file;
        remove(passwd_file.c_str());
    }
}

// Trying to learn templates - ignore
template<>
void Note::learn_templates<int>(int x) {
    // Testing templates
    cout << "You specified: " << x << endl;
}

// Trying to learn templates - ignore
template<>
void Note::learn_templates<const char *>(const char* x) {
    // Testing templates
    cout << "You specified: " << x << endl;
}

void Note::encrypt_profile() {
    BOOST_LOG_TRIVIAL(debug) << "encrypt_profile: loading notes";
    load_notes();
    BOOST_LOG_TRIVIAL(debug) << "encrypt_profile: getting new password";
    get_and_save_passwd_to_file("Enter new password: ");
    BOOST_LOG_TRIVIAL(debug) << "encrypt_profile: saving notes";
    save_notes();
}

void Note::load_profile(string path) {
    if (path != "") {
        BOOST_LOG_TRIVIAL(debug) << "load_profile: Changing profile path: "
            << path;
        NOTES_FILE = path;
    }
}

void Note::show_note(int id) {
    using boost::format;
    Note * note;
    try
    {
        note = get_note(id);
    }
    catch (runtime_error err)
    {
        cout << "Error: " << err.what() << endl;
        destroy_all_notes();
        exit(1);
    }
    cout << format("Title: %s\nBody: %s") % note->title % note->message
        << endl;
    destroy_all_notes();
}

// Edit a note
void Note::edit_note(int id) {
    using boost::format;
    // Try-catch errors - http://stackoverflow.com/a/26171850/7142682
    Note * note; 
    try 
    {
        note = get_note(id);
    }
    catch (runtime_error err)
    {
        cout << "Error: " << err.what() << endl;
        destroy_all_notes();
        exit(1);
    }
    // Old message details
    string old_title = note->title;
    string old_msg = note->message;

    // Get new message details
    string new_title, new_msg;
    format get_title_msg = format("Enter a title for your message[%s]: ") % old_title;
    cout << get_title_msg;
    getline(cin, new_title);
    //boost::algorithm::trim(new_title); To-do: check that title is not whitespace
    if (new_title == "")
        new_title = old_title;

    new_msg = get_tmp_message(old_msg);

    // Save note
    note->title = new_title;
    note->message = new_msg;
    save_notes();
}

// Delete a note
void Note::delete_note(int id) {
    bool found_note = false;
    int index = 0;
    for (vector<Note *>::iterator note_p = NoteList.begin();
            note_p != NoteList.end(); ++note_p) {
        if ((*note_p)->id == id) {
            BOOST_LOG_TRIVIAL(debug) << "delete_note: Found note with id: " << id;
            found_note = true;
            break;
        }
        index++;
    }

    if (found_note){
        BOOST_LOG_TRIVIAL(debug) << "delete_note: Deleting note with id: " <<
            id;
        NoteList.erase(NoteList.begin()+index);
    }
    else {
        BOOST_LOG_TRIVIAL(debug) << "delete_note: Did not find note with id: "
            << id;
    }
}

// Retrieve a Note
// Throw runtime error if note is not found
Note*  Note::get_note(int id) {
    for (vector<Note *>::iterator note_p = Note::NoteList.begin();
            note_p != Note::NoteList.end(); ++note_p) {
        if ((*note_p)->id == id) {
            BOOST_LOG_TRIVIAL(debug) << "Found note with id: " << id;
            return *note_p;
        }
    }
    string error_message = "Note not found with id - " + boost::lexical_cast<string>(id);
    throw std::runtime_error(error_message);
}


// Create a temporary file and put message in it
void Note::create_tmp_file(string tmp_file, string message) {
    ofstream outfile(tmp_file.c_str());
    outfile << message << endl;
    outfile.close();
}

string Note::get_default_editor() {
    string editor;
    char * default_editor;

    if ( (default_editor = getenv("EDITOR")) )
        editor = string(default_editor);
    else if ( (default_editor = getenv("VISUAL")) )
        editor = string(default_editor);
    else
        editor = "nano";
        
    return editor;
}

void Note::open_file_in_editor(string editor, string file) {
    using boost::format;
    format cmd = format("%1% %2%") % editor % file;
    const char * command = cmd.str().c_str();
    system(command);
}

string Note::read_tmp_file(string tmp_file) {
    std::ifstream input(tmp_file.c_str());
    std::stringstream sstr;
    while(input >> sstr.rdbuf());
    return sstr.str();
}

void Note::delete_tmp_file(string tmp_file) {
    BOOST_LOG_TRIVIAL(debug) << "delete_tmp_file: Deleting: " << tmp_file;
    remove(tmp_file.c_str());
}

string Note::get_tmp_message(string note_message) {

    // Check of a message was passed
    if (note_message == "")
        note_message = ADD_NOTE_MSG; 

    // Create a tempfile path
    string tmp_file = s_temp_dir + "/notetaker_%%%%_%%%%.txt";
    boost::filesystem::path temp = boost::filesystem::unique_path(
            "/tmp/notetaker_%%%%_%%%%.txt");
    tmp_file = temp.native();
    BOOST_LOG_TRIVIAL(debug) << "get_tmp_message: tmp_file: " 
        << tmp_file;

    // Open and write to temporary file
    create_tmp_file(tmp_file, note_message);

    // Get default editor
    string default_editor = get_default_editor();

    // Run editor against tempfile
    open_file_in_editor(default_editor, tmp_file);

    // Read tmp file
    string message = boost::algorithm::trim_copy(read_tmp_file(tmp_file));

    // Delete tmp file
    delete_tmp_file(tmp_file);

    return message;
    
}

// Method to create note (force allocation on heap)
Note * Note::create_note(string title_, string message_, string date_, int id_,
        bool new_message) {
    return new Note(title_, message_, date_, id_, new_message);
}

//Note constructor
Note::Note(string title_, string message_, string date_, int id_, bool new_message) {
    // This needs cleaning up, it is ugly!!!
    if (new_message) 
        load_notes();

    if (!id_)
        id = get_id();
    else
        id = id_;
    title = title_;
    message = message_;
    if (new_message)
        date = get_current_date();
    else 
        date = date_;
    NoteList.push_back(this);
    if (new_message) 
        save_notes();
}

Note::~Note() {
    BOOST_LOG_TRIVIAL(debug) << "~Note: Destroying note with title - " << title;
}

// Delete notes in memory
void Note::destroy_all_notes() {
    BOOST_LOG_TRIVIAL(debug) << "destroy_all_notes: destroying all notes";
    for (vector<Note *>::iterator note_p = Note::NoteList.begin();
            note_p != Note::NoteList.end(); ++note_p)
        delete *note_p;
}

// Helper functions for sorting
template<>
bool Note::sorter<int>(int x, int y) {
    return x < y;
}

template<>
bool Note::sorter<string>(string x, string y) {
    int comparison = x.compare(y);
    if (comparison < 0)
        return true;
    else
        return false;
}

// Comparator function to help sort NoteList
// See - http://stackoverflow.com/a/4892701
bool Note::sort_notes_by_id(Note* n1, Note* n2) {
    return sorter(n1->id, n2->id);
}

bool Note::sort_notes_by_title(Note* n1, Note* n2) {
    return sorter(n1->title, n2->title);
}

bool Note::sort_notes_by_date(Note* n1, Note* n2) {
    return sorter(n1->date, n2->date);
}

// Get an id of message (get first free id)
// 
int Note::get_id() {
    // Create a copy of NoteList for sorting
    vector<Note *> sorted_notes;
    for (size_t i=0; i<NoteList.size(); i++)
        sorted_notes.push_back(NoteList[i]);
    
    // sort sorted_notes
    sort(sorted_notes.begin(), sorted_notes.end(), sort_notes_by_id);

    // Find first empty id
    int id = 1;
    int size_of_note_list = sorted_notes.size();
    for (vector<Note *>::iterator note_p = sorted_notes.begin();
            note_p != sorted_notes.end(); ++note_p) {
        if ((*note_p)->id != id) {
            BOOST_LOG_TRIVIAL(debug) << "get_id: Found empty id: " <<
                id;
            break;
        }
        id++;
    }

    //Check if we iterated through all of vector without finding empty id
    if (id > size_of_note_list)
        BOOST_LOG_TRIVIAL(debug) << "get_id: Issuing new id: " << id;

    return id;
}

bool Note::check_if_profile_encrypted() {
    ifstream infile(NOTES_FILE.c_str());
    bool retval = false;
    if (infile.good()) {
        string first_line;
        getline(infile, first_line);
        BOOST_LOG_TRIVIAL(debug) << "check_if_profile_encrypted: first line of"
           << " profile - " << first_line;
        if (first_line == GPG_HEADER) {
            BOOST_LOG_TRIVIAL(debug) << "check_if_profile_encrypted: profile "
                "is encrypted";
            retval = true;
            s_profile_encrypted = true;
        }
    }
    infile.close();
    return retval;
}

// Attempt to decrypt profile (throw runtime error if return code of gpg command is not
// 0)
string Note::decrypt_profile() {
    using boost::format;
    string passwd_file_path = get_passwd_file_path();
    string gpg_decrypt_cmd = boost::str(format(GPG_DECRYPTION_CMD) %
            passwd_file_path % NOTES_FILE);
    int exit_code;
    string output;
    run_cmd(gpg_decrypt_cmd, exit_code, output);
    BOOST_LOG_TRIVIAL(debug) << "decrypt_profile: Attempting to decrypt profile - " <<
        NOTES_FILE;

    if (exit_code != 0) {
        string error_message = "decrypt_profile failed as cmd returned "
            "non-zero value: " + boost::lexical_cast<string>(exit_code);
        do_cleanup();
        throw std::runtime_error(error_message);
    }

    BOOST_LOG_TRIVIAL(debug) << "decrypt_profile: decryption was successful!";
    BOOST_LOG_TRIVIAL(debug) << "decrypt_profile: decrypted_profile is: " <<
        output;
    return output;
}

void Note::run_cmd(string cmd, int& exit_code, string& output) {
    //string data;
    FILE * stream;
    const int max_buffer = 256;
    char buffer[max_buffer];
    exit_code = 1;
    //cmd.append(" 2>&1"); Don't need stderr redirection for now
    cmd.append(" 2>/dev/null");

    BOOST_LOG_TRIVIAL(debug) << "run_cmd: preparing to run the following " <<
        "command - " << cmd;

    stream = popen(cmd.c_str(), "r");
    if (stream) {
        while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL)
                output.append(buffer);
        exit_code = pclose(stream);
    }
}

void Note::load_notes() {
    // Destroy all notes before proceeding
    BOOST_LOG_TRIVIAL(debug) << "load_notes: destroy any existing notes loaded"
        << "before loading notes";
    destroy_all_notes();

    if (boost::filesystem::exists(NOTES_FILE)) {
    //if (std::ifstream(NOTES_FILE.c_str())) {
        using boost::property_tree::ptree;
        ptree pt;
        if (check_if_profile_encrypted()) {
            // See this - http://stackoverflow.com/a/21537818
            // ask for password here and save it to password file
            cout << "Profile appears to be encrypted!" << endl;
            get_and_save_passwd_to_file();
            string decrypted_profile = decrypt_profile();
            stringstream ss;
            ss << decrypted_profile; 
            boost::property_tree::read_json(ss, pt);
        }
        else 
            boost::property_tree::read_json(NOTES_FILE, pt);

        BOOST_FOREACH( ptree::value_type& node, pt.get_child("Notes") ) {
            int id = node.second.get<int>("id", -1);
            string title = node.second.get<string>("title", "");
            string message = node.second.get<string>("message", "");
            string date = node.second.get<string>("date", "");
            create_note(title, message, date, id, false);
        }
    }
}

// Getters
string Note::get_title() {
    return title;
}

string Note::get_message() {
    return message;
}

// Save messages
// See - http://stackoverflow.com/a/12821620
void Note::save_notes() {
    using boost::property_tree::ptree;
    ptree pt, notes;
    for (vector<Note *>::iterator note_p = Note::NoteList.begin();
            note_p != Note::NoteList.end(); ++note_p) {
        ptree note;
        note.put("id", (*note_p)->id);
        note.put("title", (*note_p)->title);
        note.put("message", (*note_p)->message);
        note.put("date", (*note_p)->date);
        notes.push_back(make_pair("", note));
        BOOST_LOG_TRIVIAL(debug) << "save_notes: *note_p is " << *note_p;
        //delete *note_p;
    }
    pt.add_child("Notes", notes);

    // Check if encryption is enabled
    if (s_profile_encrypted) {
        BOOST_LOG_TRIVIAL(debug) << "save_notes: encryption is set, saving "
            "encrypted profile to " << NOTES_FILE;
        stringstream ss;
        write_json(ss, pt);
        write_encrypted_profile(ss.str());
    }
    else
        write_json(NOTES_FILE, pt);
    destroy_all_notes();
}

/* Encrypts profile
 * Requires systemd and the /run/user/$(id -u) tmpfs to be present 
 *
 * Doing it this way as don't want to write decrypted profile to disk
 * Todo 1: If no /run/user/$(id -u) fallback to using /tmp/ (insecure)
 * Todo 2: maybe use GPGME library
 */
void Note::write_encrypted_profile(string profile) {
    using boost::format;
    int exit_code;
    string output;
    string tmp_file;
    tmp_file = s_temp_dir + "/notetaker_%%%%_%%%%.profile";

    // create tmp file path
    boost::filesystem::path temp = boost::filesystem::unique_path(tmp_file);
    tmp_file = temp.native();

    //Create temp file
    create_tmp_file(tmp_file, profile);

    // construct cmd to encrypt profile 
    string gpg_encrypt_cmd = boost::str(format(GPG_ENCRYPTION_CMD) %
            NOTES_FILE % get_passwd_file_path() % tmp_file);
    
    run_cmd(gpg_encrypt_cmd, exit_code, output);

    if (exit_code != 0) {
        // throw for now (see todo)
        throw std::runtime_error("Error while trying to encrypt profile");
    }

    delete_tmp_file(tmp_file);
}

// Get $UID
// Note: class field instead?
string Note::get_uid() {
    string uid;
    int exit_code;
    run_cmd(GET_UID, exit_code, uid);
    boost::algorithm::trim(uid);
    return uid;
}

void Note::create_app_tmp_dir(string temp_dir) {
    //string temp_dir = tmp_path + "/NoteTaker"; # delete
    boost::filesystem::path path(temp_dir);
    if (boost::filesystem::create_directory(path)) {
        BOOST_LOG_TRIVIAL(debug) << "create_user_tmpfs_dir: Created temporary "
            << "directory for app: " << temp_dir;
    }
    else {
        BOOST_LOG_TRIVIAL(debug) <<
            "create_user_tmpfs_dir: Temporary directory already exists: "  <<
            temp_dir;
    }
}


//Get console width
int Note::get_console_width() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

// Calculate title width so that width of print_all_notes output dynamically
// changes in response to console width
int Note::calc_title_width(int console_width) {
    int title_width;
    if (console_width < 60)
        title_width = console_width -25;
    else
        title_width = 60;

    // cout << "Title width: " <<  title_width << endl;
    return title_width;
}

// Create row format for print_all_notes
const char * Note::create_note_row(int title_width) {
    using boost::lexical_cast;
    // "%-5s%-<title_width>s%-10s"
    string row = "%-5s%-" + lexical_cast<string>(title_width) + "s" + "%-10s";
    return row.c_str();
}

// Get current datetime
// http://stackoverflow.com/a/2493977
string Note::get_current_date() {
    namespace pt = boost::posix_time;
    pt::ptime now = pt::second_clock::local_time();
    // cout << boost::posix_time::to_simple_string(now) << endl;
    return  boost::posix_time::to_simple_string(now);
}

//print all notes
void Note::print_all_notes() {
    using boost::format;
    int title_width = calc_title_width(get_console_width());
    load_notes();
    if (!NoteList.size())
        cout << "No notes saved!" << endl;
    else {
        format title = format(create_note_row(title_width)) % "ID" % "Title" % "Date";
        cout << title << endl;
        cout << string(title_width + 5 + 20, '-') << endl;
        for (vector<Note *>::iterator note_p = NoteList.begin();
                note_p != NoteList.end(); ++note_p) {
            // We want the first line of the message
            // This may not be efficient - https://studiofreya.com/cpp/boost/a-few-boostformat-examples/
            //format title_message = format("%-5s%-60s") % (*note_p)->id % (*note_p)->title;
            format row = format(create_note_row(title_width)) % (*note_p)->id % (*note_p)->title
                % (*note_p)->date;
            cout << row << endl;
        }
    }
    destroy_all_notes();
}
