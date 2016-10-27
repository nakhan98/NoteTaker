#include <iostream>
#include <ctime>
#include <vector>
#include <stdio.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <cstdlib>
#include <stdlib.h>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include "note.h"

using namespace std;

// Define location to save file
const string Note::NOTES_FILE = "./notes.json";

// http://stackoverflow.com/a/20256365
vector<Note *> Note::NoteList;

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
        editor = string("nano");
        
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
Note::Note(string title_, string message_, int id_, bool new_message) {
    if (new_message) 
        load_notes();

    if (!id_)
        id = get_id();
    else
        id = id_;
    title = title_;
    message = message_;
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
            new Note(title, message, id, false);
        }
    }
}

// Setters
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
        notes.push_back(make_pair("", note));
        cout << "*note_p is " << *note_p << endl;
        delete *note_p;
    }
    pt.add_child("Notes", notes);
    write_json(NOTES_FILE, pt);
}

//print all notes
void Note::print_all_notes() {
    using boost::format;
    load_notes();
    cout << "Title\tMessage" << endl;
    cout << "--------------" << endl;
    for (vector<Note *>::iterator note_p = Note::NoteList.begin();
            note_p != Note::NoteList.end(); ++note_p) {
        // We want the first line of the message
        // This may not be efficient - https://studiofreya.com/cpp/boost/a-few-boostformat-examples/
        format title_message = format("%-40s%-60s") % (*note_p)->title % (*note_p)->message;
        cout << title_message << endl;
    }
}
