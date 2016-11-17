notetaker: main.cpp note.cpp note.h
	g++ -DBOOST_LOG_DYN_LINK -lboost_date_time -lboost_filesystem -lboost_system -lboost_program_options -lpthread -lboost_log -Wall -o notetaker main.cpp note.cpp note.h

debug: clean
	g++ -DBOOST_LOG_DYN_LINK -lboost_date_time -lboost_filesystem -lboost_system -lboost_program_options -lpthread -lboost_log -Wall -g -o notetaker main.cpp note.cpp note.h

clean:
	rm -vf notetaker note.h.gch a.out 
