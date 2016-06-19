#include "server.h"

//-------------------------------------------
// Used for listen server
struct l_server {
	l_server()
	{
		t = NULL;
		id = -1;
	};

	l_server(server * s, int s_id)
	{
		t = s;
		id = s_id;
	};
	l_server(server * s, int s_id, int d_id)
	{
		t = s;
		id = s_id;
		id2 = d_id;
	};
	server * t;
	int id;
	int id2;
};


struct s_server
{
	s_server(server * s, int s_id, int d_id, string msg)
	{
		t = s;
		id = s_id;
		id2 = d_id;
		this->msg = msg;
	};
	server * t;
	int id;
	int id2;
	string msg;
};


string itos(int num)
{
    stringstream ss;
    ss << num;
    return ss.str();
}
//-------------------------------------------
server::server()
{
	server_on = 1;
	//INIT Eventual Consistency Data Structs
	w = W;
	r = R;
	// memset(r_busy, 0, MAX_REPLICAS*sizeof(int));
	// memset(w_busy, 0, MAX_REPLICAS*sizeof(int));
	memset(busy, 0, MAX_REPLICAS*sizeof(int));
	int t = gettime();
	for(int i = 0; i<MAX_REPLICAS; i++)
	{
		for(int j = 0; j<NUM_ALPHA; j++)
		{
			ts[i][j] = t;
		}		
	}

	// Create Replica Server
	for(int i = 0; i<MAX_REPLICAS; i++)
	{
		listen_soc[i] = -1;
		num_connect[i] = 0;
		r_ack[i] = r;
		w_ack[i] = w;
	}
	memset(vars, 0, MAX_REPLICAS*NUM_ALPHA*sizeof(int));

	setup_listen();
	usleep(20000);
	connect_replicas();
	// Open Log Files
	create_logs();

	get_command();
}

server::~server()
{
	killserver();
}

void server::create_logs()
{
	for (int i = 0; i<MAX_REPLICAS; i++)
	{
		f[i].open("logs/log_" + itos(i) + ".txt");
		// f[i] << "Writing this to a file.\n";
	}
}
void server::get_command()
{
	while(1)
	{
		string cmd;
		getline(cin, cmd);


		int idx_w = cmd.find("w");			// Set W
		int idx_r = cmd.find("r");			// Set R
		int idx_kill = cmd.find("kill");	// Kill a server

		int idx_l_soc = cmd.find("l");		// Print Listen Sockets
		int idx_c_soc = cmd.find("c");		// Print Client Sockets
		// int idx_delay = cmd.find("delay");
		int idx_cs = cmd.find("cs");	// Client Send
		int idx_ss = cmd.find("ss");		// Server Send 
		int idx_close = cmd.find("exit");	// Exit the Replica Server


		if(idx_w >= 0)
		{
			int n_w = (int)(cmd[cmd.find_first_of(NUMBER)] - '0');
			w = n_w;
			cout<<"Setting W as "<<w<<endl;
		}
		else if(idx_r >= 0)
		{
			int n_r = (int)(cmd[cmd.find_first_of(NUMBER)] - '0');
			r = n_r;
			cout<<"Setting R as "<<r<<endl;
		}
		else if(idx_cs >= 0)
		{
			int serv_id = (int)(cmd[idx_cs + 3] - '0');
			int client_id = (int)(cmd[idx_cs + 5] - '0');
			string msg = cmd.substr(idx_cs + 6);
			client_send(serv_id, client_id, msg);
		}
		else if(idx_ss >= 0)
		{
			int serv_id = (int)(cmd[idx_ss + 3] - '0');
			int serv_id2 = (int)(cmd[idx_ss + 5] - '0');
			string msg = cmd.substr(idx_ss + 6);
			unicast_send(serv_id, serv_id2, msg);	
		}
		else if(idx_kill >= 0)
		{
			int id = (int)(cmd[cmd.find_first_of(NUMBER)] - '0');
			kill_a_server(id);
			cout<<"S"<<id<<" is dead"<<endl;

		}

		else if(idx_l_soc >= 0)
		{
			
		}
		else if(idx_c_soc >= 0)
		{
			
		}
		else if(idx_close >= 0)
		{
			cout<<"Closing Replica Server"<<endl;
			break;
		}
		else
		{
			cout<<"Invalid Command"<<endl;
		}
	}
}

