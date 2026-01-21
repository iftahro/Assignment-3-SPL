#pragma once
#include "../include/ConnectionHandler.h"
#include "../include/event.h"
#include "../include/StompFrame.h"
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>

class StompProtocol
{
private:
    ConnectionHandler *handler;
    std::string username;
    volatile bool *shouldTerminate;
    std::map<int, std::string> &receiptToMessage;
    std::map<std::string, int> &channelToSubId;
    std::map<std::string, std::map<std::string, std::vector<Event>>> &gameReports;
    std::mutex *reportMutex;
    std::atomic<bool> *isLoggedIn;

public:
    StompProtocol(ConnectionHandler *handler, std::string username, volatile bool *shouldTerminate,
                  std::map<int, std::string> &receipts, std::map<std::string, int> &subs,
                  std::map<std::string, std::map<std::string, std::vector<Event>>> &reports, 
                  std::mutex *mutex, std::atomic<bool> *isLoggedIn);

    void process(StompFrame &frame);
};
