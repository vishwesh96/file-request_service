all: compile

compile:
	@g++ -w -pthread -o server-mt server-mt.cpp
	@g++ -w -pthread -o multi-client multi-client.cpp
