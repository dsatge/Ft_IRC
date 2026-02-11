#include "channel.hpp"
#include "client.hpp"

Channel::Channel() : _channelName("default") {}

Channel::Channel(const std::string &name) : _channelName(name) {}

Channel::Channel(const Channel &other)
	: _channelName(other._channelName), _clients(other._clients) {}

Channel& Channel::operator=(const Channel &other)
{
	if (this != &other)
	{
		this->_channelName = other._channelName;
		this->_clients = other._clients;
	}
	return (*this);
}

Channel::~Channel() {}

void Channel::SetChannelName(const std::string &name)
{
	this->_channelName = name;
}

void Channel::AddClient(const std::string &nickname, Client *client)
{
	this->_clients[nickname] = client;
}

void Channel::RemoveClient(const std::string &nickname)
{
	this->_clients.erase(nickname);
}

std::string Channel::GetChannelName() const
{
	return (this->_channelName);
}

Client* Channel::GetClient(const std::string &nickname) const
{
	std::map<std::string, Client*>::const_iterator it = this->_clients.find(nickname);
	if (it != this->_clients.end())
		return (it->second);
	return (NULL);
}

const std::map<std::string, Client*>& Channel::GetAllClients() const
{
	return (this->_clients);
}

bool Channel::ClientExists(const std::string &nickname) const
{
	return (this->_clients.find(nickname) != this->_clients.end());
}

int Channel::GetClientCount() const
{
	return (this->_clients.size());
}
