#include "client.h"

struct client_msg {
	client_msg(client * c, string s)
	{
		this->c = c;
		msg = s;
	};

	client * c;
	string msg;
};

string itos(int num)
{
    stringstream ss;
    ss << num;
    return ss.str();
};


//---------------------------------------------------

client::client()
{
	//assign after making connections
	my_id = -1;
	//assign after getting input from user
	my_server_id = -1;
	my_server_socket = -1;
	alive = 1;
	//increment or decrement according to send/receive
	ack_count = 0;



	// pthread_t cmd_t;
	// pthread_create(&cmd_t, NULL, get_command_callback, this);
	get_command(NULL);
}

client::~client()
{
	close_client();
}


void client::close_client()
{
	alive = 0;
	//close all threads
	//disconnect socket connections
	//close client
	//delete
	return;
}


void * client::get_command_callback(void * t)
{
	client * p = (client *) t;
	p->get_command(NULL);
	pthread_exit(NULL);
}

void * client::get_command(void * t)
{
	//take input and parse to get command
	while(alive){
		while(ack_count < 0)
		{
			usleep(100000);
		}
		cout<<"Command: ";
		string cmd;
		getline(cin, cmd);

		int idx_id = cmd.find("id");
		int idx_pull = cmd.find("get");
		int idx_push = cmd.find("put");
		int idx_dump = cmd.find("dump");
		int idx_delay = cmd.find("delay");
		int idx_connect = cmd.find("connect");
		int idx_send = cmd.find("send");
		int idx_close = cmd.find("exit");
		int idx_ack = cmd.find("ack");

		if(idx_id >= 0)
		{
			int id_idx = cmd.find_first_of(NUMBER);
			char * id_ = &cmd[id_idx];
			my_id = atoi(id_);
			cout<<"Setting client ID to "<<my_id<<endl;
		}
		else if(idx_connect >= 0)
		{
			// string::size_type sz;
			int server_idx = cmd.find_first_of(NUMBER);
			if(server_idx >= 0)
			{
				int new_id = atoi(&(cmd[server_idx]));
				connect_to_server(new_id);
			}
			else
			{
				cout<<"Server ID incorrect/missing!"<<endl;
			}
		}
		else if(idx_ack >= 0)
		{
			cout<<"Ack count for C"<<my_id<<"is : "<<ack_count<<endl;
		}
		else if(idx_close >= 0)
		{
			//exit client
			break;
		}
		else if(idx_pull >= 0)
		{
			string var = cmd.substr(idx_pull + 3);
			int var_idx = var.find_first_of(ALPHA);
			if(var_idx < 0)
			{
				cout<<"Variable incorrect/missing!"<<endl;
			}
			else
			{
				// cout<<"GET "<<var[var_idx]<<endl;
				string s = &(var[var_idx]);
				pull_data_from_server(s);

			}
		}
		else if(idx_push >= 0)
		{
			string var_val = cmd.substr(idx_push + 3);
			int val_idx = var_val.find_first_of(NUMBER);
			int var_idx = var_val.find_first_of(ALPHA);
			if(var_val.length() <= 0 || val_idx < 0 || var_idx < 0)
			{
				cout<<"Variable/Value incorrect/missing!"<<endl;
			}
			else
			{
				string var = string(&(var_val[var_idx]), 1);
				string val = string(&(var_val[val_idx]));
				// cout<<"PUT "<<var<<" "<<val<<endl;
				push_data_to_server(var, val);
			}
		}
		else if(idx_dump >= 0)
		{
			send_msg("d");
		}
		else if(idx_delay >= 0)
		{
			//put client to sleep
			int num_idx = cmd.find_first_of(NUMBER);
			if(num_idx < 0)
			{
				cout<<"Please enter no. with delay"<<endl;
				continue;
			}
			string t = cmd.substr(num_idx);
			int delay = atoi(&t[0u]);
			// cout<<"Delay "<<delay<<endl;
			delay_client(delay);
		}
		else if (idx_send >= 0)
		{
			string msg = cmd.substr(idx_send + 4);
			send_msg(msg);
		}
		else
		{
			cout<<"Invalid Command"<<endl;
		}

	}
	return NULL;
}

