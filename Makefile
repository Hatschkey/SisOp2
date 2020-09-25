CC = g++

SRC_DIR := src/
OBJ_DIR := obj/
BIN_DIR := bin/
INC_DIR := include/
HIST_DIR := bin/hist/

all: dirs ${BIN_DIR}client ${BIN_DIR}server

${BIN_DIR}server: ${OBJ_DIR}RW_Monitor.o ${OBJ_DIR}User.o ${OBJ_DIR}Group.o ${OBJ_DIR}serverApp.o ${OBJ_DIR}CommunicationUtils.o
	${CC} ${OBJ_DIR}serverApp.o ${OBJ_DIR}Server.o ${OBJ_DIR}RW_Monitor.o ${OBJ_DIR}User.o ${OBJ_DIR}Group.o ${OBJ_DIR}CommunicationUtils.o -o ${BIN_DIR}server -lpthread -Wall

${BIN_DIR}client: ${OBJ_DIR}ClientInterface.o ${OBJ_DIR}clientApp.o ${OBJ_DIR}CommunicationUtils.o
	${CC} ${OBJ_DIR}ClientInterface.o ${OBJ_DIR}clientApp.o ${OBJ_DIR}Client.o ${OBJ_DIR}CommunicationUtils.o -o ${BIN_DIR}client -lncurses -lpthread -Wall
	
${OBJ_DIR}serverApp.o: ${OBJ_DIR}Server.o ${SRC_DIR}serverApp.cpp ${INC_DIR}data_types.h ${INC_DIR}constants.h
	${CC} -c ${SRC_DIR}serverApp.cpp -I ${INC_DIR} -o ${OBJ_DIR}serverApp.o -Wall
	
${OBJ_DIR}clientApp.o: ${OBJ_DIR}Client.o ${SRC_DIR}clientApp.cpp ${INC_DIR}data_types.h ${INC_DIR}constants.h
	${CC} -c ${SRC_DIR}clientApp.cpp -I ${INC_DIR} -o ${OBJ_DIR}clientApp.o -Wall

${OBJ_DIR}Server.o: ${SRC_DIR}Server.cpp ${INC_DIR}Server.h ${INC_DIR}data_types.h ${INC_DIR}constants.h
	${CC} -c ${SRC_DIR}Server.cpp -I ${INC_DIR} -o ${OBJ_DIR}Server.o -Wall

${OBJ_DIR}Client.o: ${SRC_DIR}Client.cpp ${INC_DIR}Client.h ${INC_DIR}data_types.h ${INC_DIR}constants.h
	${CC} -c ${SRC_DIR}Client.cpp -I ${INC_DIR} -o ${OBJ_DIR}Client.o -Wall

${OBJ_DIR}Group.o:
	${CC} -c ${SRC_DIR}Group.cpp -I ${INC_DIR} -o ${OBJ_DIR}Group.o -Wall

${OBJ_DIR}User.o:
	${CC} -c ${SRC_DIR}User.cpp -I ${INC_DIR} -o ${OBJ_DIR}User.o -Wall

${OBJ_DIR}RW_Monitor.o:
	${CC} -c ${SRC_DIR}RW_Monitor.cpp -I ${INC_DIR} -o ${OBJ_DIR}RW_Monitor.o -Wall

${OBJ_DIR}ClientInterface.o: ${SRC_DIR}ClientInterface.cpp ${INC_DIR}ClientInterface.h
	${CC} -c ${SRC_DIR}ClientInterface.cpp -I ${INC_DIR} -o ${OBJ_DIR}ClientInterface.o -Wall

${OBJ_DIR}CommunicationUtils.o: ${SRC_DIR}CommunicationUtils.cpp ${INC_DIR}CommunicationUtils.h
	${CC} -c ${SRC_DIR}CommunicationUtils.cpp -I ${INC_DIR} -o ${OBJ_DIR}CommunicationUtils.o -Wall

dirs:
	mkdir -p ${OBJ_DIR}
	mkdir -p ${BIN_DIR}
	mkdir -p ${HIST_DIR}

clean:	
	rm ${OBJ_DIR}*.o ${BIN_DIR}server ${BIN_DIR}client ${HIST_DIR}*.hist

run_server: ${BIN_DIR}server
	cd ${BIN_DIR} && ./server 50

run_client: ${BIN_DIR}client
	cd ${BIN_DIR} && ./client user group 127.0.0.1 6789
