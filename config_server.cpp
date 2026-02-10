#include "config_server.hpp"

ConfigServer::ConfigServer() : _port(""), _password("") {}

ConfigServer::ConfigServer(std::string port, std::string password)
	: _port(port), _password(password) {}

ConfigServer::ConfigServer(const ConfigServer &other)
	: _port(other._port), _password(other._password) {}

ConfigServer& ConfigServer::operator=(const ConfigServer &other)
{
	if (this != &other)
	{
		this->_port = other._port;
		this->_password = other._password;
	}
	return (*this);
}

ConfigServer::~ConfigServer() {}

void	ConfigServer::SetPort(std::string port)
{
	this->_port = port;
}

std::string	ConfigServer::GetPort() const
{
	return (this->_port);
}

void	ConfigServer::SetPassword(std::string password)
{
	this->_password = password;
}

std::string	ConfigServer::GetPassword() const
{
	return (this->_password);
}

static int check_port(const std::string& port)
{
    for (size_t i = 0; i < port.length(); ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(port[i])))
            return (EXIT_FAILURE);
    }
    int port_num = std::atoi(port.c_str());
    if (port_num < 1 || port_num > 65535)
        return (EXIT_FAILURE);
    return (EXIT_SUCCESS);
}

int ConfigServer::check_server(const std::string& port, const std::string& password) const
{
    if (check_port(port) == EXIT_FAILURE)
    {
        std::cerr << RED << "Error: Invalid port number" << RESET << std::endl;
        return (EXIT_FAILURE);
    }

    std::cout << "Print password: " << password << std::endl;

    if (password.empty())
    {
        std::cerr << RED << "Error: Password cannot be empty" << RESET << std::endl;
        return (EXIT_FAILURE);
    }

    return (EXIT_SUCCESS);
}