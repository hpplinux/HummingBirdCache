CXXFLAGS=-g -O0 -I../include

Humming_bird_server: Humming_bird_server.o dh_thread.o dh_thread_pool.o Humming_bird_work.o
	$(CXX) -o $@ $^ -pthread

install: HummingBirdServer
	cp HummingBirdServer ../../bin

Humming_bird_server.o: Humming_bird_server.cpp

dh_thread.o: dh_thread.cpp

dh_thread_pool.o: dh_thread_pool.cpp

Humming_bird_work.o: Humming_bird_work.cpp

.PHONY:
clean:
	rm Humming_bird_server.o Humming_bird_server dh_thread.o dh_thread_pool.o Humming_bird_work.o