#include "Server.hpp"
#include <sstream>

Server::Server(int port, const std::string& password) : port(port), _password(password), Server_fd(-1)
{
    std::cout<< "intialized the attributes"<<std::endl;
}

std::map<std::string, Channel>& Server::getChannels()
{
    return serv_channel;
}

Server::~Server()
{
    if(Server_fd >= 0)
        close(Server_fd);
    std::cout<<"Destructor is called \n";
}

void Server::CreateServer()
{
    CreateSocket();
    BindSocket();
    StartListen();
    CreatEpoll();
    run();
}

void Server::CreateSocket()
{
    Server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(Server_fd < 0)
        throw std::runtime_error("Socket failed: ");
    HandelNonBlocking(Server_fd);
    int option_value = 1;
    if(setsockopt(Server_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, 
        sizeof(option_value)) < 0) //to release the port once you close the server
        throw std::runtime_error("SetSockopt failed ~!");

    std::cout<<"socket is created ;\n";
}

void Server::BindSocket()
{
    struct sockaddr_in ServAddress;
    std::memset(&ServAddress, 0, sizeof(ServAddress));
    ServAddress.sin_family = AF_INET;
    ServAddress.sin_addr.s_addr = INADDR_ANY;
    ServAddress.sin_port = htons(port);
    std::cout << (sockaddr*)&ServAddress << "______"<<std::endl;
    std::cout << "port = " << port << std::endl;
std::cout << "network port = " << ntohs(ServAddress.sin_port) << std::endl;
    if(bind(Server_fd, (sockaddr*)&ServAddress, sizeof(ServAddress)) < 0)
    {
        close(Server_fd);
        std::cerr << "bind failed: " << std::strerror(errno) << std::endl;
        throw std::runtime_error("bind failed: ");
    }
    std::cout <<"bind is successfuly\n";
}

void Server::StartListen()
{
    if (listen(Server_fd, 128) < 0)
    {
        close(Server_fd);
        throw std::runtime_error("No listning: ");
    }
}

const std::string& Server::GetPassword() const
{
    return _password;
}

std::map<int, Client> &Server::get_clients_map(void)
{
	return (clients_map);
}

std::vector<std::string> Server::parsing_handler(std::string buffer)
{
    std::vector<std::string> Message;
    bool has_trailing = false;
    std::string str_buffer(buffer);
    size_t pos;
    std::string left;
    std::string right;
    std::string line;
    std::stringstream tokens;
    if (!str_buffer.empty() && str_buffer[0] == ':')
    {
        pos = str_buffer.find(' ');
        if (pos != std::string::npos)
            str_buffer = str_buffer.substr(pos + 1);
    }
    size_t pos_two = str_buffer.find(" :");
    if (pos_two != std::string::npos)
    {
        right = str_buffer.substr(pos_two + 2);
        str_buffer = str_buffer.substr(0,pos_two);
        has_trailing = true;
    }
    std::stringstream tmp(str_buffer);
    std::string word;
    while (tmp >> word)
        Message.push_back(word);


    if (has_trailing)
    {
        while (!right.empty() && (right[right.size()-1] == '\r' || right[right.size()-1] == '\n'))
            right.erase(right.size() - 1);
        Message.push_back(right);
    }

    return Message;
}

void Server::command_handeler(int fd, std::vector<std::string> Message)
{
    if (Message.empty())
        return;
    if(Message[0] == "PASS")
    {
        pass(fd, Message, *this);
    }
    else if (Message[0] == "NICK")
    {
        nick(fd, Message, *this);
    }
    else if (Message[0] == "USER")
    {
        user(fd, Message, *this);
    }
    else if (Message[0] == "MODE")
    {
        mode(fd, Message, *this);
    }
    else if (Message[0] == "PRIVMSG")
    { 
        privmsg(fd, Message, *this);
    }
    else if (Message[0] == "TOPIC")
    {
        topic(fd, Message, *this);
    }
    else if (Message[0] == "JOIN")
    {
        join(fd, Message, *this);
    }
    else if (Message[0] == "INVITE")
    {
        invite(fd, Message, *this);
    }
    else if (Message[0] == "KICK")
    {
        kick(fd, Message, *this);
    }
    else if (Message[0] == "PING")
    {
        std::string PONG = ":ft_irc PONG ft_irc :" + (Message.size() > 1 ? Message[1] : "ft_irc") + "\r\n";
        send(fd, PONG.c_str(), PONG.size(), 0);
    }
    else
    {
        std::cout << "Command not found : " << Message[0] << std::endl;
    }
}


