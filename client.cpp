# include "client.hpp"

Client::Client(int fd) : _fd(fd)
{
	this->_toErase = false;
	this->_authenticated = false;
	this->_nickname = "";
	this->_username = "";
	this->_realname = "";
	this->_channelName = "";
	return ;
}

Client::Client(const Client &other)
{
	this->_fd = other._fd;
	this->_IP = other._IP;
	this->_toErase = other._toErase;
	this->_authenticated = other._authenticated;
	this->_nickname = other._nickname;
	this->_username = other._username;
	this->_realname = other._realname;
	this->_Msg = other._Msg;
	return ;
}

Client& Client::operator=(const Client &other)
{
	if (this != &other)
	{
		this->_fd = other._fd;
		this->_IP = other._IP;
		this->_Msg = other._Msg;
		this->_authenticated = other._authenticated;
		this->_nickname = other._nickname;
		this->_username = other._username;
		this->_realname = other._realname;
	}
	return (*this);
}

Client& Client::operator+=(const Client &other)
{
	if (this != &other)
	{
		this->_fd = other._fd;
		this->_IP = other._IP;
		this->_Msg += other._Msg;
		this->_authenticated = other._authenticated;
		this->_nickname = other._nickname;
		this->_username = other._username;
		this->_realname = other._realname;
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

void Client::SetMsg(char *buffer, size_t bytesSize)
{
	this->_Msg.append(buffer, bytesSize);
	return ;
}

void Client::SetEraseMsg(int posInit, int posEnd)
{
	this->_Msg.erase(posInit, posEnd);
}


void Client::SetErase()
{
	this->_toErase = true;
	return ;
}

void Client::SetAuthenticated(bool value)
{
	this->_authenticated = value;
}

void Client::SetNickname(const std::string &nickname)
{
	this->_nickname = nickname;
}

void Client::SetUsername(const std::string &username)
{
	this->_username = username;
}

void Client::SetRealname(const std::string &realname)
{
	this->_realname = realname;
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

std::string Client::GetMsg() const
{
	return (this->_Msg);
}

bool Client::GetErase() const
{
	if (this->_toErase == true)
		return (true);
	return (false);
}

bool Client::GetAuthenticated() const
{
	return (this->_authenticated);
}

std::string Client::GetNickname() const
{
	return (this->_nickname);
}

std::string Client::GetUsername() const
{
	return (this->_username);
}

std::string Client::GetRealname() const
{
	return (this->_realname);
}

void Client::SetChannelName(const std::string &channel)
{
	this->_channelName = channel;
}

std::string Client::GetChannelName() const
{
	return (this->_channelName);
}

int Client::GetFd() const
{
	return (this->_fd);	
}

std::string Client::GetIP() const
{
	return (this->_IP);
}