#include "server.hpp"

// std::string	Server::getCmdFromMsg(std::string Msg)
// {
// 	std::string cmd;

// 	std::stringstream ss(Msg);
// 	ss >> cmd;
// 	return (cmd);
// }

bool Server::hasColon(std::string Msg)
{
	if (Msg[0] == ':')
		return (true);
	return (false);
}

std::string	Server::removesColon(std::string param)
{
	if (!param.empty() && param[0] != ':')
		return (param);
	std::string cleanparam = param.substr(1);
	return (cleanparam);
}

std::string	colonContentCatcher(std::string Msg)
{
	std::string	content;
	size_t colonPos = Msg.find(":");
	
	content = Msg.substr(colonPos, Msg.length());
	return (content);
}

std::string Server::cleanChannelName(std::string channelName)
{
	std::string cleanName;
	for (size_t i = 0; i < channelName.size(); i++)
	{
		if (i > 50)
			break;
		if (channelName[i] > 'A' && channelName[i] < 'Z')
			cleanName += channelName[i] + 32;
		else if (channelName[i] == ',' || channelName[i] == 7 || channelName[i] == '\0'
				|| channelName[i] == '\r' || channelName[i] == '\n' || channelName[i] == '\a')
			cleanName += '_';
		else
			cleanName += channelName[i];
	}
	return (cleanName);
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
		if (hasColon(arg) == true)
		{
			std::string argColonContent = colonContentCatcher(Msg);
			argList.push_back(argColonContent);
			break;
		}
		argList.push_back(arg);
		countArgs++;
	}
	return (EXIT_SUCCESS);
}