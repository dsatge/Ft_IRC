#include "channel.hpp"
#include "client.hpp"

Channel::Channel() : _moderator(""), _topic(""), _inviteOnly(false), _topicRestricted(false), _key(""), _userLimit(0) {}

Channel::Channel(const std::string &) : _moderator(""), _topic(""), _inviteOnly(false), _topicRestricted(false), _key(""), _userLimit(0) {}

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

void Channel::AddClient(const std::string &nickname, Client *client)
{
	this->_clients[nickname] = client;
}

void Channel::RemoveClient(const std::string &nickname)
{
	this->_clients.erase(nickname);
}

void Channel::SetModerator(const std::string &nickname)
{
	this->_moderator = nickname;
}

std::string Channel::GetModerator() const
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