void server::kill_a_server(int id)
{
	for(int i = 0; i<MAX_CLIENTS; i++)	
		client_send(id, i, "DEAD");

	// if(listen_soc[id] != -1)
	// 	close(listen_soc[id]);

	// for(int j = 0; j<MAX_REPLICAS; j++)
	// {
	// 	if(server_soc[id][j] != -1)
	// 		close(server_soc[id][j]);
	// 	server_soc[id][j] = -1;
	// }
	return;
}
void server::killserver()
{
	server_on = 0;
	// Close all listening sockets
	for(int i = 0; i<MAX_REPLICAS; i++)
	{
		if(listen_soc[i] != -1)
			close(listen_soc[i]);

		// Close all logs
		f[i].close();
	}
	// Close all open connection sockets
	for(int i = 0; i<MAX_REPLICAS; i++)
	{
		for(int j = 0; j<MAX_REPLICAS; j++)
		{
			if(server_soc[i][j] != -1)
				close(server_soc[i][j]);
			server_soc[i][j] = -1;
		}
	}
	// Kill all threads

	return;
}

void server::close_connection(int pid)
{
	close(listen_soc[pid]);
	return;
}


void server::setup_listen()
{
	pthread_t server_t[MAX_REPLICAS];
	l_server s[MAX_REPLICAS];
	for(int i = 0; i<MAX_REPLICAS; i++)
	{
		s[i].id = i;
		s[i].t = this;
		pthread_create(&(server_t[i]), NULL, create_server_callback, &(s[i]));
	}
	cout<<"Started Listening on the Replica Servers"<<endl;
	return;
}

void server::connect_replicas()
{
	for(int i = 0; i<MAX_REPLICAS; i++)
	{
		for(int j = 0; j<i; j++)
		{
			usleep(10000);
			connect_to_server(j, i);
		}
	}
	cout<<"Connected the replica servers"<<endl;
}


void * server::create_server_callback(void * t)
{
	server * p = ((l_server *) t)->t;
	int id = ((l_server *) t)->id;
	p->create_server(id);
	pthread_exit(NULL);
}

void server::create_server(int id)
{
	string port = itos(SERVER_PORT + id);
	char * port_ = &port[0u];

	create_listen_server((char *)port_, id, server_response);
	pthread_exit(NULL);
}

void server::connect_to_server(int src_id, int dest_id)
{
	string port = itos(SERVER_PORT + dest_id);
	char *port_ = &port[0u];
	int server_socket = connect_to_server_(IP, port_);

	if(server_socket < 0)
	{
		cout<<"Unable to connect "<<src_id<<" to server "<<dest_id<<endl;
		return;
	}
	server_soc[src_id][dest_id] = server_socket;
	// cout<<"Connected S"<<src_id<< " to S"<<dest_id<<endl;
	
	unicast_send(src_id, dest_id, "ID is S" + itos(src_id));
	pthread_t t_recv;
	// cout<<src_id<<" "<<dest_id<<endl;
	l_server S(this, src_id, dest_id);
	pthread_create(&t_recv, NULL, get_data_callback, &S);
	usleep(10000);
	return;
}

//------------------------------------------------------------------------------------

int server::delay_channel()
{
	return ((rand()%10) + 1);
}

// DELAY of 0.5 to 5 seconds.
void * server::send_data_callback( void * t)
{
	server * p = ((s_server *) t)->t;
	int id = ((s_server *) t)->id;
	int id2 = ((s_server *) t)->id2;
	string msg = ((s_server *) t)->msg;
	// int client_id = p->num_connect - 1;

	usleep(p->delay_channel()*500000);		// SEND DELAYED

	p->unicast_send(id, id2, msg);
	pthread_exit(NULL);
}

