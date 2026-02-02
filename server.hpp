# ifndef SERVER_HPP
	# define SERVER_HPP

# include <iostream>
# include <set>
# include <sstream>
# include <poll.h>
# include <map>
# include <sys/socket.h>
# include <netinet/in.h>
# include <fcntl.h>
# include "client.hpp"
# include <unistd.h>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"

class Client;

class Server
{
	private :
		int	_serverFd;
		int _port;
		std::vector<struct pollfd>	_Fds;
		std::map<int, Client>		_Client;
	public :
		Server();
		Server(std::string port);
		Server(const Server &other);
		Server& operator=(const Server &other);
		~Server();

		
		/// Setters
		void	SetServerFd(int serverFd);
		void	AddSocketFds(pollfd fd);
		const struct sockaddr* setSocketAdress();
		
		/// Getters
		int	GetServerFd() const;
		int GetPort() const;
		// std::vector<struct poolfd> GetListFd() const;
		std::vector<struct pollfd> GetFdsContainer() const;
		struct pollfd GetFds(int index) const;
		int		SizeList();
		struct pollfd&	operator[](size_t index);

		/// Fonctions
		int setSocket(Server *server);
		int	nonBlocking(int fd);
		int bindFt();
		int pollLoop();
		int acceptFd(int index);
};
std::ostream& operator<<(std::ostream &out, const Server &other);

# endif