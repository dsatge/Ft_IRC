# include "server.hpp"
# include "client.hpp"

#define MAX_IRC_MESSAGE 512

void	Server::initCmds()
{
	_cmds["HELP"] = &Server::cmdHelp;
	_cmds["PING"] = &Server::cmdPing;
	_cmds["PONG"] = &Server::cmdPong;
	_cmds["JOIN"] = &Server::cmdJoin;
	_cmds["NAMES"] = &Server::cmdNames;
	_cmds["LIST"] = &Server::cmdList;
	_cmds["MODE"] = &Server::cmdMode;
	_cmds["TOPIC"] = &Server::cmdTopic;
	_cmds["PRIVMSG"] = &Server::cmdPrivmsg;
	_cmds["PART"] = &Server::cmdPart;
	_cmds["QUIT"] = &Server::cmdQuit;
	_cmds["KICK"] = &Server::cmdKick;
	_cmds["INVITE"] = &Server::cmdInvite;
}

static std::string enforceMessageLimit(const std::string& msg)
{
	if (msg.length() > MAX_IRC_MESSAGE)
	{
		return (msg.substr(0, MAX_IRC_MESSAGE - 2) + "\r\n");
	}
	return (msg);
}

int Server::cmdHandler(std::vector<std::string> argList, int index, Client &client)
{
	if (argList.empty())
		return (0);
	std::vector<std::string>::iterator itcmd = argList.begin();
	std::string cmd = *itcmd;
	std::map<std::string, cmdPtr>::iterator it = _cmds.find(cmd);
	int	quitReturn = 0;

	if (it != _cmds.end())
	{
		cmdPtr func = it->second;
		quitReturn += (this->*func)(argList, index, client);
	}
	else
		quitReturn += sendErroMsgKEY(ERR_UNKNOWNCOMMAND, index, client.GetNickname(), cmd, client);
	return (quitReturn);
}

int Server::cmdHelp(std::vector<std::string> argList, int index, Client &client)
{
	(void) argList;
	(void) client;
	std::string help = "\n";
	help += "========== AVAILABLE COMMANDS (RFC 2812) ==========\n\n";
	help += "--- CONNECTION COMMANDS ---\n";
	help += "NICK <nickname>                    - Set or change your nickname\n";
	help += "USER <user> <mode> <unused> :<realname> - Set username and realname\n";
	help += "QUIT [:<message>]                    - Disconnect from server (message in option)\n\n";
	help += "--- CHANNEL OPERATIONS ---\n";
	help += "JOIN <channel> [key]               - Join a channel (or create if doesn't exist)\n";
	help += "PART <channel> [:message]          - Leave a channel\n";
	help += "NAMES <channel>                    - List users in a channel\n";
	help += "LIST                               - List all channels\n";
	help += "TOPIC <channel> [:<text>]          - View/set channel topic (: required to set)\n\n";
	help += "--- CHANNEL MANAGEMENT (Moderator Only) ---\n";
	help += "MODE <channel> <modes> [params]    - Set channel modes\n";
	help += "  +i / -i                          - Invite-only mode\n";
	help += "  +t / -t                          - Topic restricted (only moderator can set)\n";
	help += "  +k <key> / -k                    - Set/remove channel password\n";
	help += "  +l <limit> / -l                  - Set/remove user limit\n";
	help += "  +o <user> / -o <user>            - Grant/remove moderator privileges\n";
	help += "  Example: MODE #channel +tk mykey  - Set protected +t and +k modes\n";
	help += "KICK <channel> <user>              - Remove user from channel (moderator only)\n";
	help += "INVITE <user> <channel>            - Invite user to channel (moderator only)\n\n";
	help += "--- MESSAGING ---\n";
	help += "PRIVMSG <target> :<message>        - Send message to user or channel (: required)\n";
	help += "  <target> can be a nick or #channel\n\n";
	help += "--- GENERAL ---\n";
	help += "HELP                               - Show this help message\n";
	help += "====================================================\n";
	if (send(this->_Fds[index].fd, help.c_str(), help.size(), MSG_NOSIGNAL) == -1)
		return (client.SetErase(), EXIT_FAILURE);
	return (EXIT_SUCCESS);
}