void server::unicast_send(int src_id, int dest_id, string send_msg)
{

	int socket = server_soc[src_id][dest_id];
	if(socket != -1)
	{
		char *s_msg = &send_msg[0u];
		cout<<"Unicasting from "<<src_id<<" to "<<dest_id<<", "<<send_msg<<endl;
		if(send(socket, s_msg, send_msg.length(), 0) == -1)
		{
			perror("send");
		}
		// cout<<"Message sent to "<<dest_id<<endl;
	}
	return;
}

void server::client_send(int server_id, int client_id, string send_msg)
{
	int socket = client_soc[server_id][client_id];
	if(socket != -1)
	{
		char *s_msg = &send_msg[0u];
		if(send(socket, s_msg, send_msg.length(), 0) == -1)
		{
			perror("send");
		}
		// cout<<"Message sent to "<<dest_id<<endl;
	}
	return;
}

//------------------------------------------------------------------------------------
// GET DATA
void * server::get_data_callback(void * t)
{
	server * p = ((l_server *) t)->t;
	int id = ((l_server *) t)->id;
	int id2 = ((l_server *) t)->id2;
	// int client_id = p->num_connect - 1;
	p->get_data(id, id2);
	pthread_exit(NULL);
}



// Change p_id to id informed by the server that is connected to it.
void server::get_data(int id, int conn_id)
{
	// Differentiate between client and server
	int socket = -1;
	int client = 0;
	if(conn_id >= 1000)
	{
		conn_id -= 1000;
		socket = client_soc[id][conn_id];
		client = 1;
	}
	else
	{
		socket = server_soc[id][conn_id];
	}
	int bytes_received;
	char recv_msg[30];
	memset(recv_msg, 0, 30);

	while(socket != -1)
	{
		string cmd;
		if((bytes_received = recv(socket, recv_msg, 29, 0)) == -1)
		{
			perror("recv");
			pthread_exit(NULL);
		}
		if(recv_msg[0] != '\0')
		{
			cout<<endl<<"S"<<id<<": "<<recv_msg<<endl;

			cmd = recv_msg;
			memset(recv_msg, 0, 30);
		}

		// CODE FOR DIFFERENT COMMANDS
		// PUT COMMAND
		if(cmd[0] == 'p')
		{
			while(busy[id] == 1 && client == 1)
			{
				usleep(100000);
			}
			// int var = (int)(cmd[1] - 'a');
			// int num = (int)(cmd[2] - '0');
			put_server(cmd, id, conn_id, client);
			// Figure out when to send acknowledge
			// if(client == 1)
			// {
			// 	client_send(id, conn_id, "A");
			// }
			// vars[id][var] = num;
			
		}

		// GET COMMAND
		else if(cmd[0] == 'g')
		{
			while(busy[id] == 1 && client == 1)
			{
				usleep(100000);
			}
			// int var = (int)(cmd[1] - 'a');
			get_server(cmd, id, conn_id, client);
			// int val = vars[id][var];
			// client_send(id, conn_id, itos(val));
		}

		// DUMP COMMAND
		else if(cmd[0] == 'd')
		{
			for(int i = 0; i<NUM_ALPHA; i++)
			{
				char alphabet = (char)('a' + i);
				cout<<string(&alphabet)<<" "<<vars[id][i]<<endl;
			}
		}

		// For Write Ack response
		else if(cmd[0] == 'W' && cmd[1] == 'A')
		{
			w_ack[id]++;
			int client_id = (int)(cmd[2] - '0');
			if(w_ack[id] == w)
			{	
				client_send(id, client_id, "A");
				int t = gettime();
				f[id]<<"500,"<<itos(client_id)<<",put,"<<cmd[3]<<","<<t<<",resp,"<<cmd[4]<<endl;
				// w_busy[id] = 0;
				busy[id] = 0;
			}
		}

		// For Read Ack response
		else if(cmd[0] == 'R' && cmd[1] == 'A')
		{
			int idx_atr = cmd.find_first_of("@");
			int client_id = (int)(cmd[idx_atr + 1] - '0');
	
			int idx_comma = cmd.find_first_of(",");
			
			int var = (int)(cmd[idx_comma + 1] - 'a');
			int val = (int)(cmd[idx_comma + 2] - '0');
			int t = stoi(cmd.substr(idx_comma + 3));

			if(t > ts[id][var])
			{
				vars[id][var] = val;
				ts[id][var] = t;
			}
			r_ack[id]++;
			if(r_ack[id] == r)
			{	
				client_send(id, client_id, itos(vars[id][var]));
				int t = gettime();
				f[id]<<"500,"<<itos(client_id)<<",get,"<<cmd[idx_comma + 1]<<","<<t<<",resp,"<<vars[id][var]<<endl;
				// r_busy[id] = 0;
				busy[id] = 0;
			}
		}
	}
	cout<<"Exiting the receiving thread at S"<<id<<endl;
	num_connect[id]--;
	pthread_exit(NULL);
}

