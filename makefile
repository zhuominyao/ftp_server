tcp_server:ftp_server.o ftp_server_lib.o
	g++ -o ftp_server ftp_server.o ftp_server_lib.o -pthread

ftp_server_lib.o:ftp_server_lib.cpp ftp_server_lib.h
	g++ -c ftp_server_lib.cpp

ftp_server.o:ftp_server.cpp ftp_server.h ftp_server_lib.h
	g++ -c ftp_server.cpp
