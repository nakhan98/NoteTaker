#include <iostream>
#include <ctime>
#include <vector>
#include <stdio.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include "note.h"

using namespace std;

// Define location to save file
const string Note::NOTES_FILE = "./notes.json";

// http://stackoverflow.com/a/20256365
vector<Note *> Note::NoteList;

string Note::get_tmp_message() {
	boost::filesystem::path temp = boost::filesystem::unique_path();
	string tmp_path = string("/tmp/note_taker_") + temp.native() + string(".txt");

	// Create a temporary file and put message in it
	ofstream outfile(tmp_path.c_str());
	outfile << "my text here!" << std::endl;
	outfile.close();

	return string("dummy");
	
}

//Note constructor
Note::Note(string title_, string message_, bool new_message) {
    if (new_message) 
        load_notes();
    title = title_;
    message = message_;
    NoteList.push_back(this);
    if (new_message) 
        save_notes();
}

Note::~Note() {
    cout << "Destroying note with title: " << title << endl;
}

void Note::load_notes() {
    if (boost::filesystem::exists(NOTES_FILE)) {
    //if (std::ifstream(NOTES_FILE.c_str())) {
        using boost::property_tree::ptree;
        ptree pt;
        boost::property_tree::read_json(NOTES_FILE, pt);

        BOOST_FOREACH( ptree::value_type& node, pt.get_child("Notes") ) {
            string title = node.second.get<string>("title", "");
            string message = node.second.get<string>("message", "");
            Note * note = new Note(title, message, false);
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
        note.put("title", (*note_p)->title);
        note.put("message", (*note_p)->message);
        notes.push_back(make_pair("", note));
        cout << "*note_p is " << *note_p << endl;
        delete *note_p;
    }
    pt.add_child("Notes", notes);
    write_json(NOTES_FILE, pt);
}
