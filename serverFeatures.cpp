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
		return msg.substr(0, MAX_IRC_MESSAGE - 2) + "\r\n";
	}
	return msg;
}

int Server::cmdHandler(std::string Msg, int index, Client &client)
{
	std::string cmd = getCmdFromMsg(Msg);
	std::map<std::string, cmdPtr>::iterator it = _cmds.find(cmd);
	int	quitReturn = 0;

	if (it != _cmds.end())
	{
		cmdPtr func = it->second;
		quitReturn = (this->*func)(Msg, index, client);
	}
	else
		sendErroMsgKEY(ERR_UNKNOWNCOMMAND, index, client.GetNickname(), cmd);
	return (quitReturn);
}

int Server::cmdHelp(std::string Msg, int index, Client &client)
{
	(void) Msg;
	(void) client;
	std::string help = "\n";
	help += "========== AVAILABLE COMMANDS (RFC 2812) ==========\n\n";
	help += "--- CONNECTION COMMANDS ---\n";
	help += "NICK <nickname>                    - Set or change your nickname\n";
	help += "USER <user> <mode> <unused> :<realname> - Set username and realname (: required)\n";
	help += "QUIT :<message>                    - Disconnect from server (: required)\n\n";
	help += "--- CHANNEL OPERATIONS ---\n";
	help += "JOIN <channel> [key]               - Join a channel (or create if doesn't exist)\n";
	help += "PART <channel>                     - Leave a channel\n";
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
	send(this->_Fds[index].fd, help.c_str(), help.size(), 0);
	return (0);
}

int Server::cmdPing(std::string Msg, int index, Client &client)
{
	std::string serverName = "ircserv";
	size_t spacePos = Msg.find(" ");

	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
	{
		sendErroMsg(ERR_NOORIGIN, index, client.GetNickname());
		return (0);
	}

	std::string token = Msg.substr(spacePos + 1);
	if (!token.empty() && token[0] == ':')
		token.erase(0, 1);

	std::string pong = ":" + serverName + " PONG " + serverName + " :" + token + "\r\n";
	send(this->_Fds[index].fd, pong.c_str(), pong.size(), 0);
	return (0);
}

int Server::cmdPong(std::string Msg, int index, Client &client)
{
	(void)Msg;
	(void)index;
	(void)client;
	return (0);
}

int	Server::cmdJoin(std::string Msg, int index, Client &client)
{
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
	{
		std::string rest = Msg.substr(spacePos + 1);
		size_t spacePos2 = rest.find(" ");
		std::string channelName = (spacePos2 != std::string::npos) ? rest.substr(0, spacePos2) : rest;
		std::string key = (spacePos2 != std::string::npos && spacePos2 + 1 < rest.length()) ? rest.substr(spacePos2 + 1) : "";
		if (!channelName.empty() && channelName[0] == '#')
			channelName.erase(0, 1);
		if (this->_channels.find(channelName) != this->_channels.end())
		{
			if (this->_channels[channelName].ClientExists(nick))
				return (sendErroMsgCHANNEL_KEY(ERR_USERONCHANNEL, index, nick, channelName, nick), 0);
			if (this->_channels[channelName].GetUserLimit() > 0
					&& this->_channels[channelName].GetClientCount() >= this->_channels[channelName].GetUserLimit())
				return (sendErroMsgCHANNEL(ERR_CHANNELISFULL, index, nick, channelName), 0);
			if (!this->_channels[channelName].GetKey().empty() && key != this->_channels[channelName].GetKey())
				return (sendErroMsgCHANNEL(ERR_BADCHANNELKEY, index, nick, channelName), 0);
			if (this->_channels[channelName].IsInviteOnly()
					&& this->_channels[channelName].GetModerator() != nick
					&& !this->_channels[channelName].ClientExists(nick))
				return (sendErroMsgCHANNEL(ERR_INVITEONLYCHAN, index, nick, channelName), 0);
		}
		std::string oldChannel = client.GetChannelName();
		if (!oldChannel.empty() && this->_channels.find(oldChannel) != this->_channels.end())
		{
			this->_channels[oldChannel].RemoveClient(nick);
			if (this->_channels[oldChannel].GetClientCount() == 0)
				this->_channels.erase(oldChannel);
		}
		client.SetChannelName(channelName);
		if (this->_channels.find(channelName) == this->_channels.end())
		{
			Channel newChannel(channelName);
			this->_channels[channelName] = newChannel;
			this->_channels[channelName].SetModerator(nick);
			std::cerr << MAGENTA << nick << " is now moderator of #" << channelName << RESET << std::endl;
		}
		else if (this->_channels[channelName].GetClientCount() == 0)
		{
			this->_channels[channelName].SetModerator(nick);
			std::cerr << MAGENTA << nick << " is now moderator of #" << channelName << RESET << std::endl;
		}
		this->_channels[channelName].AddClient(nick, &client);
		std::string joinMsg = ":" + nick + "!" + client.GetUsername() + "@localhost JOIN #" + channelName + "\r\n";
		send(this->_Fds[index].fd, joinMsg.c_str(), joinMsg.size(), 0);
		std::cerr << GREEN << nick << " joined channel " << channelName << RESET << std::endl;
	}
	else
		sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "JOIN");
	return (0);
}

