#include "Server.hpp"
#include <sys/socket.h>
#include <map>
#include <cctype>

void privmsg(int fd, std::vector<std::string> &s, Server &serv)
{
    std::map<int, Client> &clients_map = serv.get_clients_map();

    if (!clients_map[fd].IsRegistered())
    {
        std::string err = ":ft_irc 451 * :You have not registered\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;
    }

    if (s.size() < 2)
    {
        std::string err = ":ft_irc 411 " + clients_map[fd].getNickname() 
                        + " :No recipient given (PRIVMSG)\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;
    }

    if (s.size() < 3 || s[2].empty()) 
    {
        std::string err = ":ft_irc 412 " + clients_map[fd].getNickname() 
                        + " :No text to send\r\n";
        send(fd, err.c_str(), err.size(), 0);
        return;
    }

    std::vector<std::string> targets;
    std::string tmp;
    for (size_t i = 0; i < s[1].size(); i++)
    {
        if (s[1][i] == ',')
        {
            if (!tmp.empty())
                targets.push_back(tmp);
            tmp = "";
        }
        else
            tmp += s[1][i];
    }
    if (!tmp.empty())
        targets.push_back(tmp);

    for (size_t t = 0; t < targets.size(); t++)
    {
        std::string target  = targets[t];
        std::string ltarget = target;
        for (size_t i = 0; i < ltarget.size(); i++)
            ltarget[i] = std::tolower(ltarget[i]);

        if (target[0] == '#')
        {
            std::map<std::string, Channel> &serv_channel = serv.getChannels();

            if (serv_channel.find(ltarget) == serv_channel.end())
            {
                std::string err = ":ft_irc 403 " + clients_map[fd].getNickname() 
                                + " " + target + " :No such channel\r\n";
                send(fd, err.c_str(), err.size(), 0);
                continue;
            }
            Channel &ch = serv_channel.find(ltarget)->second;

            if (!ch.check_member(fd))
            {
                std::string err = ":ft_irc 404 " + clients_map[fd].getNickname() 
                                + " " + target + " :Cannot send to channel\r\n";
                send(fd, err.c_str(), err.size(), 0);
                continue;
            }

            std::string send_M = ":" + clients_map[fd].getNickname()
                      + "!" + clients_map[fd].getUsername()
                      + "@ft_irc PRIVMSG " + target + " :" + s[2] + "\r\n";

            const std::vector<int> &m = ch.get_members();
            for (std::vector<int>::const_iterator it = m.begin(); it != m.end(); ++it)
            {
                if (*it != fd)
                    send(*it, send_M.c_str(), send_M.size(), 0);
            }
        }
        else
        {
            Client *tclient = NULL;
            for (std::map<int, Client>::iterator it = clients_map.begin(); 
                 it != clients_map.end(); ++it)
            {
                std::string nick = it->second.getNickname();
                for (size_t i = 0; i < nick.size(); i++)
                    nick[i] = std::tolower(nick[i]);
                if (nick == ltarget && it->second.IsRegistered())
                {
                    tclient = &it->second;
                    break;
                }
            }

            if (tclient == NULL)
            {
                std::string err = ":ft_irc 401 " + clients_map[fd].getNickname() 
                                + " " + target + " :No such nick/channel\r\n";
                send(fd, err.c_str(), err.size(), 0);
                continue;
            }

            std::string send_M = ":" + clients_map[fd].getNickname()
                      + "!" + clients_map[fd].getUsername()
                      + "@ft_irc PRIVMSG " + target + " :" + s[2] + "\r\n";
            send(tclient->getFd(), send_M.c_str(), send_M.size(), 0);
        }
    }
}