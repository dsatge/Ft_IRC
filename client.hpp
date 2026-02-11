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
		void SetChannelName(const std::string &channel);

		std::string GetBuffer() const;
		std::string GetMsg() const;
		bool		GetErase() const;
		bool		GetAuthenticated() const;
		std::string GetNickname() const;
		std::string GetChannelName() const;
		int			GetFd() const;
};

#endif