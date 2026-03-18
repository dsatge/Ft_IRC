# ifndef SERVER_HPP
	# define SERVER_HPP

# include "lib.hpp"
# include "channel.hpp"

#define VERSION "1.0"
#define USER_MODES "o"
#define CHANNEL_MODES "iklt"
#define SERVER_NAME "ircserv"

#define MAX_IRC_MESSAGE 512
#define ERR_NOSUCHNICK 401            // No such nick
#define ERR_NOSUCHSERVER 402
#define ERR_NOORIGIN 409              // No origin specified
#define ERR_NORECIPIENT 411
#define ERR_NOTEXTTOSEND 412          // No text to send
#define ERR_UNKNOWNCOMMAND 421        // Unknown command
#define ERR_NONICKNAMEGIVEN 431       // No nickname given
#define ERR_ERRONEUSNICKNAME 432      // Erroneous nickname
#define ERR_NICKNAMEINUSE 433         // Nickname is already in use
#define ERR_NOTREGISTERED 451         // You have not registered
#define ERR_NEEDMOREPARAMS 461        // Not enough parameters
#define ERR_ALREADYREGISTRED 462      // Unauthorized command (already registered)
#define ERR_PASSWDMISMATCH 464        // Password incorrect

/* --- Erreurs liées aux Channels --- */
#define RPL_NOTOPIC 331               // No topic is set
#define ERR_NOSUCHCHANNEL 403         // No such channel
#define ERR_CANNOTSENDTOCHAN 404      // Cannot send to channel
#define ERR_USERNOTINCHANNEL 441      // They are not on that channel
#define ERR_NOTONCHANNEL 442          // You're not on that channel
#define ERR_USERONCHANNEL 443         // is already on channel
#define ERR_CHANNELISFULL 471         // Cannot join channel (+l)
#define ERR_UNKNOWNMODE 472           // is unknown mode char to me
#define ERR_INVITEONLYCHAN 473        // Cannot join channel (+i)
#define ERR_BADCHANNELKEY 475         // Cannot join channel (+k)
#define ERR_CHANOPRIVSNEEDED 482      // You're not channel operator
class Client;

class Server
{
	public :
		// typedef int (Server::*cmdPtr)(std::string, int, Client&);
		typedef int (Server::*cmdPtr)(std::vector<std::string>, int, Client&);
		typedef int (Server::*cmdPtrMode)(int, bool, std::string, std::string, std::string);

	private :
		int								_serverFd;
		int								_port;
		std::string						_date;
		std::string						_password;
		std::map<std::string, Channel>	_channels;
		std::vector <struct pollfd>		_Fds;
		std::map<int, Client>			_Client;
		std::map<int, std::string>		_ErrorMsg;
		std::map<char, cmdPtrMode>		_Modes;
		std::map<std::string, cmdPtr>	_cmds;
		/// Errors Messages
		void	initErrorMsg();
		void	sendErroMsg(int errorCode, int index, std::string target);
		void	sendErroMsgKEY(int errorCode, int index, std::string target, std::string keyword);
		void	sendErroMsgCHANNEL(int errorCode, int index, std::string target, std::string channel);
		void	sendErroMsgCHANNEL_KEY(int errorCode, int index, std::string target, std::string channel, std::string key);
		void	sendInfoMsg(int infoCode, int index, std::string target);	
		void	sendInfoMsgCHANNEL(int infoCode, int index, std::string target, std::string channel);

		/// Server Messages
		void	sendWelcomeMsg(int index, std::string target);
		/// Features Authentification
		void	initCmdsAuthentification();
		int 	cmdAuthentificationHandler(std::vector<std::string> argList, int index, Client &client);
		int		authentificateClientPASS(std::vector<std::string> argList, int index, Client &client);
		int 	authentificateClientNICK(std::vector<std::string> argList, int index, Client &client);
		int 	authentificateClientUSER(std::vector<std::string> argList, int index, Client &client);
		/// Features commands
		void	initCmds();
		int	cmdHelp(std::vector<std::string> argList, int index, Client &client);
		int	cmdPing(std::vector<std::string> argList, int index, Client &client);
		int	cmdPong(std::vector<std::string> argList, int index, Client &client);
		int	cmdJoin(std::vector<std::string> argList, int index, Client &client);
		int	cmdNames(std::vector<std::string> argList, int index, Client &client);
		int	cmdList(std::vector<std::string> argList, int index, Client &client);
		int	cmdMode(std::vector<std::string> argList, int index, Client &client);
		int	cmdTopic(std::vector<std::string> argList, int index, Client &client);
		int	cmdPrivmsg(std::vector<std::string> argList, int index, Client &client);
		int	cmdPart(std::vector<std::string> argList, int index, Client &client);
		int	cmdQuit(std::vector<std::string> argList, int index, Client &client);
		int	cmdKick(std::vector<std::string> argList, int index, Client &client);
		int	cmdInvite(std::vector<std::string> argList, int index, Client &client);
		/// Features modes cmds
		void	initMode();
		bool modeNeedsParam(char mode);
		int modeHandler(char mode, int index, bool adding, std::string user, std::string channelName, std::string param);
		int	modeI(int index, bool adding, std::string user, std::string channelName, std::string empty);
		int	modeT(int index, bool adding, std::string user, std::string channelName, std::string empty);
		int	modeK(int index, bool adding, std::string user, std::string channelName, std::string password);
		int	modeO(int index, bool adding, std::string user, std::string channelName, std::string target);
		int	modeL(int index, bool adding, std::string user, std::string channelName, std::string limit);
	public :
		Server();
		Server(std::string port, std::string password);
		Server(const Server &other);
		Server& operator=(const Server &other);
		~Server();

		std::string	dateSetUp();
		void	SetServerFd(int serverFd);
		void	AddSocketFds(pollfd fd);
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

		int	cmdHandler(std::vector<std::string> argList, int index, Client &client);
		bool hasColon(std::string Msg);
		std::string	removesColon(std::string param);
		std::string cleanChannelName(std::string channelName);
		int	splitArgs(std::vector<std::string> &arglist, std::string Msg);
		std::string	getCmdFromMsg(std::string Msg);
		// std::string enforceMessageLimit(const std::string& msg);
};
std::ostream& operator<<(std::ostream &out, const Server &other);

# endif