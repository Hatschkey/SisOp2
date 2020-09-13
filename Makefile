CC = g++

SRC_DIR := src/
OBJ_DIR := obj/
BIN_DIR := bin/
INC_DIR := include/

all: dirs client server

server: RW_Monitor.o User.o Group.o serverApp.o
	${CC} ${OBJ_DIR}serverApp.o ${OBJ_DIR}Server.o ${OBJ_DIR}RW_Monitor.o ${OBJ_DIR}User.o ${OBJ_DIR}Group.o -o ${BIN_DIR}server -lpthread -Wall    

client: clientApp.o
	${CC} ${OBJ_DIR}clientApp.o ${OBJ_DIR}Client.o -o ${BIN_DIR}client -lpthread -Wall
	
serverApp.o: Server.o ${SRC_DIR}serverApp.cpp ${INC_DIR}data_types.h ${INC_DIR}constants.h
	${CC} -c ${SRC_DIR}serverApp.cpp -I ${INC_DIR} -o ${OBJ_DIR}serverApp.o -Wall
	
clientApp.o: Client.o ${SRC_DIR}clientApp.cpp ${INC_DIR}data_types.h ${INC_DIR}constants.h
	${CC} -c ${SRC_DIR}clientApp.cpp -I ${INC_DIR} -o ${OBJ_DIR}clientApp.o -Wall

Server.o: ${SRC_DIR}Server.cpp ${INC_DIR}Server.h ${INC_DIR}data_types.h ${INC_DIR}constants.h
	${CC} -c ${SRC_DIR}Server.cpp -I ${INC_DIR} -o ${OBJ_DIR}Server.o -Wall

Client.o: ${SRC_DIR}Client.cpp ${INC_DIR}Client.h ${INC_DIR}data_types.h ${INC_DIR}constants.h
	${CC} -c ${SRC_DIR}Client.cpp -I ${INC_DIR} -o ${OBJ_DIR}Client.o -Wall

Group.o:
	${CC} -c ${SRC_DIR}Group.cpp -I ${INC_DIR} -o ${OBJ_DIR}Group.o -Wall

User.o:
	${CC} -c ${SRC_DIR}User.cpp -I ${INC_DIR} -o ${OBJ_DIR}User.o -Wall

RW_Monitor.o:
	${CC} -c ${SRC_DIR}RW_Monitor.cpp -I ${INC_DIR} -o ${OBJ_DIR}RW_Monitor.o -Wall

dirs:
	mkdir -p ${OBJ_DIR}
	mkdir -p ${BIN_DIR}

clean:	
	rm ${OBJ_DIR}*.o ${BIN_DIR}server ${BIN_DIR}client ${BIN_DIR}*.hist
