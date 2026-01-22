# include <iostream>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
# include "server.hpp"

int main(void)
{
	Server server;
	if (server.setSocket(&server) == EXIT_FAILURE)
		return (EXIT_SUCCESS);
	server.bindFt();
}