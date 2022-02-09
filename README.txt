Nicholas Vitale nvitale 1683587

Files:

Makefile-> the makefile for the assignment. creates two executable binaries, myclient and myserver.

README.txt-> the file you are reading.

bin (directory)-> no files in it, but when make is run, the binaries will be placed here.

./bin/myclient-> this binary file is the implementation of the client side. it's job is to read a file, write it to a UDP socket which sends it to the server. Also, it is in charge of the various bookkeeping tasks of the assignment, as the server is really just a "dummy" server that reads data and sends acks.

./bin/myserver-> this binary file is the implementation of the server side. it's job is to wait for the client to send it something, read the client's messages, and then write them to the outfile.

doc (directory)-> the directory that contains the documentation file for the assignment.

./doc/documentation.txt -> the only file in the doc directory. its purpose is to describe the methodology
of the assignment, as well as 5 test cases that were utilized.

src (directory)-> the directory that contains all the source code for the assignment.

./src/client.c-> the implementation of the client side of the assignment.

./src/server.c-> the implementation of the server side of the assignment.
