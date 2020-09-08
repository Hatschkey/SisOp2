CC = g++

SRC_DIR := src/
OBJ_DIR := obj/
BIN_DIR := bin/
INC_DIR := include/

all: ${OBJ_DIR}User.o ${OBJ_DIR}Group.o ${OBJ_DIR}serverApp.o ${OBJ_DIR}clientApp.o
	${CC} serverApp.o Server.o User.o Group.o -o ${BIN_DIR}server -lpthread -Wall
	${CC} clientApp.o Client.o -o ${BIN_DIR}client -lpthread -Wall
	
${OBJ_DIR}serverApp.o: ${SRC_DIR}serverApp.cpp ${SRC_DIR}Server.cpp ${INC_DIR}Server.h ${INC_DIR}data_types.h ${INC_DIR}constants.h
	${CC} -c ${SRC_DIR}serverApp.cpp ${SRC_DIR}Server.cpp -I ${INC_DIR} -Wall
	
${OBJ_DIR}clientApp.o: ${SRC_DIR}clientApp.cpp ${SRC_DIR}Client.cpp ${INC_DIR}Client.h ${INC_DIR}data_types.h ${INC_DIR}constants.h
	${CC} -c ${SRC_DIR}clientApp.cpp ${SRC_DIR}Client.cpp -I ${INC_DIR} -Wall

${OBJ_DIR}Group.o:
	${CC} -c ${SRC_DIR}Group.cpp -I ${INC_DIR} -Wall

${OBJ_DIR}User.o:
	${CC} -c ${SRC_DIR}User.cpp -I ${INC_DIR} -Wall

clean:	
	rm *.o ${INC_DIR}*.gch ${BIN_DIR}server ${BIN_DIR}client ${BIN_DIR}*.hist