int Server::cmdPing(std::vector<std::string> argList, int index, Client &client)
{
	std::string	arg;
	std::string token;

	if (argList.size() < 2)
		return (sendErroMsg(ERR_NOORIGIN, index, client.GetNickname(), client));
	
	std::vector<std::string>::iterator itparam = argList.begin();	
	itparam++;
	std::string param = *itparam++;
	if (param[0] == ':')
		token = removesColon(param);
	else
		token = param;
	std::string pong = ":" + std::string(SERVER_NAME) + " PONG " + SERVER_NAME + " :" + token + "\r\n";
	if (send(this->_Fds[index].fd, pong.c_str(), pong.size(), MSG_NOSIGNAL) == -1)
		return (client.SetErase(), EXIT_FAILURE);
	return (EXIT_SUCCESS);
}

int Server::cmdPong(std::vector<std::string> argList, int index, Client &client)
{
	(void)argList;
	(void)index;
	(void)client;
	return (0);
}

int	Server::cmdJoin(std::vector<std::string> argList, int index, Client &client)
{
	std::string nick = client.GetNickname();
	int	nickFd = client.GetFd();
	if (argList.size() < 2)
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "JOIN", client));
	std::vector<std::string>::iterator itparam = argList.begin();
	itparam++;
	
	std::string channelName, key;
	if (itparam == argList.end())
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "JOIN", client));
	channelName = *itparam;
	itparam++;
	if (itparam == argList.end())
		key = "";
	else
		key = *itparam;	
	if (!channelName.empty() && channelName[0] == '#')
		channelName.erase(0, 1);
	channelName = cleanChannelName(channelName);
	if (this->_channels.find(channelName) != this->_channels.end())
	{
		if (this->_channels[channelName].ClientExists(nickFd))
			return (sendErroMsgCHANNEL_KEY(ERR_USERONCHANNEL, index, nick, channelName, nick, client));
		if (this->_channels[channelName].GetUserLimit() > 0
				&& this->_channels[channelName].GetClientCount() >= this->_channels[channelName].GetUserLimit())
			return (sendErroMsgCHANNEL(ERR_CHANNELISFULL, index, nick, channelName, client));
		if (!this->_channels[channelName].GetKey().empty() && key != this->_channels[channelName].GetKey())
			return (sendErroMsgCHANNEL(ERR_BADCHANNELKEY, index, nick, channelName, client));
		if (this->_channels[channelName].IsInviteOnly()
				&& this->_channels[channelName].GetModerator() != nickFd
				&& !this->_channels[channelName].ClientExists(nickFd))
			return (sendErroMsgCHANNEL(ERR_INVITEONLYCHAN, index, nick, channelName, client));
	}

	std::string oldChannel = client.GetChannelName();
	if (!oldChannel.empty() && this->_channels.find(oldChannel) != this->_channels.end())
	{
		this->_channels[oldChannel].RemoveClient(nickFd);
		if (this->_channels[oldChannel].GetClientCount() == 0)
			this->_channels.erase(oldChannel);
	}

	client.SetChannelName(channelName);
	if (this->_channels.find(channelName) == this->_channels.end())
	{
		Channel newChannel(channelName);
		this->_channels[channelName] = newChannel;
		this->_channels[channelName].SetModerator(nickFd);
		std::cerr << MAGENTA << nick << " is now moderator of #" << channelName << RESET << std::endl;
	}

	else if (this->_channels[channelName].GetClientCount() == 0)
	{
		this->_channels[channelName].SetModerator(nickFd);
		std::cerr << MAGENTA << nick << " is now moderator of #" << channelName << RESET << std::endl;
	}

	this->_channels[channelName].AddClient(nickFd, &client);
	std::string joinMsg = ":" + nick + "!" + client.GetUsername() + "@localhost JOIN #" + channelName + "\r\n";
	if (send(this->_Fds[index].fd, joinMsg.c_str(), joinMsg.size(), MSG_NOSIGNAL) == -1)
		return (client.SetErase(), EXIT_FAILURE);
	std::cerr << GREEN << nick << " joined channel " << channelName << RESET << std::endl;
	return (EXIT_SUCCESS);
}

int Server::cmdNames(std::vector<std::string> argList, int index, Client &client)
{
	std::vector<std::string>::iterator itparam = argList.begin();
	std::string nick = client.GetNickname();

	itparam++;
	std::string channelName = "";
	if (itparam != argList.end())
		channelName = *itparam;
	if (!channelName.empty() && channelName[0] == '#')
		channelName.erase(0, 1);
	if (this->_channels.find(channelName) == this->_channels.end())
		return (sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, channelName, client));
	else
	{
		std::string usersLine = "";
		const std::map<int, Client*>& channelClients = this->_channels[channelName].GetAllClients();
		for (std::map<int, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
		{
			if (!usersLine.empty())
				usersLine += " ";
			usersLine += it->second->GetNickname();
		}
		std::string response = ":" + std::string(SERVER_NAME) + " 353 " + nick + " = #" + channelName + " :" + usersLine + "\r\n";
		response += ":" + std::string(SERVER_NAME) + " 366 " + nick + " #" + channelName + " :End of /NAMES list\r\n";
		response = enforceMessageLimit(response);
		if (send(this->_Fds[index].fd, response.c_str(), response.size(), MSG_NOSIGNAL) == -1)
			return (client.SetErase(), EXIT_FAILURE);
	}
	return (0);
}

