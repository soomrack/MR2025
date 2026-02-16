CXX = g++
CXXFLAGS = -std=c++23 -pthread -Wall
TARGETS = server client logger

all: $(TARGETS)

logger: logger.cxx
	$(CXX) $(CXXFLAGS) -o logger logger.cxx

server: server.cxx
	$(CXX) $(CXXFLAGS) -o server server.cxx

client: client.cxx
	$(CXX) $(CXXFLAGS) -o client client.cxx

clean:
	rm -f $(TARGETS)

.PHONY: all clean
