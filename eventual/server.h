#ifndef _SERVER_H
#define _SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include "constants.h"




using namespace std;

struct msg_pair
{
	msg_pair(string msg, int id)
	{
		this->msg = msg;
		i = id;
	};
	string msg;
	int i;
};

class server
{
public:
	server();
	~server();
	void get_command();
	void killserver();
	void setup_listen();
	void connect_replicas();

	// int create_client_connection(int pid);			// Create a new connection with a server server
	void create_server(int id);
	void connect_to_server(int src_id, int dest_id);
	
	// For pthreads
	static void * create_server_callback(void * t);
	static void * get_data_callback(void * t);
	static void * send_data_callback( void * t);

	// void close_client(int dest_id);				// Close connection with a server server
	// void close_server();

	int connect_to_server(int pid);
	void get_data(int id, int conn_id);

	int gettime();
	//Eventual Consistency Functionality
	void multicast(int src_id, string msg);
	void put_server(string cmd, int id, int src_id, int client);
	void get_server(string cmd, int id, int src_id, int client);
	void kill_a_server(int id);

	// UNICAST
	void unicast_send(int dest_id, int src_id, string send_msg);
	void client_send(int server_id, int client_id, string send_msg);
	int delay_channel();

	// Logging Functions
	void create_logs();
	//----------------------------------------------------------------------------------------------

	// Helper Functions for handling connections
	int connect_to_server_(const char* domain_or_ip, const char* port_string);				// Open a new socket to the specified listening port
	static int server_response(int socket_to_client);										// The response sent to client server
	void close_connection(int pid);

	// Create Server Helper-------
	int create_listen_server(const char* port_to_bind, int id, int(*handleConnection)(int));		// Create a listen server at specified port
	void * get_in_addr(struct sockaddr* sa);
	void sigchld_handler(int s);
	//-----------

	// Internal Socket Structures
	int server_soc[MAX_REPLICAS][MAX_REPLICAS];	// My Open Sockets with other processes.
	int listen_soc[MAX_REPLICAS];
	int client_soc[MAX_REPLICAS][MAX_CLIENTS];	// For client to server connections.

	int num_connect[MAX_REPLICAS];
	int server_on;

	int vars[MAX_REPLICAS][NUM_ALPHA];

	int w, r;

	// EVENTUAL CONSISTENCY
	int ts[MAX_REPLICAS][NUM_ALPHA];		// The associated time stamp array for each value
	int r_ack[MAX_REPLICAS];				// List to keep track of the ack messages received for read, write operation
	int w_ack[MAX_REPLICAS];
	// int r_busy[MAX_REPLICAS], w_busy[MAX_REPLICAS];					// New Commands can be executed only when old commands are finished.
	int busy[MAX_REPLICAS];
	// Logging Structs
	ofstream f[MAX_REPLICAS];
};


#endif