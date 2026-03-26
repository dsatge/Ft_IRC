# include "server.hpp"
# include "client.hpp"

void	Server::initMode()
{
	_Modes['i'] = &Server::modeI;
	_Modes['t'] = &Server::modeT;
	_Modes['k'] = &Server::modeK;
	_Modes['o'] = &Server::modeO;
	_Modes['l'] = &Server::modeL;
}

bool Server::modeNeedsParam(char mode)
{
	if (mode == 'k' || mode == 'o' || mode == 'l')
		return (true);
	return (false);
}

int Server::modeHandler(char mode, int index, bool adding, std::string user, std::string channelName, std::string param, Client &client)
{
	std::map<char, cmdPtrMode>::iterator it = _Modes.find(mode);
	int exitStatus = 0;
	if (it != _Modes.end())
	{
		cmdPtrMode func = it->second;
		exitStatus = (this->*func)(index, adding, user, channelName, param, client);
	}
	else
	{
		std::string unknownchar(1, mode);
		sendErroMsgKEY(ERR_UNKNOWNMODE, index, user, unknownchar, client);
		return (client.SetErase(), EXIT_FAILURE);
	}
	return (exitStatus);
}

int	Server::modeI(int index, bool adding, std::string user, std::string channelName, std::string empty, Client &client)
{
	(void)empty;
	this->_channels[channelName].SetInviteOnly(adding);
	std::string reply = ":" + std::string(SERVER_NAME) + " 324 " + user + " #" + channelName + " " + 'i' + "\r\n";
	if (send(this->_Fds[index].fd, reply.c_str(), reply.size(), MSG_NOSIGNAL) == -1)
		return (client.SetErase(), EXIT_FAILURE);
	return (EXIT_SUCCESS);
}

int	Server::modeT(int index, bool adding, std::string user, std::string channelName, std::string empty, Client &client)
{
	(void)empty;
	this->_channels[channelName].SetTopicRestricted(adding);
	std::string reply = ":" + std::string(SERVER_NAME) + " 324 " + user + " #" + channelName + " " + 't' + "\r\n";
	if (send(this->_Fds[index].fd, reply.c_str(), reply.size(), MSG_NOSIGNAL) == -1)
		return (client.SetErase(), EXIT_FAILURE);
	return (EXIT_SUCCESS);
}

int	Server::modeK(int index, bool adding, std::string user, std::string channelName, std::string password, Client &client)
{
	if (password.empty() && adding == true)
	{
		sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, user, "MODE", client);
		return (EXIT_FAILURE);
	}
	else if (adding)
		this->_channels[channelName].SetKey(password);
	else
		this->_channels[channelName].ClearKey();
	std::string reply = ":" + std::string(SERVER_NAME) + " 324 " + user + " #" + channelName + " " + 'k' + "\r\n";
	if (send(this->_Fds[index].fd, reply.c_str(), reply.size(), MSG_NOSIGNAL) == -1)
		return (client.SetErase(), EXIT_FAILURE);
	return (EXIT_SUCCESS);
}

int	Server::modeO(int index, bool adding, std::string user, std::string channelName, std::string target, Client &client)
{
	int	targetFd = GetNickFd(target);
	if (target.empty() && adding == true)
	{
		sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, user, "MODE", client);
		return (EXIT_FAILURE);
	}
	if (adding == true && targetFd == -1 && !this->_channels[channelName].ClientExists(targetFd))
	{
		sendErroMsgCHANNEL_KEY(ERR_USERNOTINCHANNEL, index, user, channelName, target, client);
		return (EXIT_FAILURE);
	}
	if (adding)
		this->_channels[channelName].SetModerator(targetFd);
	else if (this->_channels[channelName].GetModerator() == targetFd)
		this->_channels[channelName].SetModerator(0);
	std::string reply = ":" + std::string(SERVER_NAME) + " 324 " + user + " #" + channelName + " " + 'o' + "\r\n";
	if (send(this->_Fds[index].fd, reply.c_str(), reply.size(), MSG_NOSIGNAL) == -1)
		return (client.SetErase(), EXIT_FAILURE);
	return (EXIT_SUCCESS);
}

int	Server::modeL(int index, bool adding, std::string user, std::string channelName, std::string limit, Client &client)
{
	if (limit.empty() && adding == true)
	{
		sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, user, "MODE", client);
		return (EXIT_FAILURE);
	}
	if (adding)
	{
		int limitNUM = std::atoi(limit.c_str());
		if (limitNUM <= 0)
		{
			sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, user, "MODE", client);
			return (EXIT_FAILURE);
		}
		this->_channels[channelName].SetUserLimit(limitNUM);
	}
	else
		this->_channels[channelName].ClearUserLimit();
	std::string reply = ":" + std::string(SERVER_NAME) + " 324 " + user + " #" + channelName + " " + 'l' + "\r\n";
	if (send(this->_Fds[index].fd, reply.c_str(), reply.size(), MSG_NOSIGNAL) == -1)
		return (client.SetErase(), EXIT_FAILURE);
	return (EXIT_SUCCESS);
}