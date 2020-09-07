all: User.o Group.o serverApp.o clientApp.o
	g++ serverApp.o Server.o User.o Group.o -o ./bin/server -lpthread -Wall
	g++ clientApp.o Client.o -o ./bin/client -lpthread -Wall
	
serverApp.o: ./src/serverApp.cpp ./src/Server.cpp ./src/Server.h ./src/data_types.h ./src/constants.h
	g++ -c ./src/serverApp.cpp ./src/Server.cpp ./src/Server.h ./src/data_types.h ./src/constants.h -Wall

clientApp.o: ./src/clientApp.cpp ./src/Client.cpp ./src/Client.h ./src/data_types.h ./src/constants.h
	g++ -c ./src/clientApp.cpp ./src/Client.cpp ./src/Client.h ./src/data_types.h ./src/constants.h -Wall

Group.o:
	g++ -c ./src/Group.cpp ./src/Group.h ./src/data_types.h ./src/constants.h -Wall

User.o:
	g++ -c ./src/User.cpp ./src/User.h ./src/data_types.h ./src/constants.h -Wall	

clean:
	rm *.o ./src/*.gch ./bin/server ./bin/client ./bin/*.hist
