#Distributed Key-Value Store
We implemented the distributed key­value store as a replica server architecture for shared memory. We created 2 classes for the replica server and the client respectively.

##Architecture
We implemented a set of Replica Servers with listening sockets on individual ports given by SERVER_PORT + id(replica id), and interconnected each replica server to the other. All of this was handled in the server class that we made. Each server had a listen socket that the client would use to connect. There was a shared array for the replica servers that hold each server’s listening socket, open socket connecting to all other servers, and an array holding sockets for the client connection.
We received data from each connection on a separate thread. And each unicast would be implemented on a new thread. For the server­server communication, we simulated random channel delays while for the client­server connection we didn’t add any channel delays.

##Linearizability
Linearizability involves per­process order preservation and real­time order preservation. Linearizability of shared memory was implemented using totally ordered read and write operations. We implemented totally ordered multicast with the help of a sequencer server which would define the order of messages for all of the other servers. We used a queue to hold the messages with the server that sent out the message delivered, and an acknowledgement queue that held all of the processed messages. This allowed us to handle the case when the sequencer order was delivered before the message itself to a server due to channel delays. We further allowed multiple clients to connect and give commands to the same server by processing only one command after the other on a particular server.

##Eventual Consistency
Eventual consistency was implemented based on the Last­writer wins rule using R, W and timestamps. R, W were the no. of servers to receive an a response from before an operation was considered complete and an ACK to the client was returned.
 We could have used Logical vector timestamps, but since the various server instances were all on the same computer and had a common notion of time, we just used system time as timestamps. During a get operation, the server would retrieve the variable value from R other servers at random and select the value with the latest timestamp. Similarly, during the write operation, the server would update its own value and send a multicast to all other servers with the value and its associated timestamp. The receiving processes would update their values depending on the timestamp and send a response back to the server. The original server would then consider the write operation to be complete once it would receive values from W other servers.
---
Logging
We further added file logging abilities for each server to make use of a consistency model visualization tool. We logged the request and response on each server.
Client Commands
var range : a­z val range : 0­9
Commands:
id val: sets client_id = val
connect val : connects client to server with server_id = val
put var val : sends the server it is connected to a request to change the value of var  to val
get var : requests the value of v ar from the server that the client is connected to delay time in ms : delay client process by time in ms milliseconds
dump : sends a dump request to the server the client is connected to, making the
server print the values of all its variables to stdout.
Contributors: Bhanu Agarwal, bhanu13, Ankur Gupta, agupta67
