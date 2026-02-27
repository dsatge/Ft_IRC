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

std::string Server::intToString(int num)
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
							perror("recv");
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

void	Server::initCmds()
{
	_cmds["HELP"] = &Server::cmdHelp;
	_cmds["JOIN"] = &Server::cmdJoin;
	_cmds["NAMES"] = &Server::cmdNames;
	_cmds["LIST"] = &Server::cmdList;
	_cmds["MODE"] = &Server::cmdMode;
	_cmds["TOPIC"] = &Server::cmdTopic;
	_cmds["PRIVMSG"] = &Server::cmdPrivmsg;
	_cmds["PART"] = &Server::cmdPart;
	_cmds["QUIT"] = &Server::cmdQuit;
	_cmds["KICK"] = &Server::cmdKick;
	_cmds["INVITE"] = &Server::cmdInvite;
}

std::string	getCmdFromMsg(std::string Msg)
{
	std::string cmd;

	size_t start = Msg.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return (Msg);
	size_t spacePos = Msg.find(' ');
	cmd = Msg.substr(start, spacePos);
	return (cmd);
}

int Server::cmdHandler(std::string Msg, int index, Client &client)
{
	std::string cmd = getCmdFromMsg(Msg);
	std::map<std::string, cmdPtr>::iterator it = _cmds.find(cmd);
	int	quitReturn = 0;

	if (it != _cmds.end())
	{
		cmdPtr func = it->second;
		quitReturn = (this->*func)(Msg, index, client);
	}
	else if (!client.GetChannelName().empty())
		msgChannel(Msg, index, client);
	else
		std::cerr << "Command not found: " << Msg << std::endl;
	return (quitReturn);
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
						std::string ok = "Password accepted.\n";
						send(this->_Fds[index].fd, ok.c_str(), ok.size(), 0);
					}
					else
					{
						std::string err = "ERROR: bad password\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
				}
				else
				{
					std::string err = "ERROR: invalid authentication command\n";
					send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				}
			}
			else
			{
				std::string err = "ERROR: not authenticated\n";
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
						std::string err = "ERROR: invalid nickname command\n";
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
								size_t colonPos = rest.find(":");
								if (colonPos != std::string::npos && colonPos + 1 < rest.length())
								{
									std::string realname = rest.substr(colonPos + 1);
									client.SetUsername(username);
									client.SetRealname(realname);
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
						std::string err = ":" + serverName + " 461 " + nick + " USER :Not enough parameters\r\n";
						send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					}
				}
				else
				{
					std::string err = "ERROR: need NICK and USER\n";
					send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				}
				if (!client.GetNickname().empty() && !client.GetUsername().empty())
				{
					std::cerr << GREEN << client.GetNickname() << " Joined Server" << RESET << std::endl;
					std::string ok = "Welcome! Use HELP to see available commands.\n";
					send(this->_Fds[index].fd, ok.c_str(), ok.size(), 0);
				}
			}
			else
			{
				initCmds();
				cmdHandler(Msg, index, client);
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