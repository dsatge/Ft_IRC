# include "server.hpp"

Server::Server()
{
	return ;
}

Server::Server(std::string port)
{
	this->_port = atoi(port.c_str());
	return ;
}

Server::Server(const Server &other)
{
	this->_serverFd = other._serverFd;
	this->_port = other._port;
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
						ssize_t msg = recv(this->_Fds[index].fd, buffer, 1024, 0);
						if (msg == 0)
							flagDisconnect += clientquittingServer(index, buffer);
						if (msg > 0)
							clientSendingMessage(index, buffer);
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
	std::cerr << GREEN << "CLIENT JOINED SERVER" << RESET << std::endl;
	struct pollfd	newClient;
	newClient.fd = this->acceptFd(index);
	if (newClient.fd < 0)
		return (EXIT_FAILURE) ;
	newClient.events = POLLIN;
	newClient.revents = 0;
	this->AddSocketFds(newClient);
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
		return (1);
	}
	else
	{
		// this->_Fds[index].fd = -1;
		std::cout << RED << "CLIENT DISCONECT FROM SERVER" << RESET << std::endl;
		return (0);
	}

}

int	Server::clientSendingMessage(int index, char* buffer)
{
	Client client(this->_Fds[index].fd);
	this->_Client.insert(std::make_pair(this->_Fds[index].fd, client));
	this->_Client.find(index)->second.SetBuffer(buffer);
	std::cout << CYAN << this->_Client.find(index)->second.GetBuffer() << RESET;
	memset(buffer, 0, 1024);
	return (EXIT_SUCCESS);
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
				// close(it->first);
				this->_Client.erase(it->first);
				pollfd lastlistfd = this->_Fds.back();
				this->_Fds.at(i) = lastlistfd;
				this->_Fds.pop_back();
				i--;
				nbrClient--;
				std::cout << RED << "CLIENT DISCONECT FROM SERVER" << RESET << std::endl;
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