int	Server::cmdList(std::vector<std::string> argList, int index, Client &client)
{
	(void) argList;
	std::string nick = client.GetNickname();
	std::string start = ":" + std::string(SERVER_NAME) + " 321 " + nick + " Channel :Users Name\r\n";
	if (send(this->_Fds[index].fd, start.c_str(), start.size(), MSG_NOSIGNAL) == -1)
		return (client.SetErase(), EXIT_FAILURE);
	for (std::map<std::string, Channel>::iterator it = this->_channels.begin(); it != this->_channels.end(); ++it)
	{
		std::string line = ":" + std::string(SERVER_NAME) + " 322 " + nick + " #" + it->first + " " + intToString(it->second.GetClientCount()) + " :\r\n";
		if (send(this->_Fds[index].fd, line.c_str(), line.size(), MSG_NOSIGNAL) == -1)
			return (client.SetErase(), EXIT_FAILURE);
	}
	std::string end = ":" + std::string(SERVER_NAME) + " 323 " + nick + " :End of /LIST\r\n";
	if (send(this->_Fds[index].fd, end.c_str(), end.size(), MSG_NOSIGNAL) == -1)
		return (client.SetErase(), EXIT_FAILURE);
	return (EXIT_SUCCESS);
}

int Server::cmdMode(std::vector<std::string> argList, int index, Client &client)
{
	std::string nick = client.GetNickname();
	int	nickFd = client.GetFd();
	std::string	arg;

	if (argList.size() < 3)
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "MODE", client));
	
	std::vector<std::string>::iterator itparameter = argList.begin();

	/// Skip "MODE" from list of command -> 'channel' argument
	itparameter++;
	std::string channelName = *itparameter;
	if (channelName[0] == '#')
		channelName.erase(0,1);
	if (this->_channels.find(channelName) == this->_channels.end())
		return (sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, channelName, client));
	if (this->_channels[channelName].GetModerator() != nickFd)
		return (sendErroMsgCHANNEL(ERR_CHANOPRIVSNEEDED, index, nick, channelName, client));
	
	/// Skip "channel" from list of command -> 'mode' argument
	itparameter++;
	std::string mode = *itparameter;
	
	bool adding = true;
	initMode();
	int i = 0;
	while (itparameter != argList.end())
	{
		if (!(mode[i] == '+' || mode[i] == '-'))
			return (EXIT_SUCCESS);
		if (mode[i] == '+')
			adding = true;
		else if (mode[i] == '-')
			adding = false;
		i++;
		std::string param = "";
		if (modeNeedsParam(mode[i]) == true && adding == true)
		{
			itparameter++;
			if (itparameter == argList.end())
				return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "MODE", client));
			param = *itparameter;
		}
		if (modeHandler(mode[i], index, adding, nick, channelName, param, client) == EXIT_FAILURE)
			return (EXIT_FAILURE);
		i++;
		if (!mode[i])
		{
			itparameter++;
			if (itparameter != argList.end())
				mode = *itparameter;
			i = 0;
		}
	}
	return (EXIT_SUCCESS);
}


