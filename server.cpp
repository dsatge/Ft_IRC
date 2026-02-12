# include "server.hpp"

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
	fcntl(this->_serverFd, F_SETFL, O_NONBLOCK);
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

int	Server::pollLoop()
{
	this->_Fds[0].events = POLLIN;
	while (1)
	{
		int pollStatus = poll(&this->_Fds[0], this->_Fds.size(), -1);
		if (pollStatus < 0)
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
							std::cout << YELLOW << "~ ELSE ~" << RESET << std::endl;
					}
				}
				this->disconnectClient(flagDisconnect);
				flagDisconnect = 0;
			}
		}
	}
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
	std::string prompt = "Password: ";
	send(newClient.fd, prompt.c_str(), prompt.size(), 0);
	return (EXIT_SUCCESS);
}

int	Server::clientquittingServer(int index, char* buffer)
{
	*buffer = '\0';
	std::map<int, Client>::iterator it = this->_Client.find(this->_Fds[index].fd);
	close(this->_Fds[index].fd);
	if (it != this->_Client.end())
	{
		it->second.SetErase();
		std::cout << YELLOW << "Client " << it->second.GetNickname() << " _toErase = " << it->second.GetErase() << RESET << std::endl;
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
			if (Msg == this->_password)
			{
				client.SetAuthenticated(true);
				std::string ok = "Password accepted.\n";
				send(this->_Fds[index].fd, ok.c_str(), ok.size(), 0);
				std::string nickPrompt = "Nickname: ";
				send(this->_Fds[index].fd, nickPrompt.c_str(), nickPrompt.size(), 0);
			}
			else
			{
				std::string err = "ERROR: bad password\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				std::string prompt = "Password: ";
				send(this->_Fds[index].fd, prompt.c_str(), prompt.size(), 0);
			}
		}
		else
		{
			if (client.GetNickname().empty())
			{
				if (!Msg.empty())
				{
					client.SetNickname(Msg);
					std::cerr << GREEN << client.GetNickname() << " Joined Server" << RESET << std::endl;
					std::string ok = "Welcome! Use JOIN <channel> or STATUS to list users.\n";
					send(this->_Fds[index].fd, ok.c_str(), ok.size(), 0);
				}
				else
				{
					std::string nickPrompt = "Nickname: ";
					send(this->_Fds[index].fd, nickPrompt.c_str(), nickPrompt.size(), 0);
				}
			}
			else
			{
				if (Msg.substr(0, 4) == "JOIN")
				{
					size_t spacePos = Msg.find(" ");
					if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
					{
						std::string channelName = Msg.substr(spacePos + 1);
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
						std::string joinMsg = "Joined channel " + channelName + "\n";
						send(this->_Fds[index].fd, joinMsg.c_str(), joinMsg.size(), 0);
						std::cerr << GREEN << client.GetNickname() << " joined channel " << channelName << RESET << std::endl;
					}
					else
					{
						std::string err = "Usage: JOIN <channel_name>\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
				}
				else if (Msg.substr(0, 6) == "STATUS")
				{
					size_t spacePos = Msg.find(" ");
					if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
					{
						std::string channelName = Msg.substr(spacePos + 1);
						if (client.GetChannelName() == channelName)
						{
							if (this->_channels.find(channelName) != this->_channels.end())
							{
								std::string response = "\n=== USERS IN #" + channelName + " ===\n";
								const std::map<std::string, Client*>& channelClients = this->_channels[channelName].GetAllClients();
								for (std::map<std::string, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
								{
									response += "- " + it->first + "\n";
								}
								response += "=======================\n";
								send(this->_Fds[index].fd, response.c_str(), response.size(), 0);
							}
						}
						else
						{
							std::string err = "You must be in channel " + channelName + " to see its users.\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
					}
					else
					{
						std::string response = "\n=== USERS ON SERVER ===\n";
						for (std::map<int, Client>::iterator it = this->_Client.begin(); it != this->_Client.end(); ++it)
						{
							if (!it->second.GetNickname().empty())
								response += "- " + it->second.GetNickname() + "\n";
						}
						response += "=======================\n";
						send(this->_Fds[index].fd, response.c_str(), response.size(), 0);
					}
				}
				else if (Msg == "CHANNEL" || Msg == "CHANNELS")
				{
					std::string response = "\n=== AVAILABLE CHANNELS ===\n";
					if (this->_channels.empty())
					{
						response += "No channels available.\n";
					}
					else
					{
						for (std::map<std::string, Channel>::iterator it = this->_channels.begin(); it != this->_channels.end(); ++it)
						{
							response += "- #" + it->first + " (" + intToString(it->second.GetClientCount()) + " users)\n";
						}
					}
					response += "==========================\n";
					send(this->_Fds[index].fd, response.c_str(), response.size(), 0);
				}
				else if (Msg.substr(0, 4) == "QUIT")
				{
					size_t spacePos = Msg.find(" ");
					if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
					{
						std::string channelToQuit = Msg.substr(spacePos + 1);
						if (client.GetChannelName() == channelToQuit)
						{
							if (this->_channels.find(channelToQuit) != this->_channels.end())
							{
								this->_channels[channelToQuit].RemoveClient(client.GetNickname());
								if (this->_channels[channelToQuit].GetClientCount() == 0)
								{
									this->_channels.erase(channelToQuit);
									std::cerr << YELLOW << "Channel " << channelToQuit << " deleted (empty)" << RESET << std::endl;
								}
							}
							client.SetChannelName("");
							std::string quitMsg = "You left channel " + channelToQuit + "\n";
							send(this->_Fds[index].fd, quitMsg.c_str(), quitMsg.size(), 0);
							std::cerr << YELLOW << client.GetNickname() << " left channel " << channelToQuit << RESET << std::endl;
						}
						else
						{
							std::string err = "You are not in channel " + channelToQuit + "\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
					}
					else
					{
						std::string bye = "Goodbye!\n";
						send(this->_Fds[index].fd, bye.c_str(), bye.size(), 0);
						close(this->_Fds[index].fd);
						client.SetErase();
						quitFlag = 1;
						break;
					}
				}
				else if (Msg.substr(0, 4) == "KICK")
				{
					size_t spacePos = Msg.find(" ");
					if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
					{
						std::string targetNick = Msg.substr(spacePos + 1);
						std::string channelName = client.GetChannelName();
						if (channelName.empty())
						{
							std::string err = "You must be in a channel to kick someone.\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
						}
						else if (this->_channels.find(channelName) != this->_channels.end())
						{
							if (this->_channels[channelName].GetModerator() != client.GetNickname())
							{
								std::string err = "Only the moderator can kick users.\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							}
							else if (!this->_channels[channelName].ClientExists(targetNick))
							{
								std::string err = targetNick + " is not in this channel.\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							}
							else if (targetNick == client.GetNickname())
							{
								std::string err = "You cannot kick yourself.\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							}
							else
							{
								Client* targetClient = this->_channels[channelName].GetClient(targetNick);
								if (targetClient)
								{
									std::string kickMsg = "You have been kicked from #" + channelName + " by " + client.GetNickname() + "\n";
									send(targetClient->GetFd(), kickMsg.c_str(), kickMsg.size(), 0);
									targetClient->SetChannelName("");
									this->_channels[channelName].RemoveClient(targetNick);
									std::string confirmMsg = targetNick + " has been kicked from the channel.\n";
									send(this->_Fds[index].fd, confirmMsg.c_str(), confirmMsg.size(), 0);
									std::cerr << RED << client.GetNickname() << " kicked " << targetNick << " from #" << channelName << RESET << std::endl;
								}
							}
						}
					}
					else
					{
						std::string err = "Usage: KICK <nickname>\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
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
					std::string err = "You must JOIN a channel first. Use: JOIN <channel>\n";
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