int Server::cmdNames(std::string Msg, int index, Client &client)
{
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
	{
		std::string channelName = Msg.substr(spacePos + 1);
		if (!channelName.empty() && channelName[0] == '#')
			channelName.erase(0, 1);
		
		if (this->_channels.find(channelName) == this->_channels.end())
			sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, channelName);
		else
		{
			std::string usersLine = "";
			const std::map<std::string, Client*>& channelClients = this->_channels[channelName].GetAllClients();
			for (std::map<std::string, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
			{
				if (!usersLine.empty())
					usersLine += " ";
					usersLine += it->first;
			}
			std::string response = ":" + std::string(SERVER_NAME) + " 353 " + nick + " = #" + channelName + " :" + usersLine + "\r\n";
			response = enforceMessageLimit(response);
			send(this->_Fds[index].fd, response.c_str(), response.size(), 0);
			std::string end = ":" + std::string(SERVER_NAME) + " 366 " + nick + " #" + channelName + " :End of /NAMES list.\r\n";
			send(this->_Fds[index].fd, end.c_str(), end.size(), 0);
		}
	}
	else
		sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "NAMES");
	return (0);
}

int	Server::cmdList(std::string Msg, int index, Client &client)
{
	(void) Msg;
	std::string nick = client.GetNickname();
	std::string start = ":" + std::string(SERVER_NAME) + " 321 " + nick + " Channel :Users Name\r\n";
	send(this->_Fds[index].fd, start.c_str(), start.size(), 0);
	for (std::map<std::string, Channel>::iterator it = this->_channels.begin(); it != this->_channels.end(); ++it)
	{
		std::string line = ":" + std::string(SERVER_NAME) + " 322 " + nick + " #" + it->first + " " + intToString(it->second.GetClientCount()) + " :\r\n";
		send(this->_Fds[index].fd, line.c_str(), line.size(), 0);
	}
	std::string end = ":" + std::string(SERVER_NAME) + " 323 " + nick + " :End of /LIST\r\n";
	send(this->_Fds[index].fd, end.c_str(), end.size(), 0);
	return (0);
}

int Server::cmdMode(std::string Msg, int index, Client &client)
{
	std::string nick = client.GetNickname();
	std::stringstream ss(Msg);
	std::string	arg;
	std::vector<std::string> params;

	while (ss >> arg)
		params.push_back(arg);
	if (params.size() < 3)
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "MODE"), EXIT_FAILURE);
	
	std::vector<std::string>::iterator itparameter = params.begin();

	/// Skip "MODE" from list of command -> 'channel' argument
	itparameter++;
	std::string channelName = *itparameter;
	if (channelName[0] == '#')
		channelName.erase(0,1);
	if (this->_channels.find(channelName) == this->_channels.end())
		return (sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, channelName), EXIT_FAILURE);
	if (this->_channels[channelName].GetModerator() != nick)
		return (sendErroMsgCHANNEL(ERR_CHANOPRIVSNEEDED, index, nick, channelName), EXIT_FAILURE);
	
	/// Skip "channel" from list of command -> 'mode' argument
	itparameter++;
	std::string mode = *itparameter;
	
	bool adding = true;
	initMode();
	int i = 0;
	while (itparameter != params.end())
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
			if (itparameter == params.end())
				return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "MODE"), EXIT_FAILURE);
			param = *itparameter;
		}
		if (modeHandler(mode[i], index, adding, nick, channelName, param) == EXIT_FAILURE)
			return (EXIT_FAILURE);
		i++;
		if (!mode[i])
		{
			itparameter++;
			mode = *itparameter;
			i = 0;
		}
	}
	return (EXIT_SUCCESS);
}


