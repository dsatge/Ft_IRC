ft_irc : Internet Relay Chat

This project has been created as part of the 42 curriculum by :
Akhmed Dovletov (akdovlet), Deborah Satge (dsatge), Enzo Schneider (Enschnei).

~ Content ~

1/ Descrpition
2/ Instructions
	a/ Compilation
	b/ Launch server
	c/ Connect client
	d/ Features
3/ Resources


1/ Description

This project is about creating an Internet Relay Chat (IRC) server. It needs to create a real time messaging that can be either public or private. User need to be able to exchange private messages and join discussion group. The server needs to keep running with multiple client and even when client disconnect unexpectedly. Everything needs to follow the RFC 2812 protocol.
This projet needs to be able to be complie as c++98 standard.

2/ Instructions

a/ Compilation
This project contain a Makefile. To compile use the command:
 ```bash
 make
 ```

b/ Launch server
The server takes two argument, ./ircserv <port> <password> for exemple:
```bash
./ircserv 6067 secretCode
```

c/ Connect client
The is two different way to connect a client, with nc or irssi.
nc protocole :
```bash
nc -v localhost 6067
```
irssi protocole :
```bash
irssi -c localhost -p 6067 -w secretCode
```

After the client enter the command to join the server he needs to enter, the password as 
PASS <password>
```bash
PASS secretCode
```
Then he needs to set his nickname (it will be used to name the client in the server) as:
NICK <nickname>
```bash
NICK boby
```

Then he needs to set the username and features (following the RFC 2812 protocole)
USER <nickname> <mode> <server_name> :<real_name>
<username> : Name of the account.
<mode> : A number between 0 and 8. It set up the initial setup.
<server_name> : Server's name. Not used anymore, can be feeled with '*'.
<real_name> : Real name of the user, it follows ':' and is the last parameter. In this way it can contains spaces.
```bash
USER bob 0 * :Robert Jr Smith
```

The client is now set in the server and ready to use other features.


d/ Features

Here are all the command available in the server:

QUIT                      - Disconnect from server
KICK <users>              - Remove user from channel (moderator only)
INVITE <users> <channel>  - Invite user to channel (moderator only)
TOPIC <text>              - Set channel topic (moderator only)
TOPIC                     - View channel topic
MODE <channel> +flag      - Change channel's mode
	 i                    - Set/remove Invite-only channel
	 t                    - Set/remove the restrictions of the TOPIC command to channel
								operators
	 k                    - Set/remove the channel key (password)
	 o                    - Give/take channel operator privilege
	 l                    - Set/remove the user limit to channel
JOIN <channel>            - Join or create a channel
MESSAGE > PRIVMSG target :msg
PART #channel             - Leave a channel
STATUS                    - Show online users
STATUS <channel>          - Show users in a channel
LIST                      - List all channels
MESSAGE <users> <message> - Send a private message


3/ Resources

 section listing classic references related to the topic (documen-
tation, articles, tutorials, etc.), as well as a description of how AI was used —
specifying for which tasks and which parts of the project.


➠ Additional sections may be required depending on the project (e.g., usage
examples, feature list, technical choices, etc.).
