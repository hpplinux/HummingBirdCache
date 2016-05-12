CXXFLAGS=-g -O0

HummingBirdClient: HummingBirdClient.o
	$(CXX) -o $@ $^

install: HummingBirdClient
	cp HummingBirdClient ../../HummingBirdClient

HummingBirdClient.o: HummingBirdClient.cpp

.PHONY:
clean:
	rm HummingBirdClient.o HummingBirdClient