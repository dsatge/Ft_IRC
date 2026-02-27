#define _XOPEN_SOURCE 700

# include "server.hpp"
# include "client.hpp"

Server::Server()
{
	return ;
}

Server::Server(std::string port, std::string password)
{
	this->_port = atoi(port.c_str());
	this->_password = password;
	return ;
}

Server::Server(const Server &other)
{
	this->_serverFd = other._serverFd;
	this->_port = other._port;
	this->_password = other._password;
	int	socketsFdsTabLen = this->_Fds.size() - 1;
	for (int i = 0; i < socketsFdsTabLen; i++)
	{
		this->_Fds.push_back(other._Fds[i]);
	}
	return ;
}

Server& Server::operator=(const Server &other)
{
	if (this != &other)
	{
		this->_serverFd = other._serverFd;
		this->_password = other._password;
		int	socketsFdsTabLen = other._Fds.size();
		for (int i = 0; i < (socketsFdsTabLen); i++)
			this->_Fds.push_back(other._Fds[i]);	}
	return (*this);
}

Server::~Server()
{
	return ;
}

void Server::SetServerFd(int serverFd)
{
	this->_serverFd = serverFd;
}

int Server::GetServerFd() const
{
	return (this->_serverFd);
}

int Server::GetPort() const
{
	return (this->_port);
}

std::string Server::GetPassword() const
{
	return (this->_password);
}

struct pollfd Server::GetFds(int index) const
{
	return (this->_Fds[index]);
}

std::vector<struct pollfd> Server::GetFdsContainer() const
{
	return (this->_Fds);
}


int	Server::SizeList()
{
	return (this->_Fds.size());
}

void Server::AddSocketFds(pollfd fd)
{
	this->_Fds.push_back(fd);
	return ;
}

void	handleSignal(int sig);

void Server::SetUpSignals()
{
	struct sigaction sig_act;
	memset(&sig_act, 0, sizeof(sig_act));
	sig_act.sa_handler = handleSignal;
	sigaction(SIGINT, &sig_act, NULL);
}

static std::string intToString(int num)
{
	std::stringstream ss;
	ss << num;
	return ss.str();
}

int	Server::setSocket(Server *server)
{
	struct pollfd serverFd;
	serverFd.fd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd.fd == -1)
	{
		std::cerr << RED << "Error: fail socket creation" << RESET << std::endl;
		return (EXIT_FAILURE);
	}
	server->SetServerFd(serverFd.fd);
	server->AddSocketFds(serverFd);
	/// set option to reuse adress without wait-time
	int opt_onOff = 1;
	setsockopt(this->_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt_onOff, sizeof(opt_onOff));
	/// set in nonblocking mode
	if (this->nonBlocking(serverFd.fd) == EXIT_FAILURE)
		return (EXIT_FAILURE);
	// fcntl(this->_serverFd, F_SETFL, O_NONBLOCK);
	return (EXIT_SUCCESS);
}

