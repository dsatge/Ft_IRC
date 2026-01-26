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

std::vector<struct pollfd> Server::GetFdsList() const
{
	return (this->_Fds);
}


int	Server::Size()
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