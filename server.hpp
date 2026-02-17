# ifndef SERVER_HPP
	# define SERVER_HPP

# include "lib.hpp"
# include "channel.hpp"

class Client;

class Server
{
	private :
		int	_serverFd;
		int	_port;
		std::string	_password;
		Channel	_channel;
		std::map<std::string, Channel> _channels;
		std::vector <struct pollfd>	_Fds;
		std::map<int, Client>		_Client;
	public :
		Server();
		Server(std::string port, std::string password);
		Server(const Server &other);
		Server& operator=(const Server &other);
		~Server();

		
		/// Setters
		void	SetServerFd(int serverFd);
		void	AddSocketFds(pollfd fd);
		const struct sockaddr*	setSocketAdress();
		void	SetUpSignals();
		
		/// Getters
		int	GetServerFd() const;
		int	GetPort() const;
		std::string	GetPassword() const;
		// std::vector<struct poolfd> GetListFd() const;
		std::vector<struct pollfd> GetFdsContainer() const;
		struct pollfd GetFds(int index) const;
		int		SizeList();
		struct pollfd&	operator[](size_t index);

		/// Fonctions
		int		setSocket(Server *server);
		int		nonBlocking(int fd);
		int		bindFt();
		int		pollLoop();
		int		acceptFd(int index);
		void	disconnectClient(int nbrClient);
		int		clientJoiningServer(int index);
		int		clientquittingServer(int index, char* buffer);
		int		clientSendingMessage(int index, char* buffer, size_t bytesSize);
		void	closeFds();
};
std::ostream& operator<<(std::ostream &out, const Server &other);

# endif