int	Server::nonBlocking(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	if (flag == -1)
	{
		std::cerr << RED << "Error: fcntl F_GETFL" << RESET << std::endl;
		return (EXIT_FAILURE);
	}
	if (fcntl(fd, F_SETFL, flag | O_NONBLOCK, 0) == -1)
	{
		std::cerr << RED << "Error: fcntl F_SETFL" << RESET << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

int	Server::bindFt()
{
	struct sockaddr_in addrIn;
	memset(&addrIn, 0, sizeof(addrIn));
	addrIn.sin_family = AF_INET;
	addrIn.sin_port = htons(this->_port);
	addrIn.sin_addr.s_addr = INADDR_ANY;
	if (bind(this->_serverFd, reinterpret_cast<const sockaddr *>(&addrIn), sizeof(addrIn)) != 0)
	{
		perror("bind");
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

static volatile sig_atomic_t sig_serverStop = 0;

void	handleSignal(int sig)
{
	(void) sig;
	sig_serverStop = 1;
}

void	Server::closeFds()
{
	std::vector<struct pollfd>::iterator it = this->_Fds.begin();
	if (it == this->_Fds.end())
		return ;
	for (;it != this->_Fds.end(); it++)
	{
		close(it->fd);
	}
}

int	Server::pollLoop()
{
	this->_Fds[0].events = POLLIN;
	SetUpSignals();
	while (sig_serverStop == 0)
	{
		int pollStatus = poll(&this->_Fds[0], this->_Fds.size(), -1);
		if (pollStatus < 0 && errno != EINTR)
			perror("poll");
		if (pollStatus > 0)
		{
			for (size_t index = 0; index < this->_Fds.size(); index++)
			{
				int flagDisconnect = 0;
				if (this->_Fds[index].revents & POLLIN)
				{
					if (index == 0 && clientJoiningServer(index) == EXIT_FAILURE)
						break ;
					if (index != 0)
					{
						char buffer[1024];
						ssize_t msg = recv(this->_Fds[index].fd, buffer, 1023, 0);
						if (msg == 0)
							flagDisconnect += clientquittingServer(index, buffer);
						if (msg > 0)
						{
							buffer[msg] = '\0';
							int quitFlag = clientSendingMessage(index, buffer, msg);
							if (quitFlag == 1)
								flagDisconnect += 1;
						}
						if (msg < 0)
						{
							// Client disconnected abruptly (e.g., Ctrl+C)
							flagDisconnect += clientquittingServer(index, buffer);
						}
					}
				}
				this->disconnectClient(flagDisconnect);
				flagDisconnect = 0;
			}
		}
	}
	closeFds();
	return (EXIT_SUCCESS);
}

int	Server::acceptFd(int index)
{
	struct sockaddr addr_client;
	memset(&addr_client, 0, sizeof(addr_client));
	socklen_t addr_size = sizeof(addr_client);
	int	clientFD = accept(this->_Fds[index].fd, &addr_client, &addr_size);
	if (clientFD < 0)
		perror("accept");
	return (clientFD);
}

int	Server::clientJoiningServer(int index)
{
	struct pollfd	newClient;
	newClient.fd = this->acceptFd(index);
	if (newClient.fd < 0)
		return (EXIT_FAILURE) ;
	newClient.events = POLLIN;
	newClient.revents = 0;
	this->AddSocketFds(newClient);
	Client client(newClient.fd);
	this->_Client.insert(std::make_pair(newClient.fd, client));
	return (EXIT_SUCCESS);
}

int	Server::clientquittingServer(int index, char* buffer)
{
	*buffer = '\0';
	std::map<int, Client>::iterator it = this->_Client.find(this->_Fds[index].fd);
	close(this->_Fds[index].fd);
	if (it != this->_Client.end())
	{
		// Remove client from their channel if they're in one
		std::string channelName = it->second.GetChannelName();
		if (!channelName.empty() && this->_channels.find(channelName) != this->_channels.end())
		{
			this->_channels[channelName].RemoveClient(it->second.GetNickname());
			if (this->_channels[channelName].GetClientCount() == 0)
			{
				this->_channels.erase(channelName);
				std::cerr << YELLOW << "Channel " << channelName << " deleted (empty)" << RESET << std::endl;
			}
		}
		
		it->second.SetErase();
		// std::cout << YELLOW << "Client " << it->second.GetNickname() << " _toErase = " << it->second.GetErase() << RESET << std::endl;
		return (1);
	}
	else
	{
		std::cerr << RED << "Unknown client (FD: " << this->_Fds[index].fd << ") Quit Server" << RESET << std::endl;
		return (0);
	}

}

int	Server::clientSendingMessage(int index, char* buffer, size_t bytesSize)
{
	int quitFlag = 0;
	if (this->_Client.find(this->_Fds[index].fd) == this->_Client.end())
	{
		Client client(this->_Fds[index].fd);
		this->_Client.insert(std::make_pair(this->_Fds[index].fd, client));
	}
	this->_Client.find(this->_Fds[index].fd)->second.SetMsg(buffer, bytesSize);
	std::string ClientMsg = this->_Client.find(this->_Fds[index].fd)->second.GetMsg();
	size_t pos = ClientMsg.find("\r\n");
	if (pos == std::string::npos)
		pos = ClientMsg.find("\n");
	while (pos != std::string::npos)
	{
		std::string Msg = ClientMsg.substr(0, pos);
		if (!Msg.empty() && Msg[Msg.size() - 1] == '\r')
			Msg.erase(Msg.size() - 1);
		while (!Msg.empty() && (Msg[0] == '\r' || Msg[0] == '\n' || Msg[0] == '\t'))
			Msg.erase(0, 1);
		this->_Client.find(this->_Fds[index].fd)->second.SetEraseMsg(0, pos + 2);
		ClientMsg.erase(0, pos + 2);
		Client &client = this->_Client.find(this->_Fds[index].fd)->second;
		if (client.GetAuthenticated() == false)
		{
			if (Msg.substr(0, 4) == "PASS")
			{
				size_t spacePos = Msg.find(" ");
				if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
				{
					std::string pass = Msg.substr(spacePos + 1);
					if (pass == this->_password)
					{
						client.SetAuthenticated(true);
					}
					else
					{
						std::string serverName = "ircserv";
						std::string err = ":" + serverName + " 464 * PASS :Password incorrect\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
				}
				else
				{
					std::string serverName = "ircserv";
					std::string err = ":" + serverName + " 461 * PASS :Not enough parameters\r\n";
					send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				}
			}
			else
			{
				std::string serverName = "ircserv";
				std::string err = ":" + serverName + " 451 * :You have not registered\r\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
			}
		}
		else
		{
			if (client.GetNickname().empty() || client.GetUsername().empty())
			{
				if (Msg.substr(0, 4) == "NICK")
				{
					size_t spacePos = Msg.find(" ");
					if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
					{
						std::string newNick = Msg.substr(spacePos + 1);
						// Check if nickname is already in use
						bool nickExists = false;
						for (std::map<int, Client>::iterator it = this->_Client.begin(); it != this->_Client.end(); ++it)
						{
							if (it->second.GetNickname() == newNick && it->first != this->_Fds[index].fd)
							{
								nickExists = true;
								break;
							}
						}
						if (nickExists)
						{
							std::string serverName = "ircserv";
							std::string err = ":" + serverName + " 433 * " + newNick + " :Nickname is already in use\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
						else
						{
							client.SetNickname(newNick);
						}
					}
					else
					{
						std::string serverName = "ircserv";
						std::string nick = client.GetNickname();
						if (nick.empty())
							nick = "*";
						std::string err = ":" + serverName + " 431 " + nick + " :No nickname given\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
				}
				else if (Msg.substr(0, 4) == "USER")
				{
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					if (nick.empty())
						nick = "*";
					size_t firstSpace = Msg.find(" ");
					if (firstSpace != std::string::npos && firstSpace + 1 < Msg.length())
					{
						std::string rest = Msg.substr(firstSpace + 1);
						size_t secondSpace = rest.find(" ");
						if (secondSpace != std::string::npos && secondSpace + 1 < rest.length())
						{
							std::string username = rest.substr(0, secondSpace);
							rest = rest.substr(secondSpace + 1);
							size_t thirdSpace = rest.find(" ");
							if (thirdSpace != std::string::npos && thirdSpace + 1 < rest.length())
							{
								std::string mode = rest.substr(0, thirdSpace);
								rest = rest.substr(thirdSpace + 1);
								// Now rest should be "<unused> :<realname>"
								// Find the colon that marks the realname start
								size_t colonPos = rest.find(":");
								// The colon should be at the start or after the unused parameter
								if (colonPos == std::string::npos)
								{
									std::string err = ":" + serverName + " 461 " + nick + " USER :Not enough parameters\r\n";
									send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								}
								else if (colonPos + 1 >= rest.length())
								{
									std::string err = ":" + serverName + " 461 " + nick + " USER :Not enough parameters\r\n";
									send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								}
								else
								{
									// Extract realname after the colon
									std::string realname = rest.substr(colonPos + 1);
									client.SetUsername(username);
									client.SetRealname(realname);
								}
							}
							else
							{
								std::string err = ":" + serverName + " 461 " + nick + " USER :Not enough parameters\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							}
						}
						else
						{
							std::string err = ":" + serverName + " 461 " + nick + " USER :Not enough parameters\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
					}
					else
					{
						std::string err = ":" + serverName + " 461 " + nick + " USER :Not enough parameters\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
				}
				else
				{
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					if (nick.empty())
						nick = "*";
					std::string err = ":" + serverName + " 451 " + nick + " :You have not registered\r\n";
					send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				}
				if (!client.GetNickname().empty() && !client.GetUsername().empty())
				{
					std::cerr << GREEN << client.GetNickname() << " Joined Server" << RESET << std::endl;
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					
					std::string welcome = ":" + serverName + " 001 " + nick + " :Welcome to the IRC network " + nick + "\r\n";
					send(this->_Fds[index].fd, welcome.c_str(), welcome.size(), 0);
					
					std::string yourhost = ":" + serverName + " 002 " + nick + " :Your host is " + serverName + ", running version 1.0\r\n";
					send(this->_Fds[index].fd, yourhost.c_str(), yourhost.size(), 0);
					
					std::string created = ":" + serverName + " 003 " + nick + " :This server was created Mon Feb 27 2026\r\n";
					send(this->_Fds[index].fd, created.c_str(), created.size(), 0);
					
					std::string myinfo = ":" + serverName + " 004 " + nick + " " + serverName + " 1.0 ao iklmnst\r\n";
					send(this->_Fds[index].fd, myinfo.c_str(), myinfo.size(), 0);
					
					std::string ok = "Use HELP to see available commands.\n";
					send(this->_Fds[index].fd, ok.c_str(), ok.size(), 0);
				}
			}
			else
			{
				if (Msg == "HELP")
				{
					std::string help = "\n";
					help += "========== AVAILABLE COMMANDS (RFC 2812) ==========\n\n";
					help += "--- CONNECTION COMMANDS ---\n";
					help += "NICK <nickname>                    - Set or change your nickname\n";
					help += "USER <user> <mode> <unused> :<realname> - Set username and realname (: required)\n";
					help += "QUIT :<message>                    - Disconnect from server (: required)\n\n";
					help += "--- CHANNEL OPERATIONS ---\n";
					help += "JOIN <channel> [key]               - Join a channel (or create if doesn't exist)\n";
					help += "PART <channel>                     - Leave a channel\n";
					help += "NAMES <channel>                    - List users in a channel\n";
					help += "LIST                               - List all channels\n";
					help += "TOPIC <channel> [:<text>]          - View/set channel topic (: required to set)\n\n";
					help += "--- CHANNEL MANAGEMENT (Moderator Only) ---\n";
					help += "MODE <channel> <modes> [params]    - Set channel modes\n";
					help += "  +i / -i                          - Invite-only mode\n";
					help += "  +t / -t                          - Topic restricted (only moderator can set)\n";
					help += "  +k <key> / -k                    - Set/remove channel password\n";
					help += "  +l <limit> / -l                  - Set/remove user limit\n";
					help += "  +o <user> / -o <user>            - Grant/remove moderator privileges\n";
					help += "  Example: MODE #channel +tk mykey  - Set protected +t and +k modes\n";
					help += "KICK <channel> <user>              - Remove user from channel (moderator only)\n";
					help += "INVITE <user> <channel>            - Invite user to channel (moderator only)\n\n";
					help += "--- MESSAGING ---\n";
					help += "PRIVMSG <target> :<message>        - Send message to user or channel (: required)\n";
					help += "  <target> can be a nick or #channel\n\n";
					help += "--- GENERAL ---\n";
					help += "HELP                               - Show this help message\n";
					help += "====================================================\n";
					send(this->_Fds[index].fd, help.c_str(), help.size(), 0);
				}
				else if (Msg.substr(0, 4) == "JOIN")
				{
					size_t spacePos = Msg.find(" ");
					if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
					{
						std::string rest = Msg.substr(spacePos + 1);
						size_t spacePos2 = rest.find(" ");
						std::string channelName = (spacePos2 != std::string::npos) ? rest.substr(0, spacePos2) : rest;
						std::string key = (spacePos2 != std::string::npos && spacePos2 + 1 < rest.length()) ? rest.substr(spacePos2 + 1) : "";
						if (!channelName.empty() && channelName[0] == '#')
							channelName.erase(0, 1);
						if (this->_channels.find(channelName) != this->_channels.end())
						{
							if (this->_channels[channelName].ClientExists(client.GetNickname()))
							{
								std::string serverName = "ircserv";
								std::string err = ":" + serverName + " 443 " + client.GetNickname() + " " + client.GetNickname() + " #" + channelName + " :User is already on channel\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								continue;
							}
							if (this->_channels[channelName].GetUserLimit() > 0
								&& this->_channels[channelName].GetClientCount() >= this->_channels[channelName].GetUserLimit())
							{
								std::string err = ":ircserv 471 " + client.GetNickname() + " #" + channelName + " :Cannot join channel (+l)\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								continue;
							}
							if (!this->_channels[channelName].GetKey().empty() && key != this->_channels[channelName].GetKey())
							{
								std::string err = ":ircserv 475 " + client.GetNickname() + " #" + channelName + " :Cannot join channel (+k)\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								continue;
							}
							if (this->_channels[channelName].IsInviteOnly()
								&& this->_channels[channelName].GetModerator() != client.GetNickname()
								&& !this->_channels[channelName].ClientExists(client.GetNickname()))
							{
								std::string err = ":ircserv 473 " + client.GetNickname() + " #" + channelName + " :Cannot join channel (+i)\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								continue;
							}
						}
						std::string oldChannel = client.GetChannelName();
						if (!oldChannel.empty() && this->_channels.find(oldChannel) != this->_channels.end())
						{
							this->_channels[oldChannel].RemoveClient(client.GetNickname());
							if (this->_channels[oldChannel].GetClientCount() == 0)
							{
								this->_channels.erase(oldChannel);
							}
						}
						client.SetChannelName(channelName);
						if (this->_channels.find(channelName) == this->_channels.end())
						{
							Channel newChannel(channelName);
							this->_channels[channelName] = newChannel;
							this->_channels[channelName].SetModerator(client.GetNickname());
							std::cerr << MAGENTA << client.GetNickname() << " is now moderator of #" << channelName << RESET << std::endl;
						}
						else if (this->_channels[channelName].GetClientCount() == 0)
						{
							this->_channels[channelName].SetModerator(client.GetNickname());
							std::cerr << MAGENTA << client.GetNickname() << " is now moderator of #" << channelName << RESET << std::endl;
						}
						this->_channels[channelName].AddClient(client.GetNickname(), &client);
					std::string joinMsg = ":" + client.GetNickname() + "!" + client.GetUsername() + "@localhost JOIN #" + channelName + "\r\n";
						send(this->_Fds[index].fd, joinMsg.c_str(), joinMsg.size(), 0);
						std::cerr << GREEN << client.GetNickname() << " joined channel " << channelName << RESET << std::endl;
					}
					else
					{
						std::string err = ":ircserv 461 " + client.GetNickname() + " JOIN :Not enough parameters\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
				}
				else if (Msg.substr(0, 5) == "NAMES")
				{
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					size_t spacePos = Msg.find(" ");
					if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
					{
						std::string channelName = Msg.substr(spacePos + 1);
						if (!channelName.empty() && channelName[0] == '#')
							channelName.erase(0, 1);
						
						if (this->_channels.find(channelName) == this->_channels.end())
						{
							std::string err = ":" + serverName + " 403 " + nick + " #" + channelName + " :No such channel\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
						else
						{
							std::string usersLine = "";
							const std::map<std::string, Client*>& channelClients = this->_channels[channelName].GetAllClients();
							for (std::map<std::string, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
							{
								if (!usersLine.empty())
									usersLine += " ";
								usersLine += it->first;
							}
							std::string response = ":" + serverName + " 353 " + nick + " = #" + channelName + " :" + usersLine + "\r\n";
							send(this->_Fds[index].fd, response.c_str(), response.size(), 0);
							std::string end = ":" + serverName + " 366 " + nick + " #" + channelName + " :End of /NAMES list.\r\n";
							send(this->_Fds[index].fd, end.c_str(), end.size(), 0);
						}
					}
					else
					{
						std::string err = ":" + serverName + " 461 " + nick + " NAMES :Not enough parameters\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
				}
				else if (Msg == "LIST")
				{
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					std::string start = ":" + serverName + " 321 " + nick + " Channel :Users Name\r\n";
					send(this->_Fds[index].fd, start.c_str(), start.size(), 0);
					for (std::map<std::string, Channel>::iterator it = this->_channels.begin(); it != this->_channels.end(); ++it)
					{
						std::string line = ":" + serverName + " 322 " + nick + " #" + it->first + " " + intToString(it->second.GetClientCount()) + " :\r\n";
						send(this->_Fds[index].fd, line.c_str(), line.size(), 0);
					}
					std::string end = ":" + serverName + " 323 " + nick + " :End of /LIST\r\n";
					send(this->_Fds[index].fd, end.c_str(), end.size(), 0);
				}
				else if (Msg.substr(0, 4) == "MODE")
				{
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					size_t spacePos = Msg.find(" ");
					if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
					{
						std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
					else
					{
						std::string rest = Msg.substr(spacePos + 1);
						size_t spacePos2 = rest.find(" ");
						std::string channelName = (spacePos2 != std::string::npos) ? rest.substr(0, spacePos2) : rest;
						
						if (!channelName.empty() && channelName[0] == '#')
							channelName.erase(0, 1);
						
						if (this->_channels.find(channelName) == this->_channels.end())
						{
							std::string err = ":" + serverName + " 403 " + nick + " #" + channelName + " :No such channel\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
						else if (spacePos2 == std::string::npos)
						{
							std::string modes = "+";
							if (this->_channels[channelName].IsInviteOnly())
								modes += "i";
							if (this->_channels[channelName].IsTopicRestricted())
								modes += "t";
							if (!this->_channels[channelName].GetKey().empty())
								modes += "k";
							if (this->_channels[channelName].GetUserLimit() > 0)
								modes += "l";
							std::string reply = ":" + serverName + " 324 " + nick + " #" + channelName + " " + modes + "\r\n";
							send(this->_Fds[index].fd, reply.c_str(), reply.size(), 0);
						}
						else if (this->_channels[channelName].GetModerator() != nick)
						{
							std::string err = ":" + serverName + " 482 " + nick + " #" + channelName + " :You're not channel operator\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
						else
						{
							std::string modeParams = rest.substr(spacePos2 + 1);
							std::stringstream ss(modeParams);
							std::string modeStr;
							ss >> modeStr;
							if (modeStr.empty())
							{
								std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								continue;
							}
							std::vector<std::string> params;
							std::string param;
							while (ss >> param)
								params.push_back(param);
							bool adding = true;
							size_t paramIndex = 0;
							bool modeError = false;
							for (size_t i = 0; i < modeStr.size(); ++i)
							{
								char m = modeStr[i];
								if (m == '+')
								{
									adding = true;
									continue;
								}
								if (m == '-')
								{
									adding = false;
									continue;
								}
								switch (m)
								{
									case 'i':
										this->_channels[channelName].SetInviteOnly(adding);
										break;
									case 't':
										this->_channels[channelName].SetTopicRestricted(adding);
										break;
									case 'k':
										if (adding)
										{
											if (paramIndex >= params.size())
											{
												std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
												send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
												modeError = true;
											}
											else
											{
												this->_channels[channelName].SetKey(params[paramIndex++]);
											}
										}
										else
										{
											this->_channels[channelName].ClearKey();
										}
										break;
									case 'o':
										if (paramIndex >= params.size())
										{
											std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
											send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
											modeError = true;
											break;
										}
										{
											std::string target = params[paramIndex++];
											if (!this->_channels[channelName].ClientExists(target))
											{
												std::string err = ":" + serverName + " 441 " + nick + " " + target + " #" + channelName + " :They are not on that channel\r\n";
												send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
												modeError = true;
												break;
											}
											if (adding)
												this->_channels[channelName].SetModerator(target);
											else if (this->_channels[channelName].GetModerator() == target)
												this->_channels[channelName].SetModerator("");
										}
										break;
									case 'j':
									case 'l':
										if (adding)
										{
											if (paramIndex >= params.size())
											{
												std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
												send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
												modeError = true;
											}
											else
											{
												int limit = std::atoi(params[paramIndex++].c_str());
												if (limit <= 0)
												{
													std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
													send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
													modeError = true;
												}
												else
												{
													this->_channels[channelName].SetUserLimit(limit);
												}
											}
										}
										else
										{
											this->_channels[channelName].ClearUserLimit();
										}
										break;
									default:
									{
										std::string err = ":" + serverName + " 472 " + nick + " " + m + " :is unknown mode char to me\r\n";
										send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
										modeError = true;
										break;
									}
								}
								if (modeError)
									break;
							}
						}
					}
				}
				else if (Msg.substr(0, 5) == "TOPIC")
				{
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					size_t spacePos = Msg.find(" ");
					if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
					{
						std::string err = ":" + serverName + " 461 " + nick + " TOPIC :Not enough parameters\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
					else
					{
						std::string rest = Msg.substr(spacePos + 1);
						size_t spacePos2 = rest.find(" ");
						std::string channelName = (spacePos2 != std::string::npos) ? rest.substr(0, spacePos2) : rest;
						
						if (!channelName.empty() && channelName[0] == '#')
							channelName.erase(0, 1);
						
						if (this->_channels.find(channelName) == this->_channels.end())
						{
							std::string err = ":" + serverName + " 403 " + nick + " #" + channelName + " :No such channel\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
						else if (spacePos2 == std::string::npos)
						{
							// Query topic
							std::string topic = this->_channels[channelName].GetTopic();
							if (topic.empty())
							{
								std::string response = ":" + serverName + " 331 " + nick + " #" + channelName + " :No topic is set\r\n";
								send(this->_Fds[index].fd, response.c_str(), response.size(), 0);
							}
							else
							{
								std::string response = ":" + serverName + " 332 " + nick + " #" + channelName + " :" + topic + "\r\n";
								send(this->_Fds[index].fd, response.c_str(), response.size(), 0);
							}
						}
						else
						{
							// Set topic
							if (this->_channels[channelName].IsTopicRestricted()
								&& this->_channels[channelName].GetModerator() != nick)
							{
								std::string err = ":" + serverName + " 482 " + nick + " #" + channelName + " :You're not channel operator\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							}
							else
							{
								std::string newTopic = rest.substr(spacePos2 + 1);
								if (newTopic.empty() || newTopic[0] != ':')
								{
									std::string err = ":" + serverName + " 461 " + nick + " TOPIC :Not enough parameters\r\n";
									send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
									continue;
								}
								newTopic.erase(0, 1);
								
								this->_channels[channelName].SetTopic(newTopic);
								std::string confirmMsg = ":" + nick + " TOPIC #" + channelName + " :" + newTopic + "\r\n";
								send(this->_Fds[index].fd, confirmMsg.c_str(), confirmMsg.size(), 0);
								std::cerr << MAGENTA << nick << " changed topic of #" << channelName << " to: " << newTopic << RESET << std::endl;
							}
						}
					}
				}
				else if (Msg.substr(0, 7) == "PRIVMSG")
				{
					std::string serverName = "ircserv";
					std::string senderNick = client.GetNickname();
					size_t spacePos = Msg.find(" ");
					if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
					{
						std::string err = ":" + serverName + " 461 " + senderNick + " PRIVMSG :Not enough parameters\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
					else
					{
						std::string rest = Msg.substr(spacePos + 1);
						size_t spacePos2 = rest.find(" ");
						if (spacePos2 == std::string::npos || spacePos2 + 1 >= rest.length())
						{
							std::string err = ":" + serverName + " 461 " + senderNick + " PRIVMSG :Not enough parameters\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
						else
						{
							std::string target = rest.substr(0, spacePos2);
							std::string message = rest.substr(spacePos2 + 1);
							
							if (message.empty() || message[0] != ':')
							{
								std::string err = ":" + serverName + " 412 " + senderNick + " :No text to send\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								continue;
							}
							message.erase(0, 1);
							
							// Check if target is a channel or a user
							bool isChannel = (target[0] == '#');
							
							if (isChannel)
							{
								// Message to channel (RFC 2812)
								if (target.length() > 1)
									target.erase(0, 1);
								
								if (this->_channels.find(target) == this->_channels.end())
								{
									std::string err = ":" + serverName + " 403 " + senderNick + " #" + target + " :No such channel\r\n";
									send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								}
								else
								{
									std::string privmsgLine = ":" + senderNick + "!" + client.GetUsername() + "@localhost PRIVMSG #" + target + " :" + message + "\r\n";
									const std::map<std::string, Client*>& channelClients = this->_channels[target].GetAllClients();
									for (std::map<std::string, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
									{
										if (it->second && it->second->GetFd() != this->_Fds[index].fd)
											send(it->second->GetFd(), privmsgLine.c_str(), privmsgLine.size(), 0);
									}
									std::cout << CYAN << "[#" << target << "] " << senderNick << ": " << message << RESET << std::endl;
								}
							}
							else
							{
								// Message to user (RFC 2812)
								Client* targetClient = NULL;
								for (std::map<int, Client>::iterator it = this->_Client.begin(); it != this->_Client.end(); ++it)
								{
									if (it->second.GetNickname() == target)
									{
										targetClient = &it->second;
										break;
									}
								}
								
								if (!targetClient)
								{
									std::string err = ":" + serverName + " 401 " + senderNick + " " + target + " :No such nick\r\n";
									send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								}
								else
								{
									std::string privmsgLine = ":" + senderNick + "!" + client.GetUsername() + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
									send(targetClient->GetFd(), privmsgLine.c_str(), privmsgLine.size(), 0);
									std::cout << CYAN << "[PM] " << senderNick << " -> " << target << ": " << message << RESET << std::endl;
								}
							}
						}
					}
				}
				else if (Msg.substr(0, 4) == "PART")
				{
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					size_t spacePos = Msg.find(" ");
					if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
					{
						std::string channelToQuit = Msg.substr(spacePos + 1);
						if (!channelToQuit.empty() && channelToQuit[0] == '#')
							channelToQuit.erase(0, 1);
						if (this->_channels.find(channelToQuit) == this->_channels.end())
						{
							std::string err = ":" + serverName + " 403 " + nick + " #" + channelToQuit + " :No such channel\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
						else if (!this->_channels[channelToQuit].ClientExists(nick))
						{
							std::string err = ":" + serverName + " 442 " + nick + " #" + channelToQuit + " :You're not on that channel\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
						else
						{
							std::string partLine = ":" + nick + " PART #" + channelToQuit + "\r\n";
							const std::map<std::string, Client*>& channelClients = this->_channels[channelToQuit].GetAllClients();
							for (std::map<std::string, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
							{
								if (it->second)
									send(it->second->GetFd(), partLine.c_str(), partLine.size(), 0);
							}
							this->_channels[channelToQuit].RemoveClient(nick);
							if (this->_channels[channelToQuit].GetClientCount() == 0)
							{
								this->_channels.erase(channelToQuit);
								std::cerr << YELLOW << "Channel " << channelToQuit << " deleted (empty)" << RESET << std::endl;
							}
							client.SetChannelName("");
							std::cerr << YELLOW << nick << " left channel " << channelToQuit << RESET << std::endl;
						}
					}
					else
					{
						std::string err = ":" + serverName + " 461 " + nick + " PART :Not enough parameters\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
				}
				else if (Msg.substr(0, 4) == "QUIT")
				{
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					size_t spacePos = Msg.find(" ");
					if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
					{
						std::string err = ":" + serverName + " 461 " + nick + " QUIT :Not enough parameters\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
					else
					{
						std::string message = Msg.substr(spacePos + 1);
						if (message.empty() || message[0] != ':')
						{
							std::string err = ":" + serverName + " 461 " + nick + " QUIT :Not enough parameters\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							continue;
						}
						message.erase(0, 1);
						std::string nick = client.GetNickname();
					std::string quitLine = ":" + nick + "!" + client.GetUsername() + "@localhost QUIT :" + message + "\r\n";
						for (std::map<int, Client>::iterator it = this->_Client.begin(); it != this->_Client.end(); ++it)
						{
							if (!it->second.GetNickname().empty())
								send(it->second.GetFd(), quitLine.c_str(), quitLine.size(), 0);
						}
						close(this->_Fds[index].fd);
						client.SetErase();
						quitFlag = 1;
						break;
					}
				}
				else if (Msg.substr(0, 4) == "KICK")
				{
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					size_t spacePos = Msg.find(" ");
					if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
					{
						std::string err = ":" + serverName + " 461 " + nick + " KICK :Not enough parameters\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
					else
					{
						std::string rest = Msg.substr(spacePos + 1);
						size_t spacePos2 = rest.find(" ");
						if (spacePos2 == std::string::npos || spacePos2 + 1 >= rest.length())
						{
							std::string err = ":" + serverName + " 461 " + nick + " KICK :Not enough parameters\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
						else
						{
							std::string channelName = rest.substr(0, spacePos2);
							std::string targetNick = rest.substr(spacePos2 + 1);
							
							if (!channelName.empty() && channelName[0] == '#')
								channelName.erase(0, 1);
							
							if (this->_channels.find(channelName) == this->_channels.end())
							{
								std::string err = ":" + serverName + " 403 " + nick + " #" + channelName + " :No such channel\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							}
							else if (this->_channels[channelName].GetModerator() != nick)
							{
								std::string err = ":" + serverName + " 482 " + nick + " #" + channelName + " :You're not channel operator\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							}
							else if (!this->_channels[channelName].ClientExists(targetNick))
							{
								std::string err = ":" + serverName + " 441 " + nick + " " + targetNick + " #" + channelName + " :They are not on that channel\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							}
							else
							{
								Client* targetClient = this->_channels[channelName].GetClient(targetNick);
								if (targetClient)
								{
									std::string kickLine = ":" + nick + "!" + client.GetUsername() + "@localhost KICK #" + channelName + " " + targetNick + "\r\n";
									const std::map<std::string, Client*>& channelClients = this->_channels[channelName].GetAllClients();
									for (std::map<std::string, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
									{
										if (it->second)
											send(it->second->GetFd(), kickLine.c_str(), kickLine.size(), 0);
									}
									targetClient->SetChannelName("");
									this->_channels[channelName].RemoveClient(targetNick);
									std::cerr << RED << nick << " kicked " << targetNick << " from #" << channelName << RESET << std::endl;
								}
							}
						}
					}
				}
				else if (Msg.substr(0, 6) == "INVITE")
				{
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					size_t spacePos = Msg.find(" ");
					if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
					{
						std::string err = ":" + serverName + " 461 " + nick + " INVITE :Not enough parameters\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
					else
					{
						std::string rest = Msg.substr(spacePos + 1);
						size_t spacePos2 = rest.find(" ");
						if (spacePos2 == std::string::npos || spacePos2 + 1 >= rest.length())
						{
							std::string err = ":" + serverName + " 461 " + nick + " INVITE :Not enough parameters\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
						else
						{
							std::string targetNick = rest.substr(0, spacePos2);
							std::string targetChannel = rest.substr(spacePos2 + 1);
							
							if (!targetChannel.empty() && targetChannel[0] == '#')
								targetChannel.erase(0, 1);
							
							if (this->_channels.find(targetChannel) == this->_channels.end())
							{
								std::string err = ":" + serverName + " 403 " + nick + " #" + targetChannel + " :No such channel\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							}
							else if (this->_channels[targetChannel].GetModerator() != nick)
							{
								std::string err = ":" + serverName + " 482 " + nick + " #" + targetChannel + " :You're not channel operator\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							}
							else
							{
								Client* targetClient = NULL;
								for (std::map<int, Client>::iterator it = this->_Client.begin(); it != this->_Client.end(); ++it)
								{
									if (it->second.GetNickname() == targetNick)
									{
										targetClient = &it->second;
										break;
									}
								}
								
								if (!targetClient)
								{
									std::string err = ":" + serverName + " 401 " + nick + " " + targetNick + " :No such nick\r\n";
									send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								}
								else if (this->_channels[targetChannel].ClientExists(targetNick))
								{
									std::string err = ":" + serverName + " 443 " + nick + " " + targetNick + " #" + targetChannel + " :User is already on channel\r\n";
									send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								}
								else
								{
									std::string inviteLine = ":" + nick + "!" + client.GetUsername() + "@localhost INVITE " + targetNick + " #" + targetChannel + "\r\n";
									send(targetClient->GetFd(), inviteLine.c_str(), inviteLine.size(), 0);
									this->_channels[targetChannel].AddClient(targetNick, targetClient);
									targetClient->SetChannelName(targetChannel);
									std::cerr << GREEN << nick << " invited " << targetNick << " to #" << targetChannel << RESET << std::endl;
								}
							}
						}
					}
				}
				else if (!client.GetChannelName().empty())
				{
					std::string channelName = client.GetChannelName();
					if (this->_channels.find(channelName) != this->_channels.end())
					{
						std::string fullMsg = client.GetNickname() + ": " + Msg + "\n";
						const std::map<std::string, Client*>& channelClients = this->_channels[channelName].GetAllClients();
						for (std::map<std::string, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
						{
							if (it->second && it->second->GetFd() != this->_Fds[index].fd)
								send(it->second->GetFd(), fullMsg.c_str(), fullMsg.size(), 0);
						}
						std::cout << CYAN << "[" << channelName << "] " << client.GetNickname() << ": " << Msg << RESET << std::endl;
					}
				}
				else
				{
					std::string serverName = "ircserv";
					std::string nick = client.GetNickname();
					std::string command = Msg;
					size_t spacePos = Msg.find(" ");
					if (spacePos != std::string::npos)
						command = Msg.substr(0, spacePos);
					std::string err = ":" + serverName + " 421 " + nick + " " + command + " :Unknown command\r\n";
					send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				}
			}
		}
		pos = ClientMsg.find("\r\n");
		if (pos == std::string::npos)
			pos = ClientMsg.find("\n");
	}
	memset(buffer, 0, 1024);
	return (quitFlag);
}

void Server::disconnectClient(int nbrClient)
{
	if (nbrClient == 0)
		return ;
	for (size_t i = 0; i < this->_Fds.size(); i++)
	{
		if (nbrClient == 0)
			return ;
		std::map<int, Client>::iterator it = this->_Client.find(this->_Fds[i].fd);
		if (it != this->_Client.end())
		{
			if (it->second.GetErase() == true)
			{
				std::cerr << RED << it->second.GetNickname() << " Quit Server" << RESET << std::endl;
				// close(it->first);
				this->_Client.erase(it->first);
				pollfd lastlistfd = this->_Fds.back();
				this->_Fds.at(i) = lastlistfd;
				this->_Fds.pop_back();
				i--;
				nbrClient--;
			}
		}
	}
	return ;
}

// void	Server::signalHandling()
// {
// 	signal(SIGINT, SIGQUIT);
// }

struct pollfd& Server::operator[](size_t index)
{
	if (this->_Fds.size() == 0 || index > (this->_Fds.size() - 1))
	{
		static struct pollfd invalid = {-1, 0, 0};
		return (invalid);
	}	
	return (this->_Fds[index]);
}

std::ostream& operator<<(std::ostream &out, const Server &other)
{
	std::cout << other.GetServerFd();
	return (out);
}