void Server::HandelClient(int fd)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    while(1)
    {
        int BytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if(BytesReceived > 0)
        {
            client_buffers[fd].append(buffer, BytesReceived);
        }
        else if(BytesReceived == 0)
        {
            std::cout << "here is" << fd << " -=== disconnected\n";
            RemoveClient(fd);
            return ;
        }
        else 
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                break ;
            std::cout<<"recv error fd="<<fd<<std::endl;
            RemoveClient(fd);
            return ;
        }
    }
    ExtractedMessages(fd);
}


void Server::CreatEpoll()
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0)
        throw std::runtime_error("epoll_create1 failed");

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = Server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, Server_fd, &ev) < 0)
        throw std::runtime_error("epoll_ctl ADD server_fd failed");

    std::cout << "Server_fd added to epoll\n";
}

void Server::AddClientes(int fd)
{
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
        throw std::runtime_error("epoll_ctl ADD client failed!");
    std::cout << "Client added — fd=" << fd << "\n";
}

void Server::RemoveClient(int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);

    std::string nick;
    std::map<int, Client>::iterator cit = clients_map.find(fd);
    if (cit != clients_map.end())
        nick = cit->second.getNickname();

    for (std::map<std::string, Channel>::iterator it = serv_channel.begin();
        it != serv_channel.end(); )
    {
        Channel &channel = it->second;
        channel.pop(fd);
        channel.pop_op(fd);
        channel.pop_invite(fd);

        if (!nick.empty())
            channel.get_channel_members().erase(nick);

        if (channel.get_members().empty())
            serv_channel.erase(it++);
        else
            ++it;
    }
    clients_map.erase(fd);
    client_buffers.erase(fd);
    close(fd);
    std::cout << "Client is removed — fd=" << fd << "\n";
}

void Server::run()
{
    struct epoll_event events[MAX_EVENTS];
    std::cout << "IS running with epoll...\n";
    while (1)
    {
        int client_fd;
        int number_fd = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (number_fd < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("epoll_wait failed!");
        }
        for (int i = 0; i < number_fd; i++)
        {
            int tmpfd = events[i].data.fd;
            if (tmpfd == Server_fd)
            {
                while (1)
                {
                    struct sockaddr_in client_addr;
                    socklen_t len = sizeof(client_addr);
                    client_fd = accept(Server_fd, (struct sockaddr*)&client_addr, &len);
                    if(client_fd < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        std::cout << "accept failed!\n";
                        break;
                    }
                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
                    HandelNonBlocking(client_fd);

                    clients_map[client_fd] = Client(client_fd);
                    clients_map[client_fd].set_host(ip);
                    AddClientes(client_fd);
                    std::cout << "New client fd = " << client_fd << std::endl;
                }
            }
            else
            {
                HandelClient(tmpfd);
            }
        }
    }
}

void Server::HandelNonBlocking(int fd)
{
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("Fcntl F_SETFL failes !");
}

void Server::ExtractedMessages(int fd)
{
    size_t pos;
    while ((pos = client_buffers[fd].find("\n")) != std::string::npos)
    {
        std::string line = client_buffers[fd].substr(0, pos);
        client_buffers[fd].erase(0, pos + 1);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            continue;

        std::cout << "CMD: " << line << std::endl;
        std::vector<std::string> Mesg = parsing_handler(line);
        command_handeler(fd, Mesg);
    }
}

Client& Server::GetClient(int fd)
{
    std::map<int, Client>::iterator it = clients_map.find(fd);
    if(it == clients_map.end())
        throw std::runtime_error("Client not found");
    return it->second;
}