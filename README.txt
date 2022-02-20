Nicholas Vitale nvitale 1683587

Files:

Makefile-> the makefile for the assignment. creates two executable binaries, myclient and myserver.

README.txt-> the file you are reading.

bin (directory)-> no files in it, but when make is run, the binaries will be placed here.

./bin/myclient-> this binary file is the implementation of the client side. it's job is to read a file, write it to a UDP socket which sends it to the server. Also, it is in charge of the various bookkeeping tasks of the assignment, as the server is really just a "dummy" server that reads data and sends acks.

./bin/myserver-> this binary file is the implementation of the server side. it's job is to wait for the client to send it something, read the client's messages, and then write them to the outfile.

doc (directory)-> the directory that contains the documentation file for the assignment and the graph of sent data and received acks with respect to time.

./doc/documentation.txt -> This file's purpose is to describe the methodology of the assignment, as well as 5 test cases that were utilized.

./doc/dataAndAcksGraph.pdf -> This file is a pdf that contains the graph of data that is send and acks that are received by the client with respect to time.

src (directory)-> the directory that contains all the source code for the assignment.

./src/client.c-> the implementation of the client side of the assignment.

./src/server.c-> the implementation of the server side of the assignment.
