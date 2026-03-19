# include "server.hpp"
# include "client.hpp"

void	Server::initCmdsAuthentification()
{
	_cmds["PASS"] = &Server::authentificateClientPASS;
	_cmds["NICK"] = &Server::authentificateClientNICK;
	_cmds["USER"] = &Server::authentificateClientUSER;
}

int Server::cmdAuthentificationHandler(std::vector<std::string> argList, int index, Client &client)
{
	std::vector<std::string>::iterator itArg = argList.begin();
	std::map<std::string, cmdPtr>::iterator itcmd = _cmds.find(*itArg);

	if (itcmd != _cmds.end())
	{
		cmdPtr func = itcmd->second;
		(this->*func)(argList, index, client);
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


int	Server::authentificateClientPASS(std::vector<std::string> argList, int index, Client &client)
{
	std::vector<std::string>::iterator itArg = argList.begin();
	if (itArg == argList.end() || *itArg != "PASS")
		return (EXIT_FAILURE);
	std::string nick = client.GetNickname();
	if (nick.empty())
		nick = "*";
	if (client.GetAuthenticated() == true)
		return (sendErroMsg(ERR_ALREADYREGISTRED, index, nick), EXIT_FAILURE);
	itArg++;
	if (itArg != argList.end())
	{
		if (*itArg == this->_password)
			client.SetAuthenticated(true);
		else
			sendErroMsgKEY(ERR_PASSWDMISMATCH, index, "", "PASS");
		return (EXIT_SUCCESS);
	}
	return (EXIT_FAILURE);
}

bool isForbiddenChar(char c)
{
	if (c == '@' || c == '!' || c == '#' || c == '$' || c == '*' || c == ' ')
		return (true);
	return (false);
}

bool	isValideNick(std::string nick)
{
	if (nick.empty())
		return (false);
	if (nick.size() > 9)
		return (false);
	char c = nick[0];
	if (isalpha(c) == false || (c == '[' || c == ']' || c == '\'' || c == '`' || c == '_' || c == '{' || c == '}' || c == '|'))
		return (std::cerr << "j ai failli ici" << std::endl, false);
	for (size_t i = 0; i < nick.size(); i++)
	{
		if (isForbiddenChar(nick[i]) == true)
			return (false);
	}
	return (true);
}

int Server::authentificateClientNICK(std::vector<std::string> argList, int index, Client &client)
{
	std::vector<std::string>::iterator itArg = argList.begin();
	itArg++;
	if (itArg != argList.end())
	{
		std::string newNick = *itArg;
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
			return (sendErroMsg(ERR_NICKNAMEINUSE, index, newNick), EXIT_FAILURE);
		if (isValideNick(newNick) == false)
		{
			std::cerr << GREEN << "wtf : " << newNick << RESET << std::endl;
			return (sendErroMsg(ERR_ERRONEUSNICKNAME, index, newNick), EXIT_FAILURE);
		}
		else
		{
			if (!client.GetNickname().empty())
			{
				std::string updateMsg = std::string(YELLOW) + ":" + client.GetNickname() + "!"
						+ "@localhost NICK :" + newNick + std::string(RESET) + "\r\n";
				send(this->_Fds[index].fd, updateMsg.c_str(), updateMsg.size(), 0);
				std::cerr << YELLOW << client.GetNickname() << " changed nick to " << newNick << RESET << std::endl;
			}
			client.SetNickname(newNick);
		}
	}
	else
			sendErroMsg(ERR_NONICKNAMEGIVEN, index, client.GetNickname());
	return (EXIT_SUCCESS);
}

std::string replaceWrongChar(std::string initUser)
{
	std::string cleanUserName;
	int i = 0;
	if (initUser[i] == ':')
		i++;
	for (int max = initUser.size(); i < max; i++)
	{
		if (isForbiddenChar(initUser[i]) == true)
			cleanUserName += '_';
		else
			cleanUserName += initUser[i];
	}
	return (cleanUserName);
}

int Server::authentificateClientUSER(std::vector<std::string> argList, int index, Client &client)
{
	std::string nick = client.GetNickname();
	if (nick.empty())
		nick = "*";
	if (!client.GetUsername().empty())
		return (sendErroMsg(ERR_ALREADYREGISTRED, index, nick), EXIT_FAILURE);
	std::string userName, mode, unUsed, realName;
	int paramNumber = 0;
	for (std::vector<std::string>::iterator itArg = argList.begin(); itArg != argList.end(); itArg++)
	{
		switch(paramNumber)
		{
			case 1: userName = replaceWrongChar(*itArg); break;
			case 2: mode = *itArg; break;
			case 3: unUsed = *itArg; break;
			case 4: realName = replaceWrongChar(*itArg); break;
		}
		paramNumber++;
	}
	if (paramNumber < 5)
		return (sendErroMsgKEY(ERR_NEEDMOREPARAMS, index, nick, "USER"), EXIT_FAILURE);
	client.SetUsername(userName);
	client.SetRealname(realName);
	return (EXIT_SUCCESS);
}
