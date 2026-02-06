CXX = g++
CXXFLAGS = -std=c++23 -pthread -Wall
TARGETS = server client

all: $(TARGETS)

server: server.cxx
	$(CXX) $(CXXFLAGS) -o server server.cxx

client: client.cxx
	$(CXX) $(CXXFLAGS) -o client client.cxx

clean:
	rm -f $(TARGETS)

.PHONY: all clean