void client::send_msg(string msg)
{
	client_msg c(this, msg);
	pthread_t send_t;
	pthread_create(&send_t, NULL, delay_send_callback, &c);
	usleep(10000);
	return;
}

void * client::delay_send_callback(void * t)
{
	client * p = ((client_msg *) t)->c;
	string s = ((client_msg *) t)->msg;
	p->delay_send(s);
	pthread_exit(NULL);
}


void client::delay_send(string send_msg)
{
	//sleep and then unicast
	// usleep(delay_channel()*500000);
	unicast_send(send_msg);
	return;
}

void client::unicast_send(string send_msg)
{
	if(my_server_socket != -1)
	{
		char *s_msg = &send_msg[0u];
		if(send(my_server_socket, s_msg, send_msg.length(), 0) == -1)
		{
			perror("send");
		}
	}
	return;
}

void * client::get_data_callback(void * s)
{
	client * t = (client *) s;
	t->get_data(NULL);
	pthread_exit(NULL);
}

void * client::get_data(void* s)
{
	//data from the server
	int bytes_received;
	char rec_msg[30];
	memset(rec_msg, 0, 30);

	while(my_server_socket != -1)
	{
		if((bytes_received = recv(my_server_socket, rec_msg, 29, 0)) == -1)
		{
			perror("recv");
			pthread_exit(NULL);
		}
		if(rec_msg[0] != '\0')
		{
			// if(! ((((rec_msg[0] - '0' >= 0) && (rec_msg[0] - '9' <= 0)) || rec_msg[0] == 'A') && ack_count >= 0))
				cout<<endl<<"C"<<my_id<<": "<<rec_msg<<endl;
		
			if(rec_msg[0] == 'A')
				ack_count = 0;
			else if((rec_msg[0] - '0' >= 0) && (rec_msg[0] - '9' <= 0))
				ack_count = 0;
			memset(rec_msg, 0, 30);
		}
	}
	pthread_exit(NULL);
	return NULL;
}


void client::connect_to_server(int server_id)
{
	string port = itos(SERVER_PORT + server_id);
	char *port_ = &port[0u];
	int server_socket = connect_to_server_(IP, port_);

	if(server_socket < 0)
	{
		cout<<"Unable to connect to server "<<server_id<<endl;
		return;
	}

	my_server_id = server_id;
	my_server_socket = server_socket;
	cout<<"Connected client to S"<<server_id<<endl;
	
	unicast_send("ID is C" + itos(my_id));
	pthread_t t_recv;
	pthread_create(&t_recv, NULL, get_data_callback, this);

	return;
}

int client::pull_data_from_server(string var)
{
	//delay_send(server_id, "g var")
	//wait for returned value
	//returns success? the value?

	string msg = "g" + var;
	send_msg(msg);
	ack_count--;
	//string send_to_server = 'g';
	//send_to_server.append(var);
	//delay_send(my_server_id, send_to_server);
	//returns 1 for success
	return 0;
}

int client::push_data_to_server(string var, string val)
{
	//delay_send(server_id, "p var val")
	//returns success/ack count
	//wait for ack
	string msg = "p" + var + val;
	// cout<<msg<<endl;
	send_msg(msg);
	ack_count--;
	//string send_to_server = 'p';
	//send_to_server.append(var + (string)val);
	//delay_send(my_server_id, send_to_server);
	//returns 1 for success
	return 0;
}

int client::server_dump()
{
	unicast_send("d");
	return 0;
}

void client::delay_client(int ms)
{
	usleep(1000*ms);
	//delay all client processes
	//maybe put all threads other than stdin to sleep?
	//returns nothing.
	return;
}

int client::delay_channel()
{
	return ((rand()%10) + 1);
}


//-----------------------------------------------------------------------------
// Open a new connection to a listening server.
int client::connect_to_server_(const char* domain_or_ip, const char* port_string)
{
	struct addrinfo hints, *servinfo, *p;
	int rv, sockfd;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((rv = getaddrinfo(domain_or_ip, port_string, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}

		if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}

	if(p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		return -1;
	}

	freeaddrinfo(servinfo); // all done with this structure
	
	return sockfd;
}