# include "client.hpp"

Client::Client(int fd) : _fd(fd)
{
	return ;
}

Client::Client(const Client &other)
{
	this->_fd = other._fd;
	this->_IP = other._IP;
	return ;
}

Client& Client::operator=(const Client &other)
{
	if (this != &other)
	{
		this->_fd = other._fd;
		this->_IP = other._IP;
	}
	return (*this);
}

Client::~Client()
{
	return ;
}

void Client::SetFd(int fd)
{
	this->_fd = fd;
	return ;
}

void Client::SetIP(std::string IP)
{
	this->_IP = IP;
	return ;
}

void Client::SetBuffer(char *buffer)
{
	this->_Buffer = buffer;
	return ;
}

std::string Client::GetBuffer() const
{
	std::stringstream ss;
	std::string str;
	std::getline(this->_Buffer);
	if (!this->_Buffer || this->_Buffer == NULL)
		return (str);
	ss << this->_Buffer;
	ss >> str;
	std::cerr << "_Buffer = " << this->_Buffer << " str = " << str;
	return (str);
}