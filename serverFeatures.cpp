# include "server.hpp"
# include "client.hpp"

void	Server::initCmds()
{
	_cmds["HELP"] = &Server::cmdHelp;
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
	else if (!client.GetChannelName().empty())
		msgChannel(Msg, index, client);
	else
		std::cerr << "Command not found: " << Msg << std::endl;
	return (quitReturn);
}


////////////////////////////////////////////////////////////////////////////////
/////////////////////////     FEATURES DEFINITIONS     /////////////////////////
////////////////////////////////////////////////////////////////////////////////


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

int	Server::cmdJoin(std::string Msg, int index, Client &client)
{
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
			if (this->_channels[channelName].GetUserLimit() > 0
				&& this->_channels[channelName].GetClientCount() >= this->_channels[channelName].GetUserLimit())
			{
				std::string err = ":ircserv 471 " + client.GetNickname() + " #" + channelName + " :Cannot join channel (+l)\r\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				return (0);
			}
			if (!this->_channels[channelName].GetKey().empty() && key != this->_channels[channelName].GetKey())
			{
				std::string err = ":ircserv 475 " + client.GetNickname() + " #" + channelName + " :Cannot join channel (+k)\r\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				return (0);
			}
			if (this->_channels[channelName].IsInviteOnly()
					&& this->_channels[channelName].GetModerator() != client.GetNickname()
					&& !this->_channels[channelName].ClientExists(client.GetNickname()))
			{
				std::string err = "Channel #" + channelName + " is invite-only.\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				return (0);
			}
		}
		std::string oldChannel = client.GetChannelName();
		if (!oldChannel.empty() && this->_channels.find(oldChannel) != this->_channels.end())
		{
			this->_channels[oldChannel].RemoveClient(client.GetNickname());
			if (this->_channels[oldChannel].GetClientCount() == 0)
			{
				this->_channels.erase(oldChannel);
			}
		}
		client.SetChannelName(channelName);
		if (this->_channels.find(channelName) == this->_channels.end())
		{
			Channel newChannel(channelName);
			this->_channels[channelName] = newChannel;
			this->_channels[channelName].SetModerator(client.GetNickname());
			std::cerr << MAGENTA << client.GetNickname() << " is now moderator of #" << channelName << RESET << std::endl;
		}
		else if (this->_channels[channelName].GetClientCount() == 0)
		{
			this->_channels[channelName].SetModerator(client.GetNickname());
			std::cerr << MAGENTA << client.GetNickname() << " is now moderator of #" << channelName << RESET << std::endl;
		}
		this->_channels[channelName].AddClient(client.GetNickname(), &client);
		std::string joinMsg = ":" + client.GetNickname() + "!" + client.GetUsername() + "@localhost JOIN #" + channelName + "\r\n";
		send(this->_Fds[index].fd, joinMsg.c_str(), joinMsg.size(), 0);
		std::cerr << GREEN << client.GetNickname() << " joined channel " << channelName << RESET << std::endl;
	}
	else
	{
		std::string err = "Usage: JOIN <channel_name>\n";
		send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
	}
	return (0);
}

int Server::cmdNames(std::string Msg, int index, Client &client)
{
	std::string serverName = "ircserv";
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
	{
		std::string channelName = Msg.substr(spacePos + 1);
		if (!channelName.empty() && channelName[0] == '#')
			channelName.erase(0, 1);
		
		if (this->_channels.find(channelName) == this->_channels.end())
		{
			std::string err = ":" + serverName + " 403 " + nick + " #" + channelName + " :No such channel\r\n";
			send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
		}
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
			std::string response = ":" + serverName + " 353 " + nick + " = #" + channelName + " :" + usersLine + "\r\n";
			send(this->_Fds[index].fd, response.c_str(), response.size(), 0);
			std::string end = ":" + serverName + " 366 " + nick + " #" + channelName + " :End of /NAMES list.\r\n";
			send(this->_Fds[index].fd, end.c_str(), end.size(), 0);
		}
	}
	else
	{
		std::string err = ":" + serverName + " 461 " + nick + " NAMES :Not enough parameters\r\n";
		send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
	}
	return (0);
}

int	Server::cmdList(std::string Msg, int index, Client &client)
{
	(void) Msg;
	std::string serverName = "ircserv";
	std::string nick = client.GetNickname();
	std::string start = ":" + serverName + " 321 " + nick + " Channel :Users Name\r\n";
	send(this->_Fds[index].fd, start.c_str(), start.size(), 0);
	for (std::map<std::string, Channel>::iterator it = this->_channels.begin(); it != this->_channels.end(); ++it)
	{
		std::string line = ":" + serverName + " 322 " + nick + " #" + it->first + " " + intToString(it->second.GetClientCount()) + " :\r\n";
		send(this->_Fds[index].fd, line.c_str(), line.size(), 0);
	}
	std::string end = ":" + serverName + " 323 " + nick + " :End of /LIST\r\n";
	send(this->_Fds[index].fd, end.c_str(), end.size(), 0);
	return (0);
}

