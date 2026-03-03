# include "server.hpp"
# include "client.hpp"

void	Server::initCmdsAuthentification()
{
	_cmds["PASS"] = &Server::authentificateClientPASS;
	_cmds["NICK"] = &Server::authentificateClientNICK;
	_cmds["USER"] = &Server::authentificateClientUSER;
}

int Server::cmdAuthentificationHandler(std::string Msg, int index, Client &client)
{
	std::string cmd = getCmdFromMsg(Msg);
	std::map<std::string, cmdPtr>::iterator it = _cmds.find(cmd);

	if (it != _cmds.end())
	{
		cmdPtr func = it->second;
		(this->*func)(Msg, index, client);
	}
	else
		return (EXIT_FAILURE);
	if (!client.GetNickname().empty() && !client.GetUsername().empty())
	{
		/// Server log :
		std::cerr << GREEN << client.GetNickname() << " Joined Server" << RESET << std::endl;
		
		/// Client log :
		sendWelcomeMsg(index, client.GetNickname());
	}
	return (EXIT_SUCCESS);
}

int	Server::authentificateClientPASS(std::string Msg, int index, Client &client)
{
	std::string cmd = getCmdFromMsg(Msg);
	if (cmd == "PASS")
	{
		size_t spacePos = Msg.find(" ");
		if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
		{
			std::string pass = Msg.substr(spacePos + 1);
			if (pass == this->_password)
				client.SetAuthenticated(true);
			else
				sendErroMsgKEY(ERR_PASSWDMISMATCH, index, "", "PASS");
		}
		else
			sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, "", "PASS");
		return (EXIT_SUCCESS);
	}
	return (EXIT_FAILURE);
}

int Server::authentificateClientNICK(std::string Msg, int index, Client &client)
{
	size_t spacePos = Msg.find(" ");
	if (spacePos != std::string::npos && spacePos + 1 < Msg.length())
		{
			std::string newNick = Msg.substr(spacePos + 1);
			// Check if nickname is already in use
			bool nickExists = false;
			for (std::map<int, Client>::iterator it = this->_Client.begin(); it != this->_Client.end(); ++it)
			{
				if (it->second.GetNickname() == newNick && it->first != this->_Fds[index].fd)
				{
					nickExists = true;
					break;
				}
			}
			if (nickExists)
				sendErroMsg(ERR_NICKNAMEINUSE, index, newNick);
			else
				client.SetNickname(newNick);
		}
	else
			sendErroMsg(ERR_NONICKNAMEGIVEN, index, client.GetNickname());
	return (EXIT_SUCCESS);
}

int Server::authentificateClientUSER(std::string Msg, int index, Client &client)
{
	std::string serverName = "ircserv";
	std::string nick = client.GetNickname();
	if (nick.empty())
		nick = "*";
	size_t firstSpace = Msg.find(" ");
	if (firstSpace != std::string::npos && firstSpace + 1 < Msg.length())
	{
		std::string rest = Msg.substr(firstSpace + 1);
		size_t secondSpace = rest.find(" ");
		if (secondSpace != std::string::npos && secondSpace + 1 < rest.length())
		{
			std::string username = rest.substr(0, secondSpace);
			rest = rest.substr(secondSpace + 1);
			size_t thirdSpace = rest.find(" ");
			if (thirdSpace != std::string::npos && thirdSpace + 1 < rest.length())
			{
				rest = rest.substr(thirdSpace + 1);
				size_t colonPos = rest.find(":");
				if (colonPos == std::string::npos || colonPos + 1 >= rest.length())
					sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "USER");
				else
				{
					std::string realname = rest.substr(colonPos + 1);
					client.SetUsername(username);
					client.SetRealname(realname);
				}
			}
			else
				sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "USER");
		}
		else
			sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "USER");
	}
	else
		sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "USER");

	return (EXIT_SUCCESS);
}
