#pragma once

#include <string>
#include <cctype>
#include <cstdlib>
#include <iostream>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"

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