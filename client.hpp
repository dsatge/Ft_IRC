# ifndef CLIENT_HPP
	# define CLIENT_HPP

#include "lib.hpp"

class Client
{
	private:
		int	_fd;
		bool	_toErase;
		bool	_authenticated;
		std::string _nickname;
		std::string _username;
		std::string _realname;
		std::string _channelName;
		std::string _IP;
		char*		_Buffer;
		std::string _Msg;
	public:
		Client(int fd);
		Client(const Client &other);
		Client& operator=(const Client &othe);
		Client& operator+=(const Client &othe);
		~Client();

		void SetFd(int fd);
		void SetIP(std::string IP);
		void SetBuffer(char *buffer);
		void SetMsg(char *buffer, size_t bytesSize);
		void SetEraseMsg(int posInit, int posEnd);
		void SetErase();
		void SetAuthenticated(bool value);
		void SetNickname(const std::string &nickname);
		void SetUsername(const std::string &username);
		void SetRealname(const std::string &realname);
		void SetChannelName(const std::string &channel);

		std::string GetBuffer() const;
		std::string GetMsg() const;
		bool		GetErase() const;
		bool		GetAuthenticated() const;
		std::string GetNickname() const;
		std::string GetUsername() const;
		std::string GetRealname() const;
		std::string GetChannelName() const;
		std::string GetIP() const;
		int			GetFd() const;
};

#endif