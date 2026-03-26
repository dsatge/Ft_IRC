#include "channel.hpp"
#include "client.hpp"

Channel::Channel() : _moderator(0), _topic(""), _inviteOnly(false), _topicRestricted(false), _key(""), _userLimit(0) {}

Channel::Channel(const std::string &) : _moderator(0), _topic(""), _inviteOnly(false), _topicRestricted(false), _key(""), _userLimit(0) {}

Channel::Channel(const Channel &other)
	: _moderator(other._moderator), _topic(other._topic), _inviteOnly(other._inviteOnly), _topicRestricted(other._topicRestricted), _key(other._key), _userLimit(other._userLimit), _clients(other._clients) {}

Channel& Channel::operator=(const Channel &other)
{
	if (this != &other)
	{
		this->_moderator = other._moderator;
		this->_topic = other._topic;
		this->_inviteOnly = other._inviteOnly;
		this->_topicRestricted = other._topicRestricted;
		this->_key = other._key;
		this->_userLimit = other._userLimit;
		this->_clients = other._clients;
	}
	return (*this);
}

Channel::~Channel() {}

void Channel::AddClient(int fd, Client *client)
{
	this->_clients[fd] = client;
}

void Channel::RemoveClient(const int fd)
{
	this->_clients.erase(fd);
}

void Channel::SetModerator(const int fd)
{
	this->_moderator = fd;
}

int Channel::GetModerator() const
{
	return (this->_moderator);
}

void Channel::SetTopic(const std::string &topic)
{
	this->_topic = topic;
}

void Channel::SetInviteOnly(bool inviteOnly)
{
	this->_inviteOnly = inviteOnly;
}

void Channel::SetTopicRestricted(bool topicRestricted)
{
	this->_topicRestricted = topicRestricted;
}

void Channel::SetKey(const std::string &key)
{
	this->_key = key;
}

void Channel::ClearKey()
{
	this->_key.clear();
}

void Channel::SetUserLimit(int limit)
{
	this->_userLimit = limit;
}

void Channel::ClearUserLimit()
{
	this->_userLimit = 0;
}

std::string Channel::GetTopic() const
{
	return (this->_topic);
}

bool Channel::IsInviteOnly() const
{
	return (this->_inviteOnly);
}

bool Channel::IsTopicRestricted() const
{
	return (this->_topicRestricted);
}

std::string Channel::GetKey() const
{
	return (this->_key);
}

int Channel::GetUserLimit() const
{
	return (this->_userLimit);
}

Client* Channel::GetClient(const int fd)
{
	std::map<int, Client*>::iterator it = _clients.find(fd);
	if (it != this->_clients.end())
		return (it->second);
	return (NULL);
}

const std::map<int, Client*>& Channel::GetAllClients() const
{
	return (this->_clients);
}

bool Channel::ClientExists(const int fd) const
{
	return (this->_clients.find(fd) != this->_clients.end());
}

int Channel::GetClientCount() const
{
	return (this->_clients.size());
}
