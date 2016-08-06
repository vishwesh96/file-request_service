all: compile

compile:
	@gcc -pthread -o server server.c
	@gcc -pthread -o client client.c
	@mv server ../../server	
	@mv client ../../client	