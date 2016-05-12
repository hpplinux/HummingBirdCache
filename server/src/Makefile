CXXFLAGS=-g -O0

HummingBirdServer: HummingBirdServer.o dh_thread.o dh_thread_pool.o
	$(CXX) -o $@ $^ -pthread

install: HummingBirdServer
	cp HummingBirdServer ../../bin

HummingBirdServer.o: HummingBirdServer.cpp

dh_thread.o: dh_thread.cpp

dh_thread_pool: dh_thread_pool.cpp

.PHONY:
clean:
	rm HummingBirdServer.o HummingBirdServer dh_thread.o dh_thread_pool.o