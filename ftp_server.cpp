#include "ftp_server.h"

using namespace std;

int main()
{
	int server_socket_id;
	ftp_init();
	server_socket_id = get_server_socket_id();
	cout<<"server_socket_id:"<<server_socket_id<<endl;
	do_loop(server_socket_id);
}