int	Server::cmdTopic(std::string Msg, int index, Client &client)
{
	std::string serverName = "ircserv";
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
		sendErroMsg(ERR_NEEDMOREPARAMS, index, nick);
	else
	{
		std::string rest = Msg.substr(spacePos + 1);
		size_t spacePos2 = rest.find(" ");
		std::string channelName = (spacePos2 != std::string::npos) ? rest.substr(0, spacePos2) : rest;
		
		if (!channelName.empty() && channelName[0] == '#')
			channelName.erase(0, 1);
		
		if (this->_channels.find(channelName) == this->_channels.end())
			sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, channelName);
		else if (spacePos2 == std::string::npos)
		{
			std::string topic = this->_channels[channelName].GetTopic();
			if (topic.empty())
				sendInfoMsgCHANNEL(RPL_NOTOPIC, index, nick, channelName);
			else
			{
				std::string response = ":" + serverName + " 332 " + nick + " #" + channelName + " :" + topic + "\r\n";
				response = enforceMessageLimit(response);
				send(this->_Fds[index].fd, response.c_str(), response.size(), 0);
			}
		}
		else
		{
			if (this->_channels[channelName].IsTopicRestricted()
				&& this->_channels[channelName].GetModerator() != nick)
				sendErroMsgCHANNEL(ERR_CHANOPRIVSNEEDED, index, nick, channelName);
			else
			{
				std::string newTopic = rest.substr(spacePos2 + 1);
				if (newTopic.empty() || newTopic[0] != ':')
					return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "TOPIC"), 0);
				newTopic.erase(0, 1);

				this->_channels[channelName].SetTopic(newTopic);
				std::string confirmMsg = ":" + nick + " TOPIC #" + channelName + " :" + newTopic + "\r\n";
				confirmMsg = enforceMessageLimit(confirmMsg);
				send(this->_Fds[index].fd, confirmMsg.c_str(), confirmMsg.size(), 0);
				std::cerr << MAGENTA << nick << " changed topic of #" << channelName << " to: " << newTopic << RESET << std::endl;
			}
		}
	}
	return (0);
}

int	Server::cmdPrivmsg(std::string Msg, int index, Client &client)
{
	std::string senderNick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
		sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, senderNick, "PRIVMSG");
	else
	{
		std::string rest = Msg.substr(spacePos + 1);
		size_t spacePos2 = rest.find(" ");
		if (spacePos2 == std::string::npos || spacePos2 + 1 >= rest.length())
			sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, senderNick, "PRIVMSG");
		else
		{
			std::string target = rest.substr(0, spacePos2);
			std::string message = rest.substr(spacePos2 + 1);
			
			if (message.empty() || message[0] != ':')
				return(sendErroMsg(ERR_NOTEXTTOSEND, index, senderNick), 0);
			message.erase(0, 1);
			
			bool isChannel = (target[0] == '#');
			
			if (isChannel)
			{
				if (target.length() > 1)
					target.erase(0, 1);

				if (this->_channels.find(target) == this->_channels.end())
					sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, senderNick, target);
				else if (!this->_channels[target].ClientExists(senderNick))
					sendErroMsgCHANNEL(ERR_CANNOTSENDTOCHAN, index, senderNick, target);
				else
				{
					std::string privmsgLine = ":" + senderNick + "!" + client.GetUsername() + "@localhost PRIVMSG #" + target + " :" + message + "\r\n";
					privmsgLine = enforceMessageLimit(privmsgLine);
					const std::map<std::string, Client*>& channelClients = this->_channels[target].GetAllClients();
					for (std::map<std::string, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
					{
						if (it->second && it->second->GetFd() != this->_Fds[index].fd)
						send(it->second->GetFd(), privmsgLine.c_str(), privmsgLine.size(), 0);
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
					sendErroMsgKEY(ERR_NOSUCHNICK, index, senderNick, target);
				else
				{
					std::string privmsgLine = ":" + senderNick + "!" + client.GetUsername() + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
					privmsgLine = enforceMessageLimit(privmsgLine);
					send(targetClient->GetFd(), privmsgLine.c_str(), privmsgLine.size(), 0);
					std::cout << CYAN << "[PM] " << senderNick << " -> " << target << ": " << message << RESET << std::endl;
				}
			}
		}
	}
	return (0);
}

int	Server::cmdPart(std::string Msg, int index, Client &client)
{
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
	{
		std::string channelToQuit = Msg.substr(spacePos + 1);
		if (!channelToQuit.empty() && channelToQuit[0] == '#')
			channelToQuit.erase(0, 1);
		if (this->_channels.find(channelToQuit) == this->_channels.end())
			sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, channelToQuit);
		else if (!this->_channels[channelToQuit].ClientExists(nick))
			sendErroMsgCHANNEL(ERR_NOTONCHANNEL, index, nick, channelToQuit);
		else
		{
			std::string partLine = ":" + nick + " PART #" + channelToQuit + "\r\n";
			const std::map<std::string, Client*>& channelClients = this->_channels[channelToQuit].GetAllClients();
			for (std::map<std::string, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
			{
				if (it->second)
				send(it->second->GetFd(), partLine.c_str(), partLine.size(), 0);
			}
			this->_channels[channelToQuit].RemoveClient(nick);
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
		sendErroMsg(ERR_NEEDMOREPARAMS, index, nick);
	return (0);
}

int Server::cmdQuit(std::string Msg, int index, Client &client)
{
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
		return(sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "QUIT"), 0);
	else
	{
		std::string message = Msg.substr(spacePos + 1);
		if (message.empty() || message[0] != ':')
			return(sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "QUIT"), 0);
		message.erase(0, 1);
		std::string nick = client.GetNickname();
	std::string quitLine = ":" + nick + "!" + client.GetUsername() + "@localhost QUIT :" + message + "\r\n";
		for (std::map<int, Client>::iterator it = this->_Client.begin(); it != this->_Client.end(); ++it)
		{
			if (!it->second.GetNickname().empty())
				send(it->second.GetFd(), quitLine.c_str(), quitLine.size(), 0);
		}
		close(this->_Fds[index].fd);
		client.SetErase();
	}
	return (1);
}

