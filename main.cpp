/* 
 Note taker app
 --------------
 Simple note taking app in C++/Boost

 */

#include <iostream>
#include <cstring>
#include "note.h"

using namespace std;

int main(int argc, char **argv) {
    if (argc == 2) {
        if (!strcmp(argv[1], "-a") || !strcmp(argv[1], "--add")) {
            string title, message;
            cout << "Enter a title for your message: ";
            //cin >> title;
            getline(cin, title);
            cout << "Enter your message: ";
            //cin >> message;
            getline(cin, message);
            //Note n1(title, message);
            new Note(title, message);
            Note::get_tmp_message();

        }
        else if (!strcmp(argv[1], "-l") || !strcmp(argv[1], "--list"))
            cout << "You want to list all notes" << endl;
    }
    else 
        cout << NOTE_TAKER_HELP << endl;

    return 0;
}
