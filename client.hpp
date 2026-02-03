# ifndef CLIENT_HPP
	# define CLIENT_HPP

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

class Client
{
	private:
		int	_fd;
		bool	_toErase;
		std::string _IP;
		char*		_Buffer;
	public:
		Client(int fd);
		Client(const Client &other);
		Client& operator=(const Client &othe);
		~Client();

		void SetFd(int fd);
		void SetIP(std::string IP);
		void SetBuffer(char *buffer);
		void SetErase();

		std::string GetBuffer() const;
		bool		GetErase() const;
		int			GetFd() const;
};

#endif