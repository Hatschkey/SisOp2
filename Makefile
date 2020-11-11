CC = g++

SRC := src/
OBJ := obj/
BIN := bin/
INC := include/
HIST := bin/hist/

all: dirs client server replica

replica: RW_Monitor Session User Group replicaApp CommunicationUtils
	${CC} ${OBJ}replicaApp.o ${OBJ}ReplicaManager.o ${OBJ}RW_Monitor.o ${OBJ}Session.o ${OBJ}User.o ${OBJ}Group.o ${OBJ}CommunicationUtils.o -o ${BIN}replica -lpthread -Wall

server: RW_Monitor Session User Group CommunicationUtils serverApp
	${CC} ${OBJ}serverApp.o ${OBJ}Server.o ${OBJ}RW_Monitor.o ${OBJ}Session.o ${OBJ}User.o ${OBJ}Group.o ${OBJ}CommunicationUtils.o -o ${BIN}server -lpthread -Wall

client: ClientInterface CommunicationUtils RW_Monitor Client clientApp ElectionListener
	${CC} ${OBJ}ClientInterface.o ${OBJ}clientApp.o ${OBJ}Client.o ${OBJ}CommunicationUtils.o ${OBJ}RW_Monitor.o ${OBJ}ElectionListener.o -o ${BIN}client -lncurses -lpthread -Wall
	
replicaApp: ReplicaManager
	${CC} -c ${SRC}replicaApp.cpp -I ${INC} -o ${OBJ}replicaApp.o -Wall

serverApp: Server
	${CC} -c ${SRC}serverApp.cpp -I ${INC} -o ${OBJ}serverApp.o -Wall
	
clientApp: Client
	${CC} -c ${SRC}clientApp.cpp -I ${INC} -o ${OBJ}clientApp.o -Wall

Server:
	${CC} -c ${SRC}Server.cpp -I ${INC} -o ${OBJ}Server.o -Wall

Client:
	${CC} -c ${SRC}Client.cpp -I ${INC} -o ${OBJ}Client.o -Wall

Group:
	${CC} -c ${SRC}Group.cpp -I ${INC} -o ${OBJ}Group.o -Wall

User:
	${CC} -c ${SRC}User.cpp -I ${INC} -o ${OBJ}User.o -Wall

Session:
	${CC} -c ${SRC}Session.cpp -I ${INC} -o ${OBJ}Session.o -Wall

RW_Monitor:
	${CC} -c ${SRC}RW_Monitor.cpp -I ${INC} -o ${OBJ}RW_Monitor.o -Wall

ClientInterface:
	${CC} -c ${SRC}ClientInterface.cpp -I ${INC} -o ${OBJ}ClientInterface.o -Wall

CommunicationUtils:
	${CC} -c ${SRC}CommunicationUtils.cpp -I ${INC} -o ${OBJ}CommunicationUtils.o -Wall

ReplicaManager:
	${CC} -c ${SRC}ReplicaManager.cpp -I ${INC} -o ${OBJ}ReplicaManager.o -Wall

ElectionListener:
	${CC} -c ${SRC}ElectionListener.cpp -I ${INC} -o ${OBJ}ElectionListener.o -Wall

dirs:
	mkdir -p ${OBJ}
	mkdir -p ${BIN}
	mkdir -p ${HIST}

clean:	
	rm ${OBJ}*.o ${BIN}server ${BIN}client ${BIN}replica ${HIST}*.hist

run_server: ${BIN}server
	cd ${BIN} && ./server 50

run_client: ${BIN}client
	cd ${BIN} && ./client user group localhost 6789
