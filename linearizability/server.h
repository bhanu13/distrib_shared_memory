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
#include <algorithm>
#include <iostream>
#include <fstream>


#define TOTAL_REPLICAS 3

#define MAX_REPLICAS (TOTAL_REPLICAS + 1)
#define seq_id TOTAL_REPLICAS					// The last replica server is the sequencer

#define MAX_CLIENTS 5
#define NUM_ALPHA 26

#define MAX_MSG_L 100
#define SERVER_PORT 5000		// Starting from Port 5000 till 5004
#define IP "localhost"

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

struct seq_pair
{
	seq_pair(int id, int num/*, int msg*/)
	{
		this->s_id = id;
		this->seq_num = num;
		// this->command = msg;
	};
	int s_id;
	int seq_num;
	// int command;

	bool operator<(const seq_pair &a) const
	{
		return seq_num < a.seq_num;
	}
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

	//Linearization Functionality
	void multicast(int src_id, string msg);
	void put_server(string cmd, int id, int src_id, int client);
	void get_server(string cmd, int id, int src_id, int client);

	// UNICAST
	void unicast_send(int dest_id, int src_id, string send_msg);
	void client_send(int server_id, int client_id, string send_msg);
	int delay_channel();

	// Logging Functions
	void create_logs();
	int gettime();
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

	// TOTAL ORDERING DATA
	int recv_order[MAX_REPLICAS];
	int sent_msg;					// For the sequencer order
	vector< vector<msg_pair> > Q;	// Hold back queue
	vector< vector<seq_pair> > seq_q; //sequencer queue

	//Helpers to sort the sequencer queue
	void sort_seq_q(int a);
	// bool compareFunc(const seq_pair x, const seq_pair y);

	// int r_busy[MAX_REPLICAS], w_busy[MAX_REPLICAS];					// New Commands can be executed only when old commands are finished.
	int busy[MAX_REPLICAS];
	// Logging Structs
	ofstream f[MAX_REPLICAS];

};


#endif