int	Server::cmdKick(std::string Msg, int index, Client &client)
{
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
		sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "KICK");
	else
	{
		std::string rest = Msg.substr(spacePos + 1);
		size_t spacePos2 = rest.find(" ");
		if (spacePos2 == std::string::npos || spacePos2 + 1 >= rest.length())
			sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "KICK");
		else
		{
			std::string channelName = rest.substr(0, spacePos2);
			std::string targetNick = rest.substr(spacePos2 + 1);
			
			if (!channelName.empty() && channelName[0] == '#')
				channelName.erase(0, 1);
			if (this->_channels.find(channelName) == this->_channels.end())
				sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, channelName);
			else if (this->_channels[channelName].GetModerator() != nick)
				sendErroMsgCHANNEL(ERR_CHANOPRIVSNEEDED, index, nick, channelName);
			else if (!this->_channels[channelName].ClientExists(targetNick))
				sendErroMsgCHANNEL_KEY(ERR_USERNOTINCHANNEL, index, nick, channelName, targetNick);
			else
			{
				Client* targetClient = this->_channels[channelName].GetClient(targetNick);
				if (targetClient)
				{
					std::string kickLine = ":" + nick + "!" + client.GetUsername() + "@localhost KICK #" + channelName + " " + targetNick + "\r\n";
					const std::map<std::string, Client*>& channelClients = this->_channels[channelName].GetAllClients();
					for (std::map<std::string, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
					{
						if (it->second)
							send(it->second->GetFd(), kickLine.c_str(), kickLine.size(), 0);
					}
					targetClient->SetChannelName("");
					this->_channels[channelName].RemoveClient(targetNick);
					std::cerr << RED << nick << " kicked " << targetNick << " from #" << channelName << RESET << std::endl;
				}
			}
		}
	}
	return (0);
}

int	Server::cmdInvite(std::string Msg, int index, Client &client)
{
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
		sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "INVITE");
	else
	{
		std::string rest = Msg.substr(spacePos + 1);
		size_t spacePos2 = rest.find(" ");
		if (spacePos2 == std::string::npos || spacePos2 + 1 >= rest.length())
			sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "INVITE");
		else
		{
			std::string targetNick = rest.substr(0, spacePos2);
			std::string targetChannel = rest.substr(spacePos2 + 1);
			
			if (!targetChannel.empty() && targetChannel[0] == '#')
				targetChannel.erase(0, 1);
			if (this->_channels.find(targetChannel) == this->_channels.end())
				sendErroMsgCHANNEL(ERR_NOSUCHCHANNEL, index, nick, targetChannel);
			else if (this->_channels[targetChannel].GetModerator() != nick)
				sendErroMsgCHANNEL(ERR_CHANOPRIVSNEEDED, index, nick, targetChannel);
			else
			{
				Client* targetClient = NULL;
				for (std::map<int, Client>::iterator it = this->_Client.begin(); it != this->_Client.end(); ++it)
				{
					if (it->second.GetNickname() == targetNick)
					{
						targetClient = &it->second;
						break;
					}
				}
				if (!targetClient)
					sendErroMsgKEY(ERR_NOSUCHNICK, index, nick, targetNick);
				else if (this->_channels[targetChannel].ClientExists(targetNick))
					sendErroMsgCHANNEL_KEY(ERR_USERONCHANNEL, index, nick, targetChannel, targetNick);
				else
				{
					std::string inviteLine = ":" + nick + "!" + client.GetUsername() + "@localhost INVITE " + targetNick + " #" + targetChannel + "\r\n";
					send(targetClient->GetFd(), inviteLine.c_str(), inviteLine.size(), 0);
					this->_channels[targetChannel].AddClient(targetNick, targetClient);
					targetClient->SetChannelName(targetChannel);
					std::cerr << GREEN << nick << " invited " << targetNick << " to #" << targetChannel << RESET << std::endl;
				}
			}
		}
	}
	return (0);
}
