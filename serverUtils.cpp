# include "server.hpp"
# include "client.hpp"

void	Server::initErrorMsg()
{
	// ERRORS
	_ErrorMsg[ERR_PASSWDMISMATCH] = "Password incorrect";
	_ErrorMsg[ERR_ALREADYREGISTRED] = "Unauthorized command (already registered)";
	_ErrorMsg[ERR_NEEDMOREPARAMS] = "Not enough parameters";
	_ErrorMsg[ERR_NOTREGISTERED] = "You have not registered";
	_ErrorMsg[ERR_NICKNAMEINUSE] = "Nickname is already in use";
	_ErrorMsg[ERR_ERRONEUSNICKNAME] = "Erroneous nickname";
	_ErrorMsg[ERR_NONICKNAMEGIVEN] = "No nickname given";
	_ErrorMsg[ERR_UNKNOWNCOMMAND] = "Unknown command";
	_ErrorMsg[ERR_NOTEXTTOSEND] = "No text to send";
	_ErrorMsg[ERR_NOORIGIN] = "No origin specified";
	_ErrorMsg[ERR_NOSUCHNICK] = "No such nick";

	/// ERRORS FOR CHANNELS
	_ErrorMsg[ERR_CHANOPRIVSNEEDED] = "You're not channel operator";
	_ErrorMsg[ERR_BADCHANNELKEY] = "Cannot join channel (+k)";
	_ErrorMsg[ERR_INVITEONLYCHAN] = "Cannot join channel (+i)";
	_ErrorMsg[ERR_UNKNOWNMODE] = "is unknown mode char to me";
	_ErrorMsg[ERR_CHANNELISFULL] = "Cannot join channel (+l)";
	_ErrorMsg[ERR_USERONCHANNEL] = "is already on channel";
	_ErrorMsg[ERR_NOTONCHANNEL] = "You're not on that channel";
	_ErrorMsg[ERR_USERNOTINCHANNEL] = "They are not on that channel";
	_ErrorMsg[ERR_CANNOTSENDTOCHAN] = "Cannot send to channel";
	_ErrorMsg[ERR_NOSUCHCHANNEL] = "No such channel";	

	/// RETURNS MSG
	_ErrorMsg[RPL_NOTOPIC] = "No topic is set";
}

