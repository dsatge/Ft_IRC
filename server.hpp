# ifndef SERVER_HPP
	# define SERVER_HPP

# include <iostream>
# include <sstream>
# include <poll.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <fcntl.h>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"

class Server
{
	private :
		int	_serverFd;
		int _port;
		std::vector<struct pollfd>	_Fds;
	public :
		Server();
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
		int		Size();
		struct pollfd&	operator[](size_t index);

		/// Fonctions
		int setSocket(Server *server);
		int	nonBlocking(int fd);
		void bindFt();
};
std::ostream& operator<<(std::ostream &out, const Server &other);

# endif