//---------------------------------------------------------------------------------
void server::multicast(int src_id, string msg)
{
	cout<<"multicasting from "<<src_id<<", "<<msg<<endl;
	pthread_t send_t[MAX_REPLICAS];
	for(int i = 0; i<MAX_REPLICAS; i++)
	{
		if(src_id != i)
		{
			s_server S(this, src_id, i, msg);
			pthread_create(&(send_t[i]), NULL, send_data_callback, &S);
			usleep(10000);			// Create a thread for handling multicast
		}
	}
	return;
}

int server::gettime()
{
	time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
	return (int)t;
}


//---------------------------------------------------------------------------------
void server::put_server(string cmd, int id, int src_id, int client)
{
	int var = (int)(cmd[1] - 'a');
	int num = (int)(cmd[2] - '0');
	if(client == 1)
	{
		int t = gettime();
		string msg = cmd + "@" + itos(src_id) + "," + itos(t);
		vars[id][var] = num;
		f[id]<<"500,"<<itos(src_id)<<",put,"<<cmd[1]<<","<<t<<",req,"<<num<<endl;
		w_ack[id] = 0;
		// w_busy[id] = 1;
		busy[id] = 1;
		multicast(id, msg);
	}
	else
	{
		int idx_comma = cmd.find_first_of(",");
		int t = stoi(cmd.substr(idx_comma + 1));

		int idx_atr = cmd.find_first_of("@");
		int client_id = (int)(cmd[idx_atr + 1] - '0');
		
		// May not end up sending enough ACK responses
		if(t > ts[id][var])
		{
			//Update the variable value
			vars[id][var] = num;
			ts[id][var] = t;

			string msg = "WA" + itos(client_id) + cmd[1] + itos(num);
			unicast_send(id, src_id, msg);
		}
	}

	return;
}

void server::get_server(string cmd, int id, int src_id, int client)
{
	int var = (int)(cmd[1] - 'a');

	if(client == 1)
	{
		string msg = cmd + "@" + itos(src_id);
		r_ack[id] = 0;
		// r_busy[id] = 1;
		busy[id] = 1;
		int t = gettime();
		f[id]<<"500,"<<itos(src_id)<<",get,"<<cmd[1]<<","<<t<<",req"<<endl;
		multicast(id, msg);
	}
	else
	{
		int idx_atr = cmd.find_first_of("@");
		int client_id = (int)(cmd[idx_atr + 1] - '0');

		//Update the variable value
		int val = vars[id][var];
		int t = ts[id][var];
		char c = cmd[1];

		string msg = "RA@" + itos(client_id) + "," + c + itos(val) + itos(t);
		unicast_send(id, src_id, msg);
	}
	return;
}

