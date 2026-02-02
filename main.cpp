# include <iostream>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include "server.hpp"
# include "client.hpp"

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
		server.pollLoop();
	}
	return (0);
}