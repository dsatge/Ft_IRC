#pragma once

class ConfigServer
{
    private :
        std::string	_port;
        std::string	_password;
    public :
        ConfigServer();
        ConfigServer(std::string port, std::string password);
        ConfigServer(const ConfigServer &other);
        ConfigServer& operator=(const ConfigServer &other);
        ~ConfigServer();

        void	SetPort(std::string port);
        void	SetPassword(std::string password);
        std::string	GetPort() const;
        std::string	GetPassword() const;

        int check_server(const std::string& port, const std::string& password) const;
};