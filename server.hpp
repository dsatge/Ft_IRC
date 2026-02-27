# ifndef SERVER_HPP
	# define SERVER_HPP

# include "lib.hpp"
# include "channel.hpp"

#define MAX_IRC_MESSAGE 512

class Client;

class Server
{
	public :
		typedef int (Server::*cmdPtr)(std::string, int, Client&);

	private :
		int	_serverFd;
		int	_port;
		std::string	_password;
		std::map<std::string, Channel> _channels;
		std::vector <struct pollfd>	_Fds;
		std::map<int, Client>		_Client;

		std::map<std::string, cmdPtr> _cmds;
		void	initCmds();
		int	cmdHelp(std::string Msg, int index, Client &client);
		int	cmdPing(std::string Msg, int index, Client &client);
		int	cmdPong(std::string Msg, int index, Client &client);
		int	cmdJoin(std::string Msg, int index, Client &client);
		int	cmdNames(std::string Msg, int index, Client &client);
		int	cmdList(std::string Msg, int index, Client &client);
		int	cmdMode(std::string Msg, int index, Client &client);
		int	cmdTopic(std::string Msg, int index, Client &client);
		int	cmdPrivmsg(std::string Msg, int index, Client &client);
		int	cmdPart(std::string Msg, int index, Client &client);
		int	cmdQuit(std::string Msg, int index, Client &client);
		int	cmdKick(std::string Msg, int index, Client &client);
		int	cmdInvite(std::string Msg, int index, Client &client);

	public :
		Server();
		Server(std::string port, std::string password);
		Server(const Server &other);
		Server& operator=(const Server &other);
		~Server();

		
		void	SetServerFd(int serverFd);
		void	AddSocketFds(pollfd fd);
		const struct sockaddr*	setSocketAdress();
		void	SetUpSignals();
		
		int	GetServerFd() const;
		int	GetPort() const;
		std::string	GetPassword() const;
		std::vector<struct pollfd> GetFdsContainer() const;
		struct pollfd GetFds(int index) const;
		int		SizeList();
		struct pollfd&	operator[](size_t index);

		int		setSocket(Server *server);
		int		nonBlocking(int fd);
		int		bindFt();
		std::string intToString(int num);
		int		pollLoop();
		int		acceptFd(int index);
		void	disconnectClient(int nbrClient);
		int		clientJoiningServer(int index);
		int		clientquittingServer(int index, char* buffer);
		int		clientSendingMessage(int index, char* buffer, size_t bytesSize);
		void	closeFds();

		int	cmdHandler(std::string Msg, int index, Client &client);
		std::string	getCmdFromMsg(std::string Msg);
		// std::string enforceMessageLimit(const std::string& msg);
};
std::ostream& operator<<(std::ostream &out, const Server &other);

# endif