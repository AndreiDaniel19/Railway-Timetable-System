CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall

SERVER_SRCS = server.cpp \
              TrainManager/TrainManager.cpp \
              Commands/Command.cpp \
              Commands/Commandqueue.cpp \
              xml_parser/tinyxml2.cpp

CLIENT_SRCS = client.cpp

SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

all: server client

server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o server $(SERVER_OBJS)

client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o client $(CLIENT_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f server client $(SERVER_OBJS) $(CLIENT_OBJS)