# include <iostream>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include "server.hpp"

int main(void)
{
	Server server("6067");
	if (server.setSocket(&server) == EXIT_FAILURE)
		return (EXIT_FAILURE);
	if (server.bindFt() != 0)
		return (EXIT_FAILURE);
	while (1)
	{
		if (listen(server.GetServerFd(), SOMAXCONN) != 0)
		{
			perror("listen");
			return (1);
		}
		while (1)
		{
			int pollStatus = poll(server.GetFdsList().data(), server.GetFdsList().size(), -1);
			if (pollStatus == -1)
			{
				perror("poll");
				return (1);
			}
			if (pollStatus )
		}
	}
	return (0);
}