int Server::cmdMode(std::string Msg, int index, Client &client)
{
	std::string serverName = "ircserv";
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
	{
		std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
		send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
	}
	else
	{
		std::string rest = Msg.substr(spacePos + 1);
		size_t spacePos2 = rest.find(" ");
		std::string channelName = (spacePos2 != std::string::npos) ? rest.substr(0, spacePos2) : rest;
		
		if (!channelName.empty() && channelName[0] == '#')
			channelName.erase(0, 1);
		
		if (this->_channels.find(channelName) == this->_channels.end())
		{
			std::string err = ":" + serverName + " 403 " + nick + " #" + channelName + " :No such channel\r\n";
			send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
		}
		else if (spacePos2 == std::string::npos)
		{
			std::string modes = "+";
			if (this->_channels[channelName].IsInviteOnly())
				modes += "i";
			if (this->_channels[channelName].IsTopicRestricted())
				modes += "t";
			if (!this->_channels[channelName].GetKey().empty())
				modes += "k";
			if (this->_channels[channelName].GetUserLimit() > 0)
				modes += "l";
			std::string reply = ":" + serverName + " 324 " + nick + " #" + channelName + " " + modes + "\r\n";
			send(this->_Fds[index].fd, reply.c_str(), reply.size(), 0);
		}
		else if (this->_channels[channelName].GetModerator() != nick)
		{
			std::string err = ":" + serverName + " 482 " + nick + " #" + channelName + " :You're not channel operator\r\n";
			send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
		}
		else
		{
			std::string modeParams = rest.substr(spacePos2 + 1);
			std::stringstream ss(modeParams);
			std::string modeStr;
			ss >> modeStr;
			if (modeStr.empty())
			{
				std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				return (0);
			}
			std::vector<std::string> params;
			std::string param;
			while (ss >> param)
				params.push_back(param);
			bool adding = true;
			size_t paramIndex = 0;
			bool modeError = false;
			for (size_t i = 0; i < modeStr.size(); ++i)
			{
				char m = modeStr[i];
				if (m == '+')
				{
					adding = true;
					continue;
				}
				if (m == '-')
				{
					adding = false;
					continue;
				}
				switch (m)
				{
					case 'i':
						this->_channels[channelName].SetInviteOnly(adding);
						break;
					case 't':
						this->_channels[channelName].SetTopicRestricted(adding);
						break;
					case 'k':
						if (adding)
						{
							if (paramIndex >= params.size())
							{
								std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								modeError = true;
							}
							else
							{
								this->_channels[channelName].SetKey(params[paramIndex++]);
							}
						}
						else
						{
							this->_channels[channelName].ClearKey();
						}
						break;
					case 'o':
						if (paramIndex >= params.size())
						{
							std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							modeError = true;
							break;
						}
						{
							std::string target = params[paramIndex++];
							if (!this->_channels[channelName].ClientExists(target))
							{
								std::string err = ":" + serverName + " 441 " + nick + " " + target + " #" + channelName + " :They are not on that channel\r\n";
								send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
								modeError = true;
								break;
							}
							if (adding)
								this->_channels[channelName].SetModerator(target);
							else if (this->_channels[channelName].GetModerator() == target)
								this->_channels[channelName].SetModerator("");
						}
						break;
						case 'j':
						case 'l':
							if (adding)
							{
								if (paramIndex >= params.size())
								{
									std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
									send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
									modeError = true;
								}
								else
								{
									int limit = std::atoi(params[paramIndex++].c_str());
									if (limit <= 0)
									{
										std::string err = ":" + serverName + " 461 " + nick + " MODE :Not enough parameters\r\n";
										send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
										modeError = true;
									}
									else
									{
										this->_channels[channelName].SetUserLimit(limit);
									}
								}
							}
							else
							{
								this->_channels[channelName].ClearUserLimit();
							}
							break;
						default:
						{
							std::string err = ":" + serverName + " 472 " + nick + " " + m + " :is unknown mode char to me\r\n";
							send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
							modeError = true;
							break;
						}
				}
			if (modeError)
				break;
			}
		}
	}
	return (0);
}