int	Server::sendInfoMsg(int infoCode, int index, std::string target, Client &client)
{
	if (target.empty())
		target = "*";
	std::map<int, std::string>::iterator itMsg = _ErrorMsg.find(infoCode);
	if (itMsg != _ErrorMsg.end())
	{
		// std::string infoMsg = std::string(BLUE) + ":" + SERVER_NAME + " " + intToString(infoCode)
		// 		+ " " + target + " :" + itMsg->second + RESET + "\r\n";
		std::string infoMsg = ":" + std::string(SERVER_NAME) + " " + intToString(infoCode)
				+ " " + target + " :" + itMsg->second + RESET + "\r\n";
		if (send(this->_Fds[index].fd, infoMsg.c_str(), infoMsg.size(), MSG_NOSIGNAL) == -1)
			return (client.SetErase(), EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

int	Server::sendInfoMsgCHANNEL(int infoCode, int index, std::string target, std::string channel, Client &client)
{
	if (target.empty())
		target = "*";
	std::map<int, std::string>::iterator itMsg = _ErrorMsg.find(infoCode);
	if (itMsg != _ErrorMsg.end())
	{
		// std::string infoMsg = std::string(BLUE) + ":" + SERVER_NAME + " " + intToString(infoCode)
		// 		+ " " + target + " #" + channel + " :" + itMsg->second + RESET + "\r\n";
		std::string infoMsg = ":" + std::string(SERVER_NAME) + " " + intToString(infoCode)
				+ " " + target + " #" + channel + " :" + itMsg->second + RESET + "\r\n";
		if (send(this->_Fds[index].fd, infoMsg.c_str(), infoMsg.size(), MSG_NOSIGNAL) == -1)
			return (client.SetErase(), EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}


int	Server::sendErroMsg(int errorCode, int index, std::string target, Client &client)
{
	if (target.empty())
		target = "*";
	std::map<int, std::string>::iterator itError = _ErrorMsg.find(errorCode);
	if (itError != _ErrorMsg.end())
	{
		// std::string errorMsg = std::string(RED) + ":" + SERVER_NAME + " " + intToString(errorCode)
		// 		+ " " + target + " :" + itError->second + RESET + "\r\n";
		std::string errorMsg = ":" + std::string(SERVER_NAME) + " " + intToString(errorCode)
				+ " " + target + " :" + itError->second + RESET + "\r\n";
		if (send(this->_Fds[index].fd, errorMsg.c_str(), errorMsg.size(), MSG_NOSIGNAL) == -1)
			return (client.SetErase(), EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

int	Server::sendErroMsgCHANNEL(int errorCode, int index, std::string target, std::string channel, Client &client)
{
	if (target.empty())
		target = "*";
	std::map<int, std::string>::iterator itError = _ErrorMsg.find(errorCode);
	if (itError != _ErrorMsg.end())
	{
		std::string errorMsg = ":" + std::string(SERVER_NAME) + " " + intToString(errorCode)
				+ " " + target + " #" + channel + " :" + itError->second + "\r\n";
		if (send(this->_Fds[index].fd, errorMsg.c_str(), errorMsg.size(), MSG_NOSIGNAL) == -1)
			return (client.SetErase(), EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

int	Server::sendErroMsgCHANNEL_KEY(int errorCode, int index, std::string target, std::string channel, std::string key, Client &client)
{
	if (target.empty())
		target = "*";
	std::map<int, std::string>::iterator itError = _ErrorMsg.find(errorCode);
	if (itError != _ErrorMsg.end())
	{
		std::string errorMsg = ":" + std::string(SERVER_NAME) + " " + intToString(errorCode)
				+ " " + target + " " + key + " #" + channel + " :" + itError->second + "\r\n";
		if (send(this->_Fds[index].fd, errorMsg.c_str(), errorMsg.size(), MSG_NOSIGNAL) == -1)
			return (client.SetErase(), EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}


int	Server::sendErroMsgKEY(int errorCode, int index, std::string target, std::string keyword, Client &client)
{
	if (target.empty())
		target = "*";
	std::map<int, std::string>::iterator itError = _ErrorMsg.find(errorCode);
	if (itError != _ErrorMsg.end())
	{
		std::string errorMsg = ":" + std::string(SERVER_NAME) + " " + intToString(errorCode)
				+ " " + target + " " + keyword + " :" + itError->second + "\r\n";
		if (send(this->_Fds[index].fd, errorMsg.c_str(), errorMsg.size(), MSG_NOSIGNAL) == -1)
			return (client.SetErase(), EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

std::string	Server::dateSetUp()
{
	std::time_t now = std::time(0);
	char buffer[80];

	std::strftime(buffer, 80, "%a %b %d %Y %H:%M:%S", std::localtime(&now));
	return (buffer);
}

int	Server::sendWelcomeMsg(int index, std::string target, Client &client)
{
	std::string welcome;
	welcome += ":" + std::string(SERVER_NAME) + " 001 " + target + " :Welcome to the IRC network " + target + "\r\n";
	welcome += ":" + std::string(SERVER_NAME) + " 002 " + target + " :Your host is " + std::string(SERVER_NAME) + ", running version " + VERSION + "\r\n";
	welcome += ":" + std::string(SERVER_NAME) + " 003 " + target + " :This server was created " + _date + "\r\n";
	welcome += ":" + std::string(SERVER_NAME) + " 004 " + target + " " + std::string(SERVER_NAME) + " " + VERSION + " " + USER_MODES + " " + CHANNEL_MODES + "\r\n";
	
	if (send(this->_Fds[index].fd, welcome.c_str(), welcome.size(), MSG_NOSIGNAL) == -1)
			return (client.SetErase(), EXIT_FAILURE);
	return (EXIT_SUCCESS);
	
	// std::string ok = "Use HELP to see available commands.\n";
	// send(this->_Fds[index].fd, ok.c_str(), ok.size(), 0);
}
