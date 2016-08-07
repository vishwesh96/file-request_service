1)Open a terminal and run the following command.
$make
This will create the executables server-mp and multi-client.

2)Move the executables to the folder containing the files folder(which has all the files from foo0.txt to foo9999.txt)

3)In terminal run following commands to start client and server

$./server-mp <port> (for server)
(eg: $./server-mp 5000)

$./multi-client <server-ip> <port> <number of threads> <time to run> <think time> <mode>
eg: $./multi-client 169.254.9.157 5001 25 120 1 fixed