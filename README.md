# ft_irc : Internet Relay Chat

This project has been created as part of the 42 curriculum by :  
 Akhmed Dovletov (akdovlet), Deborah Satge (dsatge), Enzo Schneider (Enschnei).

### ~ Content ~

 &emsp;1/ Description\
 &emsp;2/ Instructions\
&emsp;&emsp;a/ Compilation\
&emsp;&emsp;b/ Launch server\
&emsp;&emsp;c/ Connect client\
&emsp;&emsp;d/ Features\
 &emsp;3/ Resources and Notions

  
## 1/ Description

This project is about creating an Internet Relay Chat (IRC) server. It needs to create a real-time messaging system that can be either public or private. Users need to be able to exchange private messages and join discussion group. The server needs to keep running with multiple clients and even when client disconnect unexpectedly. Everything needs to follow the RFC 2812 protocol.  
This projet must be compiled using the c++98 standard.
  
## 2/ Instructions

### a/ Compilation
This project contains a Makefile. To compile use the command:
 ```bash
 make
 ```

### b/ Launch server
The server takes two arguments, ./ircserv \<port\> \<password\> for example:
```bash
./ircserv 6067 secretCode
```

### c/ Connect client
There are two ways to connect a client: with `nc` or `irssi`.  
nc protocole :
```bash
nc -v localhost 6067
```
irssi protocole :
```bash
irssi -c localhost -p 6067 -w secretCode
```

After the client enters the command to join the server he needs to enter, the password as : PASS \<password\>
```bash
PASS secretCode
```
Then he needs to set his nickname (it will be used to name the client in the server) as : NICK \<nickname\>
```bash
NICK boby
```

Then he needs to set the username and features (following the RFC 2812 protocole)  
USER <nickname> <mode> <server_name> :<real_name>  
\<username\> : Name of the account.  
\<mode\> : A number between 0 and 8. It set up the initial setup.  
\<server_name\> : Server's name. Not used anymore, can be filled with '*'.  
\<real_name\> : Real name of the user, it follows ':' and is the last parameter. In this way it can contain spaces.  
```bash
USER bob 0 * :Robert Jr Smith
```

The client is now set in the server and ready to use other features.  


### d/ Features

Here are all the commands available in the server:

|COMMANDS | UTILITIES |
|----------:| --------------------------------------------------------------------------|
|**CHANNEL OPERATION :** &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;| |
|`QUIT` :\<message\> |Disconnect from server |
|`JOIN` \<channel\> |Join or create a channel |
|`PART` \<channel\> |Leave a channel |
|`NAMES` \<channel\> |List users in a channel
|`LIST` |List all channels |
|`TOPIC` |View channel topic | 
|**MODERATORS options :**&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;| |
|`KICK` \<users\> |Remove user from channel|
|`INVITE` \<users\> \<channel\> |Invite user to channel|
|`TOPIC` \<text\> |Set channel topic | 
|`MODE` \<channel\> \<flag\> \[parameters\] |Change channel's mode from flags' list : |
|	 `+i`  or `-i`|Set/remove Invite-only channel |
|	 `+t` or `-t`|Set/remove the restrictions of the TOPIC command to channel operators|
|	 `+k` *\<password\>* or `-k`|Set/remove the channel key (password) |
|	 `+o` or `-o`|Give/take channel operator privilege |
|	 `+l` *\<limit\>* or `-l`|Set/remove the user limit to channel |
|**MESSAGING options :**&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;| |
|`PRIVMSG` \<target\> :\<message\> |Send a private message to another user with his nickname |
|`PRIVMSG` #\<channel\> :\<message\> |Send a message to a channel. Only users in this channel will see this message |
| \<message\> |When in a group, send a message to the group |
|**GENERAL options :**&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;| |
|`HELP` | Show features options *(not RFC 2812 protocol)* |
  

## 3/ Resources and Notions

In this project we used **socket** as access point. It is what is going to create a connection between different user. This [website](https://www.geeksforgeeks.org/computer-networks/socket-in-computer-network/) visually explain the use of socket to create entry points. Those entry point on the form of FDs. The first FD set up is the listening socket (server FD). We use **bind() to associate this FD with a port and IP adress. Then we use **listen()** to set the server as passive. In this way the server will wait for incoming connections.\

After setting up our server the next part is to connect and execute the different actions needed. But, to avoid to keep checking for any changes when nothing happen, we use [poll()](https://linux.die.net/man/2/poll). This function will keep everything 'on sleep' until something happens. When something is happening, **POLLIN** will inform that there is data to read. Depending on which fd's the information is coming we can understand two things.\
&emsp;If the information comes from the FDs of the server, on the listening socket: a new client is trying to connect to the server. It will then need to be **accept()**.\
&emsp;If the information comes from the FDs of a client, the information needs to be decrypted by **recv()**.\

When processing the returned value of [recv()](https://www.ibm.com/docs/en/zos/2.5.0?topic=functions-recv-receive-data-socket) we can extract different information:
&emsp;Recv() returns 0 : meaning the client is not connected anymore. He needs to be removed from the list of clients, he will no longer be able to receive messages or be moderator.\
&emsp;Recv() returns >0 : The user is trying to send a message, there is information filling the buffer. This information will be saved to be [send()](https://learn.microsoft.com/fr-fr/windows/win32/api/winsock2/nf-winsock2-send) to another user or to a channel.\
&emsp;Recv() returns <0 : There is an error, recv() is waiting for a message. In our program this return value is usually not a fatal error as our sockets are set up as [O_NONBLOCK](https://medium.com/@hajorda/non-blocking-sockets-and-i-o-multiplexing-with-epoll-in-c-bd3d8e54c20a), errno is set to `EAGAIN` or `EWOULDBLOCK`, because there is no data available, the program returns immediately and will not block indefinitly.\

<ins>Here a visual representation on how it works :</ins>
```
START
         |
    [ socket() ]  <-- Create the endpoint (the "phone")
         |
     [ bind() ]   <-- Assign IP & Port (the "phone number")
         |
    [ listen() ]  <-- Enable the queue (the "ringer")
         |
+------->+
|   [  poll()  ]  <-- SLEEP (Wait for events)
|        |
|   *Event occurs* (New connection or message)
|        |
|   [ accept() ]  <-- PICK UP (Create a specific Client FD)
|        OR          
|   [  recv()  ]  <-- READ (Get data from an existing Client)
|        |
+--------+
```

To implement the different features and command we followed the [RFC 2812 protocol](https://www.rfc-editor.org/rfc/rfc2812.txt). As the protocol is long and the point of the exercise was not to learn everything about this protocol but more to understand how an Internet Relay Chat server works we purposely ignore some specifications : 

[!NOTE] In this project AI has been mostly used to get specific information about some element to understand better how some function works. It also has been used to understand the parameters needed for some functions.