int	Server::cmdTopic(std::vector<std::string> argList, int index, Client &client)
{
	std::string nick = client.GetNickname();
	int	nickFd = client.GetFd();
	if (argList.size() < 2)
		return (sendErroMsg(ERR_NEEDMOREPARAMS, index, nick, client));
	else
	{
		std::vector<std::string>::iterator itparameter = argList.begin();
		itparameter++;
		if (itparameter == argList.end())
			return (sendErroMsg(ERR_NEEDMOREPARAMS, index, nick, client));
		
		std::string channelName = *itparameter;
		if (!channelName.empty() && channelName[0] == '#')
			channelName.erase(0, 1);
		
		itparameter++;
		if (this->_channels.find(channelName) == this->_channels.end())
			return (sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, channelName, client));
		else if (itparameter == argList.end())
		{
			std::string topic = this->_channels[channelName].GetTopic();
			if (topic.empty())
				return (sendInfoMsgCHANNEL(RPL_NOTOPIC, index, nick, channelName, client));
			else
			{
				std::string response = ":" + std::string(SERVER_NAME) + " 332 " + nick + " #" + channelName + " :" + topic + "\r\n";
				response = enforceMessageLimit(response);
				if (send(this->_Fds[index].fd, response.c_str(), response.size(), MSG_NOSIGNAL) == -1)
					return (client.SetErase(), EXIT_FAILURE);
			}
		}
		else
		{
			if (this->_channels[channelName].IsTopicRestricted()
				&& this->_channels[channelName].GetModerator() != nickFd)
			{
				if (sendErroMsgCHANNEL(ERR_CHANOPRIVSNEEDED, index, nick, channelName, client) == EXIT_FAILURE)
					return (EXIT_FAILURE);
			}
			else
			{
				std::string newTopic = *itparameter;
				if (newTopic.empty() || newTopic[0] != ':')
					return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "TOPIC", client));
				newTopic.erase(0, 1);

				this->_channels[channelName].SetTopic(newTopic);
				
				std::string topicMsg = ":" + nick + "!" + nick + "@localhost TOPIC #"
						+ channelName + " :" + newTopic + "\r\n";
				topicMsg = enforceMessageLimit(topicMsg);
				std::cerr << MAGENTA << nick << " changed topic of #" << channelName << " to: " << newTopic << RESET << std::endl;
				
				const std::map<int, Client*>& channelClients = this->_channels[channelName].GetAllClients();
				for (std::map<int, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
				{
					if (!it->second->GetFd() || it->second->GetFd() == 0)
						return (EXIT_FAILURE);
					if (send(it->second->GetFd(), topicMsg.c_str(), topicMsg.size(), MSG_NOSIGNAL) == -1)
						return (client.SetErase(), EXIT_FAILURE);
				}
			}
		}
	}
	return (EXIT_SUCCESS);
}

int	Server::cmdPrivmsg(std::vector<std::string> argList, int index, Client &client)
{
	std::string senderNick = client.GetNickname();
	int	senderNickFd = client.GetFd();
	std::vector<std::string>::iterator itparameter = argList.begin();
	itparameter++;
	if (itparameter == argList.end())
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, senderNick, "PRIVMSG", client));
	else
	{
		std::string target = *itparameter;
		itparameter++;
		if (itparameter == argList.end())
			return(sendErroMsg(ERR_NOTEXTTOSEND, index, senderNick, client));
		std::string message = *itparameter;
		if (message[0] != ':')
			return(sendErroMsg(ERR_NOTEXTTOSEND, index, senderNick, client));
		message.erase(0, 1);
			
		bool isChannel = (target[0] == '#');
		if (isChannel)
		{
			if (target.length() > 1)
				target.erase(0, 1);

			if (this->_channels.find(target) == this->_channels.end())
				return (sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, senderNick, target, client));
			else if (!this->_channels[target].ClientExists(senderNickFd))
				return (sendErroMsgCHANNEL(ERR_CANNOTSENDTOCHAN, index, senderNick, target, client));
			else
			{
				// std::string privmsgLine = std::string(CYAN) + ":" + senderNick + "!" + client.GetUsername() + "@localhost PRIVMSG #"
				// 		+ target + " :" + message + std::string(RESET) + "\r\n";
				std::string privmsgLine = ":" + senderNick + "!" + client.GetUsername() + "@localhost PRIVMSG #"
						+ target + " :" + message + "\r\n";
				privmsgLine = enforceMessageLimit(privmsgLine);
				const std::map<int, Client*>& channelClients = this->_channels[target].GetAllClients();
				for (std::map<int, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
				{
					if (it->second->GetErase() == true)
						return (EXIT_FAILURE);
					if (it->second->GetErase() == false && it->second->GetFd() != this->_Fds[index].fd)
						if (send(it->second->GetFd(), privmsgLine.c_str(), privmsgLine.size(), MSG_NOSIGNAL) == -1)
							return (client.SetErase(), EXIT_FAILURE);
				}
				std::cout << CYAN << "[#" << target << "] " << senderNick << ": " << message << RESET << std::endl;
			}
		}
		else
		{
			Client* targetClient = NULL;
			for (std::map<int, Client>::iterator it = this->_Client.begin(); it != this->_Client.end(); ++it)
			{
				if (it->second.GetNickname() == target)
				{
					targetClient = &it->second;
					break;
				}
			}

			if (!targetClient)
				return (sendErroMsgKEY(ERR_NOSUCHNICK, index, senderNick, target, client));
			else
			{
				// std::string privmsgLine = std::string(BLUE) + ":" + senderNick + "!" + client.GetUsername() + "@localhost PRIVMSG "
				// 		+ target + " :" + message + std::string(RESET) + "\r\n";
				std::string privmsgLine = ":" + senderNick + "!" + senderNick + "@localhost PRIVMSG " 
						+ targetClient->GetNickname() + " :" + message + "\r\n";
				privmsgLine = enforceMessageLimit(privmsgLine);
				if (send(targetClient->GetFd(), privmsgLine.c_str(), privmsgLine.size(), MSG_NOSIGNAL) == -1)
					return (client.SetErase(), EXIT_FAILURE);
				std::cout << BLUE << "[PM] " << senderNick << " -> " << target << ": " << message << RESET << std::endl;
			}
		}
	}
	return (EXIT_SUCCESS);
}

