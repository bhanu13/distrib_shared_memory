#ifndef _CLIENT_H
#define _CLIENT_H


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
#include <sys/time.h>
#include <cstdlib>
#include <sstream>


#define MAX_CLIENTS 100
#define MAX_MSG_L 50
#define CLIENT_BASE_PORT 7000
#define SERVER_PORT 5000
#define IP "localhost"
#define ALPHA "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define NUMBER "0123456789"


using namespace std;


struct client
{
	public:
		client();
		~client();


		void close_client();


		void connect_to_server(int server_id);
		//Helper for client to server connection
		int connect_to_server_(const char* domain_or_ip, const char* port_string);
		void listener_function();

		void unicast_send(string send_msg);
		// void unicast_recv(int src_id, string &recv_msg);

		void send_msg(string msg);
		void delay_send(string send_msg);
		void delay_recv(int src_id, string &recv_msg);
		int delay_channel();



		void * get_data(void * s);
		void * get_command(void * t);


		static void* get_command_callback(void * t);
		static void* delay_send_callback(void* t);
		static void* get_data_callback(void* t);


		int pull_data_from_server(string var);
		int push_data_to_server(string var, string val);
		int server_dump();
		void delay_client(int ms);

		int my_id;
		int my_server_id;
		int my_server_socket;
		int alive;
		int ack_count;
};

#endif