#include <iostream>
#include <ctime>
#include <vector>
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <sys/ioctl.h>
#include <unistd.h>

#include "note.h"

using namespace std;

const string Note::NOTE_TAKER_INFO = 
    "NoteTaker\n"
    "---------\n"
    "Simple note taking app in C++/Boost\n";

const float Note::VERSION = 0.1;

// Define location to save file, default in home dir
//  For testing
const string Note::NOTES_FILE = "notes.json";
// This may better done in a separate function
// const string Note::NOTES_FILE = string(getenv("HOME")) + "/" + 
// ".notes.json";

// http://stackoverflow.com/a/20256365
vector<Note *> Note::NoteList;

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
    ;
    po::variables_map args;
    po::store(
        po::parse_command_line(argc, argv, desc),
        args
    );
    po::notify(args);    

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
        new Note(title, message);
    }

    // if no args, list notes
    if (argc == 1)
        print_all_notes();
}

// Create a temporary file and put message in it
void Note::create_tmp_file(string tmp_file) {
    ofstream outfile(tmp_file.c_str());
    outfile << "Enter your message here..." << endl;
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

string Note::get_tmp_message() {

    // Create a tempfile path
    boost::filesystem::path temp = boost::filesystem::unique_path(
            "/tmp/notetaker_%%%%_%%%%.txt");
    string tmp_file = temp.native();

    // Open and write to temporary file
    create_tmp_file(tmp_file);

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
    cout << "Destroying note with title: " << title << endl;
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
            new Note(title, message, date, id, false);
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
        cout << "*note_p is " << *note_p << endl;
        delete *note_p;
    }
    pt.add_child("Notes", notes);
    write_json(NOTES_FILE, pt);
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
}