int	Server::cmdPart(std::vector<std::string> argList, int index, Client &client)
{
	std::string nick = client.GetNickname();
	int	nickFd = client.GetFd();
	std::vector<std::string>::iterator itparameter = argList.begin();
	itparameter++;
	if (itparameter != argList.end())
	{
		std::string channelToQuit = *itparameter;
		if (!channelToQuit.empty() && channelToQuit[0] == '#')
			channelToQuit.erase(0, 1);
		if (this->_channels.find(channelToQuit) == this->_channels.end())
			return (sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, channelToQuit, client));
		else if (!this->_channels[channelToQuit].ClientExists(nickFd))
			return (sendErroMsgCHANNEL(ERR_NOTONCHANNEL, index, nick, channelToQuit, client));
		else
		{
			itparameter++;
			std::string quitMessage = "";
			if (itparameter != argList.end())
				quitMessage = *itparameter;
			std::string partLine = ":" + nick + "!" + nick + "@localhost PART #" + channelToQuit + " " + quitMessage + "\r\n";
			const std::map<int, Client*>& channelClients = this->_channels[channelToQuit].GetAllClients();
			for (std::map<int, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
			{
				if (send(it->second->GetFd(), partLine.c_str(), partLine.size(), MSG_NOSIGNAL) == -1)
					return (client.SetErase(), EXIT_FAILURE);
			}
			this->_channels[channelToQuit].RemoveClient(nickFd);
			if (this->_channels[channelToQuit].GetClientCount() == 0)
			{
				this->_channels.erase(channelToQuit);
				std::cerr << YELLOW << "Channel " << channelToQuit << " deleted (empty)" << RESET << std::endl;
			}
			client.SetChannelName("");
			std::cerr << YELLOW << nick << " left channel " << channelToQuit << RESET << std::endl;
		}
	}
	else
		return (sendErroMsg(ERR_NEEDMOREPARAMS, index, nick, client));
	return (EXIT_SUCCESS);
}

int Server::cmdQuit(std::vector<std::string> argList, int index, Client &client)
{
	std::string nick = client.GetNickname();
	std::string message;
	std::vector<std::string>::iterator itparameter = argList.begin();
	itparameter++;
	if (itparameter == argList.end())
		message = ":Client Quit";
	else
		message = *itparameter;
	if (message.empty() || message[0] != ':')
		return(sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "QUIT", client));
	message.erase(0, 1);
	std::string quitLine = ":" + nick + "!" + client.GetUsername() + "@localhost QUIT :" + message + "\r\n";
	for (std::map<int, Client>::iterator it = this->_Client.begin(); it != this->_Client.end(); ++it)
	{
		if (!it->second.GetNickname().empty())
		{
			if (send(it->second.GetFd(), quitLine.c_str(), quitLine.size(), MSG_NOSIGNAL) == -1)
				return (client.SetErase(), EXIT_FAILURE);
		}
	}
	client.SetErase();
	return (1);
}

int	Server::GetNickFd(std::string nick)
{
	std::map<int, Client>::iterator itClient = this->_Client.begin();
	if (itClient == this->_Client.end())
		return (-1);
	while (itClient != this->_Client.end())
	{
		if (itClient->second.GetNickname() == nick)
			return (itClient->first);
		itClient++;
	}
	return (-1);
}

