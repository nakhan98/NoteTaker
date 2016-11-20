#include <iostream>
#include <ctime>
#include <vector>
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdexcept>
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

// Define location to save file, default in home dir
//  For testing
string Note::NOTES_FILE = "notes.json";
// This may better done in a separate function
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
        ("show,s", po::value< vector<int> >(), "Show a note")
        ("profile,p", po::value< vector<string> >(), "Specify a profile")
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

    // Set profile path
    if (args.count("profile")) {
        vector<string> input = args["profile"].as< vector<string> >();
        BOOST_LOG_TRIVIAL(debug) << "process_args: profile path has been " <<
            "specified: " << input[0];
        load_profile(input[0]);

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
    else if (args.count("show")) {
        load_notes();
        vector<int> input = args["show"].as< vector<int> >();
        show_note(input[0]);
    }

    // if no relevant args, list notes
    if (!(args.count("add") || args.count("edit") || args.count("show") ||
                args.count("list") || args.count("help") ||
                args.count("version"))) {
        BOOST_LOG_TRIVIAL(debug) << "process_args: listing notes";
        print_all_notes();
    }
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
    remove(tmp_file.c_str());
}

string Note::get_tmp_message(string note_message) {

    // Check of a message was passed
    if (note_message == "")
        note_message = ADD_NOTE_MSG; 

    // Create a tempfile path
    boost::filesystem::path temp = boost::filesystem::unique_path(
            "/tmp/notetaker_%%%%_%%%%.txt");
    string tmp_file = temp.native();

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
    BOOST_LOG_TRIVIAL(debug) << "Destroying note with title: " << title;
}

// Delete notes in memory
void Note::destroy_all_notes() {
    for (vector<Note *>::iterator note_p = Note::NoteList.begin();
            note_p != Note::NoteList.end(); ++note_p)
        delete *note_p;
}

//get an id of message
int Note::get_id() {
    int id;
    int size_of_note_list = NoteList.size();
    if (!size_of_note_list)
        id = 1;
    else {
        id  = NoteList.back()->id;
        id++;
    }
    return id;
}


void Note::load_notes() {
    if (boost::filesystem::exists(NOTES_FILE)) {
    //if (std::ifstream(NOTES_FILE.c_str())) {
        using boost::property_tree::ptree;
        ptree pt;
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
        BOOST_LOG_TRIVIAL(debug) << "*note_p is " << *note_p;
        //delete *note_p;
    }
    pt.add_child("Notes", notes);
    write_json(NOTES_FILE, pt);
    destroy_all_notes();
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
        for (vector<Note *>::iterator note_p = Note::NoteList.begin();
                note_p != Note::NoteList.end(); ++note_p) {
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
