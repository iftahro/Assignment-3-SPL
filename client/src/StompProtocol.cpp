#include "../include/StompProtocol.h"
#include <iostream>

StompProtocol::StompProtocol(ConnectionHandler *handler, std::string username, volatile bool *shouldTerminate,
                             std::map<int, std::string> &receipts, std::map<std::string, int> &subs,
                             std::map<std::string, std::map<std::string, std::vector<Event>>> &reports, 
                             std::mutex *mutex, std::atomic<bool> *isLoggedIn)
    : handler(handler), username(username), shouldTerminate(shouldTerminate),
      receiptToMessage(receipts), channelToSubId(subs),
      gameReports(reports), reportMutex(mutex), isLoggedIn(isLoggedIn)
{}

void StompProtocol::process(StompFrame &response)
{
    if (response.command == "ERROR") {
        std::cout << response.toString() << std::endl;
        *isLoggedIn = false;
        *shouldTerminate = true;
        handler->close();
    }
    else if (response.command == "CONNECTED") {
        std::cout << "Login successful" << std::endl;
        *isLoggedIn = true;
    }
    else if (response.command == "RECEIPT")
    {
        int receiptId = std::stoi(response.headers["receipt-id"]);
        if (receiptToMessage.count(receiptId))
        {
            std::cout << receiptToMessage[receiptId] << std::endl;
            receiptToMessage.erase(receiptId);
            if (receiptId == 0) // Logout receipt
            {
                handler->close();
                *shouldTerminate = true;
            }
        }
    }
    else if (response.command == "MESSAGE")
    {
        Event event = Event::parseEventFrame(response);
        std::string gameName = event.get_game_name();
        std::string sender = "";
        if (response.headers.count("sender"))
        {
            sender = response.headers["sender"];
        }
        if (!sender.empty())
        {
            if (sender != username) {
                std::lock_guard<std::mutex> lock(*reportMutex);
                gameReports[gameName][sender].push_back(event);
            }
        }
    }
}
