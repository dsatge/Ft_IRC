# include "client.hpp"

Client::Client(int fd) : _fd(fd)
{
	this->_toErase = false;
	return ;
}

Client::Client(const Client &other)
{
	this->_fd = other._fd;
	this->_IP = other._IP;
	this->_toErase = other._toErase;
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

void Client::SetErase()
{
	this->_toErase = true;
	return ;
}

std::string Client::GetBuffer() const
{
	std::stringstream ss;
	std::string str;
	if (!this->_Buffer || this->_Buffer == NULL)
		return (str);
	ss << this->_Buffer;
	return (ss.str());
}

bool Client::GetErase() const
{
	if (this->_toErase == true)
		return (true);
	return (false);
}

int Client::GetFd() const
{
	return (this->_fd);
}