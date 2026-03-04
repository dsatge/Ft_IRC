# include "server.hpp"

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

int Server::modeHandler(char mode, int index, bool adding, std::string user, std::string channelName, std::string param)
{
	std::map<char, cmdPtrMode>::iterator it = _Modes.find(mode);
	int exitStatus = 0;
	if (it != _Modes.end())
	{
		cmdPtrMode func = it->second;
		exitStatus = (this->*func)(index, adding, user, channelName, param);
	}
	else
	{
		std::string unknownchar(1, mode);
		return (sendErroMsgKEY(ERR_UNKNOWNMODE, index, user, unknownchar), EXIT_FAILURE);
	}
	return (exitStatus);
}

int	Server::modeI(int index, bool adding, std::string user, std::string channelName, std::string empty)
{
	(void)empty;
	this->_channels[channelName].SetInviteOnly(adding);
	std::string reply = ":" + std::string(SERVER_NAME) + " 324 " + user + " #" + channelName + " " + 'i' + "\r\n";
	send(this->_Fds[index].fd, reply.c_str(), reply.size(), 0);
	return (EXIT_SUCCESS);
}

int	Server::modeT(int index, bool adding, std::string user, std::string channelName, std::string empty)
{
	(void)empty;
	this->_channels[channelName].SetTopicRestricted(adding);
	std::string reply = ":" + std::string(SERVER_NAME) + " 324 " + user + " #" + channelName + " " + 't' + "\r\n";
	send(this->_Fds[index].fd, reply.c_str(), reply.size(), 0);
	return (EXIT_SUCCESS);
}

int	Server::modeK(int index, bool adding, std::string user, std::string channelName, std::string password)
{
	std::cerr << BLUE << "I ENTERED HERE" << RESET << std::endl;
	if (password.empty())
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, user, "MODE"), EXIT_FAILURE);
	else if (adding)
		this->_channels[channelName].SetKey(password);
	else
		this->_channels[channelName].ClearKey();
	std::string reply = ":" + std::string(SERVER_NAME) + " 324 " + user + " #" + channelName + " " + 'k' + "\r\n";
	send(this->_Fds[index].fd, reply.c_str(), reply.size(), 0);
	return (EXIT_SUCCESS);
}

int	Server::modeO(int index, bool adding, std::string user, std::string channelName, std::string target)
{
	if (target.empty())
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, user, "MODE"), EXIT_FAILURE);
	if (!this->_channels[channelName].ClientExists(target))
	{
		sendErroMsgCHANNEL_KEY(ERR_USERNOTINCHANNEL, index, user, channelName, target);
		std::string err = ":" + std::string(SERVER_NAME) + " 441 " + user + " " + target + " #" + channelName + " :They are not on that channel\r\n";
		send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
		return (EXIT_FAILURE);
	}
	if (adding)
		this->_channels[channelName].SetModerator(target);
	else if (this->_channels[channelName].GetModerator() == target)
		this->_channels[channelName].SetModerator("");
	std::string reply = ":" + std::string(SERVER_NAME) + " 324 " + user + " #" + channelName + " " + 'k' + "\r\n";
	send(this->_Fds[index].fd, reply.c_str(), reply.size(), 0);
	return (EXIT_SUCCESS);
}

int	Server::modeL(int index, bool adding, std::string user, std::string channelName, std::string limit)
{
	if (limit.empty())
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, user, "MODE"), EXIT_FAILURE);
	int limitNUM = std::atoi(limit.c_str());
	if (limitNUM <= 0)
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, user, "MODE"), EXIT_FAILURE);
	if (adding)
		this->_channels[channelName].SetUserLimit(limitNUM);
	else
		this->_channels[channelName].ClearUserLimit();
	std::string reply = ":" + std::string(SERVER_NAME) + " 324 " + user + " #" + channelName + " " + 'k' + "\r\n";
	send(this->_Fds[index].fd, reply.c_str(), reply.size(), 0);
	return (EXIT_SUCCESS);
}