int	Server::cmdKick(std::vector<std::string> argList, int index, Client &client)
{
	std::string nick = client.GetNickname();
	int	nickFd = client.GetFd();
	std::vector<std::string>::iterator itparameter = argList.begin();
	
	itparameter++;
	if (itparameter == argList.end())
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "KICK", client));
	std::string channelName = *itparameter;
	
	itparameter++;
	if (itparameter == argList.end())
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "KICK", client));
	std::string targetNick = *itparameter;
	int	targetNickFd = GetNickFd(targetNick);

	if (!channelName.empty() && channelName[0] == '#')
		channelName.erase(0, 1);
	if (this->_channels.find(channelName) == this->_channels.end())
		return (sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, channelName, client));
	else if (this->_channels[channelName].GetModerator() != nickFd)
		return (sendErroMsgCHANNEL(ERR_CHANOPRIVSNEEDED, index, nick, channelName, client));
	else if (targetNickFd == -1 || !this->_channels[channelName].ClientExists(targetNickFd))
		return (sendErroMsgCHANNEL_KEY(ERR_USERNOTINCHANNEL, index, nick, channelName, targetNick, client));
	else
	{
		Client* targetClient = this->_channels[channelName].GetClient(targetNickFd);
		if (targetClient->GetFd() != -1)
		{
			std::string kickLine = " :" + nick + "!" + client.GetUsername() + "@localhost KICK #"
					+ channelName + " " + targetNick + " : \r\n";
			const std::map<int, Client*>& channelClients = this->_channels[channelName].GetAllClients();
			for (std::map<int, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
			{
				if (send(it->second->GetFd(), kickLine.c_str(), kickLine.size(), MSG_NOSIGNAL) == -1)
					return (client.SetErase(), EXIT_FAILURE);
			}
			targetClient->SetChannelName("");
			this->_channels[channelName].RemoveClient(targetNickFd);
			std::cerr << MAGENTA << nick << " kicked " << targetNick << " from #" << channelName << RESET << std::endl;
		}
	}
	return (EXIT_SUCCESS);
}

int	Server::cmdInvite(std::vector<std::string> argList, int index, Client &client)
{
	std::string nick = client.GetNickname();
	int	nickFd = client.GetFd();
	std::vector<std::string>::iterator itparameter = argList.begin();
	itparameter++;
	if (itparameter == argList.end())
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "INVITE", client));
	std::string targetNick = *itparameter;
	int	targetNickFd = GetNickFd(targetNick);

	itparameter++;
	if (itparameter == argList.end())
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "INVITE", client));
	std::string targetChannel = *itparameter;

	if (!targetChannel.empty() && targetChannel[0] == '#')
		targetChannel.erase(0, 1);
	if (this->_channels.find(targetChannel) == this->_channels.end())
		return (sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, targetChannel, client));
	else if (this->_channels[targetChannel].GetModerator() != nickFd)
		return (sendErroMsgCHANNEL(ERR_CHANOPRIVSNEEDED, index, nick, targetChannel, client));
	else
	{
		if (targetNickFd == -1)
			return (sendErroMsgKEY(ERR_NOSUCHNICK, index, nick, targetNick, client));
		else if (this->_channels[targetChannel].ClientExists(targetNickFd))
			return (sendErroMsgCHANNEL_KEY(ERR_USERONCHANNEL, index, nick, targetChannel, targetNick, client));
		else
		{
			std::string inviteLine = ":" + nick + "!" + client.GetUsername() + "@localhost INVITE "
					+ targetNick + " #" + targetChannel + "\r\n";
			if (send(targetNickFd, inviteLine.c_str(), inviteLine.size(), MSG_NOSIGNAL) == -1)
				return (client.SetErase(), EXIT_FAILURE);

			std::string inviteConfirm = ":" + std::string(SERVER_NAME) + " 341 "
					+ targetNick + " #" + targetChannel + "\r\n";
			if (send(client.GetFd(), inviteConfirm.c_str(), inviteConfirm.size(), MSG_NOSIGNAL) == -1)
				return (client.SetErase(), EXIT_FAILURE);
			
			std::map<int, Client>::iterator itTargetClient = this->_Client.find(targetNickFd);
			if (itTargetClient == this->_Client.end())
				return (sendErroMsgKEY(ERR_NOSUCHNICK, index, nick, targetNick, client));			
			
			Client *targetClient = &itTargetClient->second;
			this->_channels[targetChannel].AddClient(targetNickFd, targetClient);
			targetClient->SetChannelName(targetChannel);
			std::cerr << PINK << nick << " invited " << targetNick << " to #" << targetChannel << RESET << std::endl;
		}
	}
	return (EXIT_SUCCESS);
}
