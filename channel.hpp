#pragma once

#include "lib.hpp"

class Client;

class Channel
{
	private:
		int _moderator;
		std::string _topic;
		bool _inviteOnly;
		bool _topicRestricted;
		std::string _key;
		int _userLimit;
		std::map<int, Client*> _clients;

	public:
		Channel();
		Channel(const std::string &name);
		Channel(const Channel &other);
		Channel& operator=(const Channel &other);
		~Channel();

		void	AddClient(const int fd, Client *client);
		void	RemoveClient(const int fd);
		void	SetModerator(const int fd);
		void	SetTopic(const std::string &topic);
		void	SetInviteOnly(bool inviteOnly);
		void	SetTopicRestricted(bool topicRestricted);
		void	SetKey(const std::string &key);
		void	ClearKey();
		void	SetUserLimit(int limit);
		void	ClearUserLimit();

		int	GetModerator() const;
		std::string	GetTopic() const;
		bool	IsInviteOnly() const;
		bool	IsTopicRestricted() const;
		std::string	GetKey() const;
		int		GetUserLimit() const;
		Client*	GetClient(const int fd);
		const std::map<int, Client*>& GetAllClients() const;
		bool	ClientExists(const int fd) const;
		int		GetClientCount() const;
};