int	Server::cmdTopic(std::string Msg, int index, Client &client)
{
	std::string serverName = "ircserv";
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
	{
		std::string err = ":" + serverName + " 461 " + nick + " TOPIC :Not enough parameters\r\n";
		send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
	}
	else
	{
		std::string rest = Msg.substr(spacePos + 1);
		size_t spacePos2 = rest.find(" ");
		std::string channelName = (spacePos2 != std::string::npos) ? rest.substr(0, spacePos2) : rest;
		
		if (!channelName.empty() && channelName[0] == '#')
			channelName.erase(0, 1);
		
		if (this->_channels.find(channelName) == this->_channels.end())
		{
			std::string err = ":" + serverName + " 403 " + nick + " #" + channelName + " :No such channel\r\n";
			send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
		}
		else if (spacePos2 == std::string::npos)
		{
			// Query topic
			std::string topic = this->_channels[channelName].GetTopic();
			if (topic.empty())
			{
				std::string response = ":" + serverName + " 331 " + nick + " #" + channelName + " :No topic is set\r\n";
				send(this->_Fds[index].fd, response.c_str(), response.size(), 0);
			}
			else
			{
				std::string response = ":" + serverName + " 332 " + nick + " #" + channelName + " :" + topic + "\r\n";
				send(this->_Fds[index].fd, response.c_str(), response.size(), 0);
			}
		}
		else
		{
			// Set topic
			if (this->_channels[channelName].IsTopicRestricted()
				&& this->_channels[channelName].GetModerator() != nick)
			{
				std::string err = ":" + serverName + " 482 " + nick + " #" + channelName + " :You're not channel operator\r\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
			}
			else
			{
				std::string newTopic = rest.substr(spacePos2 + 1);
				if (newTopic.empty() || newTopic[0] != ':')
				{
					std::string err = ":" + serverName + " 461 " + nick + " TOPIC :Not enough parameters\r\n";
					send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
					return (0);
				}
				newTopic.erase(0, 1);

				if (newTopic.size() >= 50)
				{
					std::string err = "Topic must be less than 50 characters.\n";
					send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				}
				else
				{
					this->_channels[channelName].SetTopic(newTopic);
					std::string confirmMsg = ":" + nick + " TOPIC #" + channelName + " :" + newTopic + "\r\n";
					send(this->_Fds[index].fd, confirmMsg.c_str(), confirmMsg.size(), 0);
					std::cerr << MAGENTA << nick << " changed topic of #" << channelName << " to: " << newTopic << RESET << std::endl;
				}
			}
		}
	}
	return (0);
}

