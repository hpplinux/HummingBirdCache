CXXFLAGS=-g -O0 -I../include

Humming_bird_client: Humming_bird_client.o dh_thread.o dh_thread_pool.o Humming_bird_work.o
	$(CXX) -o $@ $^ -lpthread

install: Humming_bird_client
	cp Humming_bird_client ../../Humming_bird_client

Humming_bird_client.o: Humming_bird_client.cpp

dh_thread.o: dh_thread.cpp

dh_thread_pool.o: dh_thread_pool.cpp

Humming_bird_work.o: Humming_bird_work.cpp

.PHONY: clean
clean:
	rm Humming_bird_client.o Humming_bird_client dh_thread.o dh_thread_pool.o Humming_bird_work.o