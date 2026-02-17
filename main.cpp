# include "server.hpp"
# include "client.hpp"
# include "config_server.hpp"
# include "lib.hpp"

int main(int ac, char **av)
{
	if (ac != 3)
	{
		std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
		return (EXIT_FAILURE);
	}

	ConfigServer configServer(av[1], av[2]);

	if (configServer.check_server(configServer.GetPort(), configServer.GetPassword()) == EXIT_FAILURE)
		return (EXIT_FAILURE);

	Server server(configServer.GetPort(), configServer.GetPassword());

	if (server.setSocket(&server) == EXIT_FAILURE)
		return (EXIT_FAILURE);
	if (server.bindFt() != 0)
		return (EXIT_FAILURE);
	if (listen(server.GetServerFd(), SOMAXCONN) != 0)
	{
		perror("listen");
		return (EXIT_FAILURE);
	}
	server.pollLoop();
	std::cerr << GREEN << "EXITING" << RESET << std::endl;
	return (EXIT_SUCCESS);
}