//---------------------------------------------------------------------------------
// get sockaddr, IPv4 or IPv6:
void* server::get_in_addr(struct sockaddr* sa)
{
	if(sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	else
		return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void server::sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


int server::server_response(int socket)
{
	char buffer[MAX_MSG_L];
	memset(buffer, 0, MAX_MSG_L);
	string connect = "Connection Setup";
	char *connect_ = &connect[0u];
	strcpy(buffer, connect_);
	if(send(socket, buffer, MAX_MSG_L, 0) == -1)
	{
		perror("send");
		return -1;
	}
	memset(buffer, 0, 30);
	if(recv(socket, buffer, MAX_MSG_L, 0) == -1)
		return -1;

	string response = string(buffer);
	int server_idx = response.find_first_of("S");
	int client_idx = response.find_first_of("C");

	int new_id = -1;
	if(server_idx >= 0)
	{
		new_id = (int)response[server_idx + 1] - '0';
		string connect_response = "Connected to S " + itos(new_id);
		cout<<connect_response<<endl;
	}
	else if(client_idx >= 0)
	{
		new_id = (int)response[client_idx + 1] - '0';
		string connect_response = "Connected to C " + itos(new_id);
		new_id += 1000;			// Client ID Differentiator
		cout<<connect_response<<endl;	
	}
	
	return new_id;
}

//NOTE: only root can bind ports < 1024.
int server::create_listen_server(const char* port_to_bind, int id, int(*handleConnection)(int))
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	// struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if((rv = getaddrinfo(NULL, port_to_bind, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("server: socket");
			continue;
		}
		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("setsockopt");
			return -1;
		}
		if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	if(p == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		return -1;
	}
	freeaddrinfo(servinfo); // all done with this structure

	if(listen(sockfd, 10) == -1) //queue up to 10 un-accept()ed connect()s
	{
		perror("listen");
		return -1;
	}

	listen_soc[id] = sockfd;

	// sa.sa_handler = sigchld_handler; // reap all dead processes
	// sigemptyset(&sa.sa_mask);
	// sa.sa_flags = SA_RESTART;
	// if(sigaction(SIGCHLD, &sa, NULL) == -1)
	// {
	// 	perror("sigaction");
	// 	return -1;
	// }

	printf("server: bound to port %s and waiting for connections...\n", port_to_bind);

	while(server_on == 1) // main accept() loop
	{
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size);
		if(new_fd == -1)
		{
			perror("accept");
			continue;
		}
		
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof s);
		num_connect[id]++;
		printf("server: got connection from %s\n", s);
		cout<<"Connected to "<<num_connect[id]<<" processes at "<<port_to_bind<<endl;

		// TODO: Specify the process id that connects to this one.

		// num_connect++;

		// int dest_id = num_connect -1;
		// new_id = num_connect - 1;
		int new_id = handleConnection(new_fd);
		if(new_id >= 1000)
		{
			client_soc[id][new_id - 1000] = new_fd;
		}
		else
		{
			server_soc[id][new_id] = new_fd;
		}
		pthread_t t_recv;
		l_server S(this, id, new_id);
		// cout<<id<<" "<<new_id<<endl;
		pthread_create(&t_recv, NULL, get_data_callback, &S);
		usleep(10000);
		//(alternatively, we could create a thread instead of forking)
		// if(!fork()) // this is the child process
		// {
		// 	int new_id = handleConnection(new_fd);
		// 	server_soc[id][new_id] = new_fd;
		// 	exit(0);
		// }
		// close(new_fd);  // parent doesn't need this
	}
	pthread_exit(NULL);
}


// Open a new connection to a listening server.
int server::connect_to_server_(const char* domain_or_ip, const char* port_string)
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
	// else
	// {
	// 	int server = stoi(string(domain_or_ip)) - BASE_PORT;
	// 	printf("Connected to process %d\n", server);
	// }
	freeaddrinfo(servinfo); // all done with this structure
	
	return sockfd;
}
//--------------------------------------------------------------------------------------------------------------------------------
