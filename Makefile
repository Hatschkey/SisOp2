all: server.o client.o
	g++ server.o Server.o -o server -lpthread -Wall
	g++ client.o Client.o -o client -lpthread -Wall
	
server.o: server.cpp Server.cpp Server.h data_types.h constants.h
	g++ -c server.cpp Server.cpp Server.h data_types.h constants.h -Wall

client.o: client.cpp Client.cpp Client.h data_types.h constants.h
	g++ -c client.cpp Client.cpp Client.h data_types.h constants.h -Wall

clean:
	rm *.o *.gch server client *.hist
