#include "server.hpp"

std::string	Server::getCmdFromMsg(std::string Msg)
{
	std::string cmd;

	size_t start = Msg.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return (Msg);
	size_t spacePos = Msg.find(' ');
	cmd = Msg.substr(start, spacePos);
	return (cmd);
}

