#pragma once

#include "../include/ConnectionHandler.h"
#include "../include/StompFrame.h"
#include "../include/event.h"
#include <map>
#include <vector>
#include <string>
#include <utility> 

class SocketReader
{
private:
    ConnectionHandler *handler;
    std::string username;
    volatile bool *shouldTerminate;
    std::map<int, std::string> &receiptToMessage;
    std::map<std::string, int> &channelToSubId;
    std::mutex* reportMutex;
    
    std::map<std::string, std::map<std::string, std::vector<Event>>> &gameReports;

public:
    SocketReader(ConnectionHandler *handler, std::string username,
                 volatile bool *shouldTerminate,
                 std::map<int, std::string> &receipts, 
                 std::map<std::string, int> &subs,
                 std::map<std::string, std::map<std::string, std::vector<Event>>> &reports
                ,std::mutex* mutex);

    void operator()();

};
