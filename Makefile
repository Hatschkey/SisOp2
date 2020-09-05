all: server.o client.o
	g++ server.o -o server -lpthread
	g++ client.o -o client -lpthread
	
server.o: server.cpp structures.h
	g++ -c server.cpp structures.h 

client.o: client.cpp structures.h
	g++ -c client.cpp structures.h 

clean:
	rm *.o *.gch server client
