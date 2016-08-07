all: compile

compile:
	@gcc -pthread -o server-mp server-mp.c
	@gcc -pthread -o multi-client multi-client.c
