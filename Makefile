all: server.o client.o
	g++ server.o -o server -lpthread 
	g++ client.o Client.o -o client -lpthread -Wall
	
server.o: server.cpp structures.h
	g++ -c server.cpp structures.h 

client.o: client.cpp Client.cpp Client.h data_types.h constants.h
	g++ -c client.cpp Client.cpp Client.h data_types.h constants.h -Wall

clean:
	rm *.o *.gch server client *.hist
