/* 
 Note taker app
 --------------
 Simple note taking app in C++/Boost

 */

#include <iostream>
#include "note.h"

using namespace std;

int main(int argc, char **argv) {
    Note::process_args(argc, argv);    
    return 0;
}
