#pragma once

#include <string>
#include <map>

class Client;

class Channel
{
	private:
		std::string _channelName;
		std::string _moderator;
		std::string _topic;
		bool _inviteOnly;
		bool _topicRestricted;
		std::string _key;
		int _userLimit;
		std::map<std::string, Client*> _clients;

	public:
		Channel();
		Channel(const std::string &name);
		Channel(const Channel &other);
		Channel& operator=(const Channel &other);
		~Channel();

		void	SetChannelName(const std::string &name);
		void	AddClient(const std::string &nickname, Client *client);
		void	RemoveClient(const std::string &nickname);
		void	SetModerator(const std::string &nickname);
		void	SetTopic(const std::string &topic);
		void	SetInviteOnly(bool inviteOnly);
		void	SetTopicRestricted(bool topicRestricted);
		void	SetKey(const std::string &key);
		void	ClearKey();
		void	SetUserLimit(int limit);
		void	ClearUserLimit();

		std::string	GetChannelName() const;
		std::string	GetModerator() const;
		std::string	GetTopic() const;
		bool	IsInviteOnly() const;
		bool	IsTopicRestricted() const;
		std::string	GetKey() const;
		int		GetUserLimit() const;
		Client*	GetClient(const std::string &nickname) const;
		const std::map<std::string, Client*>& GetAllClients() const;
		bool	ClientExists(const std::string &nickname) const;
		int		GetClientCount() const;
};