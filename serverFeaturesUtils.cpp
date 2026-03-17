#include "server.hpp"

std::string	Server::getCmdFromMsg(std::string Msg)
{
	std::string cmd;

	std::stringstream ss(Msg);
	ss >> cmd;
	return (cmd);
}

bool Server::hasColon(std::string Msg)
{
	if (Msg[0] == ':')
		return (true);
	return (false);
}

std::string	colonContentCatcher(std::string Msg)
{
	std::string	content;
	size_t colonPos = Msg.find(":");
	
	content = Msg.substr(colonPos, Msg.length());
	return (content);
}

int	Server::splitArgs(std::vector<std::string> &argList, std::string Msg)
{
	if (Msg.empty())
		return (EXIT_FAILURE);
	std::stringstream ss(Msg);
	std::string arg;
	int countArgs = 0;
	while (ss >> arg)
	{
		if (countArgs > 14)
		{
			std::string leftOfArgs;
			std::getline(ss, leftOfArgs);
			argList.push_back(leftOfArgs);
			break;
		}
		argList.push_back(arg);
		if (hasColon(arg) == true)
		{
			std::string argColonContent = colonContentCatcher(Msg);
			argList.push_back(argColonContent);
			break;
		}
		countArgs++;
	}
	return (EXIT_SUCCESS);
}