int	Server::cmdPrivmsg(std::string Msg, int index, Client &client)
{
	std::string serverName = "ircserv";
	std::string senderNick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
	{
		std::string err = ":" + serverName + " 461 " + senderNick + " PRIVMSG :Not enough parameters\r\n";
		send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
	}
	else
	{
		std::string rest = Msg.substr(spacePos + 1);
		size_t spacePos2 = rest.find(" ");
		if (spacePos2 == std::string::npos || spacePos2 + 1 >= rest.length())
		{
			std::string err = ":" + serverName + " 461 " + senderNick + " PRIVMSG :Not enough parameters\r\n";
			send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
		}
		else
		{
			std::string target = rest.substr(0, spacePos2);
			std::string message = rest.substr(spacePos2 + 1);
			
			if (message.empty() || message[0] != ':')
			{
				std::string err = ":" + serverName + " 412 " + senderNick + " :No text to send\r\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				return (0);
			}
			message.erase(0, 1);
			
			// Check if target is a channel or a user
			bool isChannel = (target[0] == '#');
			
			if (isChannel)
			{
				// Message to channel (RFC 2812)
				if (target.length() > 1)
					target.erase(0, 1);

				if (this->_channels.find(target) == this->_channels.end())
				{
					std::string err = ":" + serverName + " 403 " + senderNick + " #" + target + " :No such channel\r\n";
					send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				}
				else
				{
					std::string privmsgLine = ":" + senderNick + "!" + client.GetUsername() + "@localhost PRIVMSG #" + target + " :" + message + "\r\n";
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
				// Message to user (RFC 2812)
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
				{
					std::string err = ":" + serverName + " 401 " + senderNick + " " + target + " :No such nick\r\n";
					send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				}
				else
				{
					std::string privmsgLine = ":" + senderNick + "!" + client.GetUsername() + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
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
	std::string serverName = "ircserv";
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
	{
		std::string channelToQuit = Msg.substr(spacePos + 1);
		if (!channelToQuit.empty() && channelToQuit[0] == '#')
			channelToQuit.erase(0, 1);
		if (this->_channels.find(channelToQuit) == this->_channels.end())
		{
			std::string err = ":" + serverName + " 403 " + nick + " #" + channelToQuit + " :No such channel\r\n";
			send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
		}
		else if (!this->_channels[channelToQuit].ClientExists(nick))
		{
			std::string err = ":" + serverName + " 442 " + nick + " #" + channelToQuit + " :You're not on that channel\r\n";
			send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
		}
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
	{
		std::string err = ":" + serverName + " 461 " + nick + " PART :Not enough parameters\r\n";
		send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
	}
	return (0);
}

int Server::cmdQuit(std::string Msg, int index, Client &client)
{
	std::string serverName = "ircserv";
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
	{
		std::string err = ":" + serverName + " 461 " + nick + " QUIT :Not enough parameters\r\n";
		send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
		return (0);
	}
	else
	{
		std::string message = Msg.substr(spacePos + 1);
		if (message.empty() || message[0] != ':')
		{
			std::string err = ":" + serverName + " 461 " + nick + " QUIT :Not enough parameters\r\n";
			send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
			return (0);
		}
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
	std::string serverName = "ircserv";
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
	{
		std::string err = ":" + serverName + " 461 " + nick + " KICK :Not enough parameters\r\n";
		send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
	}
	else
	{
		std::string rest = Msg.substr(spacePos + 1);
		size_t spacePos2 = rest.find(" ");
		if (spacePos2 == std::string::npos || spacePos2 + 1 >= rest.length())
		{
			std::string err = ":" + serverName + " 461 " + nick + " KICK :Not enough parameters\r\n";
			send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
		}
		else
		{
			std::string channelName = rest.substr(0, spacePos2);
			std::string targetNick = rest.substr(spacePos2 + 1);
			
			if (!channelName.empty() && channelName[0] == '#')
				channelName.erase(0, 1);
			if (this->_channels.find(channelName) == this->_channels.end())
			{
				std::string err = ":" + serverName + " 403 " + nick + " #" + channelName + " :No such channel\r\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
			}
			else if (this->_channels[channelName].GetModerator() != nick)
			{
				std::string err = ":" + serverName + " 482 " + nick + " #" + channelName + " :You're not channel operator\r\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
			}
			else if (!this->_channels[channelName].ClientExists(targetNick))
			{
				std::string err = ":" + serverName + " 441 " + nick + " " + targetNick + " #" + channelName + " :They are not on that channel\r\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
			}
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
	std::string serverName = "ircserv";
	std::string nick = client.GetNickname();
	size_t spacePos = Msg.find(" ");
	if (spacePos == std::string::npos || spacePos + 1 >= Msg.length())
	{
		std::string err = ":" + serverName + " 461 " + nick + " INVITE :Not enough parameters\r\n";
		send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
	}
	else
	{
		std::string rest = Msg.substr(spacePos + 1);
		size_t spacePos2 = rest.find(" ");
		if (spacePos2 == std::string::npos || spacePos2 + 1 >= rest.length())
		{
			std::string err = ":" + serverName + " 461 " + nick + " INVITE :Not enough parameters\r\n";
			send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
		}
		else
		{
			std::string targetNick = rest.substr(0, spacePos2);
			std::string targetChannel = rest.substr(spacePos2 + 1);
			
			if (!targetChannel.empty() && targetChannel[0] == '#')
				targetChannel.erase(0, 1);
			if (this->_channels.find(targetChannel) == this->_channels.end())
			{
				std::string err = ":" + serverName + " 403 " + nick + " #" + targetChannel + " :No such channel\r\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
			}
			else if (this->_channels[targetChannel].GetModerator() != nick)
			{
				std::string err = ":" + serverName + " 482 " + nick + " #" + targetChannel + " :You're not channel operator\r\n";
				send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
			}
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
				{
					std::string err = ":" + serverName + " 401 " + nick + " " + targetNick + " :No such nick\r\n";
					send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				}
				else if (this->_channels[targetChannel].ClientExists(targetNick))
				{
					std::string err = ":" + serverName + " 443 " + nick + " " + targetNick + " #" + targetChannel + " :User is already on channel\r\n";
					send(this->_Fds[index].fd, err.c_str(), err.size(), 0);
				}
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

int	Server::msgChannel(std::string Msg, int index, Client &client)
{
	std::string channelName = client.GetChannelName();
	if (this->_channels.find(channelName) != this->_channels.end())
	{
		std::string fullMsg = client.GetNickname() + ": " + Msg + "\n";
		const std::map<std::string, Client*>& channelClients = this->_channels[channelName].GetAllClients();
		for (std::map<std::string, Client*>::const_iterator it = channelClients.begin(); it != channelClients.end(); ++it)
		{
			if (it->second && it->second->GetFd() != this->_Fds[index].fd)
				send(it->second->GetFd(), fullMsg.c_str(), fullMsg.size(), 0);
		}
		std::cout << CYAN << "[" << channelName << "] " << client.GetNickname() << ": " << Msg << RESET << std::endl;
	}
	return (0);
}
