all: serverApp.o clientApp.o
	g++ serverApp.o Server.o -o server -lpthread -Wall
	g++ clientApp.o Client.o -o client -lpthread -Wall
	
serverApp.o: serverApp.cpp Server.cpp Server.h data_types.h constants.h
	g++ -c serverApp.cpp Server.cpp Server.h data_types.h constants.h -Wall

clientApp.o: clientApp.cpp Client.cpp Client.h data_types.h constants.h
	g++ -c clientApp.cpp Client.cpp Client.h data_types.h constants.h -Wall

clean:
	rm *.o *.gch server client *.hist
