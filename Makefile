notetaker: main.cpp note.cpp note.h
	g++ -lboost_filesystem  -lboost_system -Wall -o notetaker main.cpp note.cpp note.h

debug: clean
	g++ -lboost_filesystem  -lboost_system -Wall -g -o notetaker main.cpp note.cpp note.h

clean:
	rm -vf notetaker note.h.gch a.out 
