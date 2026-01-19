#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <mutex>
#include "../include/ConnectionHandler.h"
#include "../include/StompFrame.h"
#include "../include/event.h"
#include "../include/SocketReader.h"

SocketReader::SocketReader(ConnectionHandler *handler,std::string username, volatile bool *shouldTerminate,
                           std::map<int, std::string> &receipts, std::map<std::string, int> &subs,
                           std::map<std::string, std::map<std::string, std::vector<Event>>> &reports, std::mutex *mutex)
    : handler(handler),username(username), shouldTerminate(shouldTerminate),
      receiptToMessage(receipts), channelToSubId(subs),
      gameReports(reports), reportMutex(mutex) {}

void SocketReader::operator()()
{
    while (!(*shouldTerminate))
    {
        std::string answer;
        if (!handler->getFrame(answer))
        {
            std::cout << "Disconnected from server." << std::endl;
            *shouldTerminate = true;
            break;
        }

        StompFrame response = StompFrame::fromString(answer);
        if (response.command == "ERROR")
            std::cout << response.body << std::endl;
        if (response.command == "CONNECTED")
            std::cout << "Login successful" << std::endl;
        if (response.command == "RECEIPT")
        {
            int receiptId = std::stoi(response.headers["receipt-id"]);
            if (receiptToMessage.count(receiptId))
            {
                std::cout << receiptToMessage[receiptId] << std::endl;
                receiptToMessage.erase(receiptId);
                if (receiptId == 0)
                {
                    handler->close();
                    *shouldTerminate = true;
                }
            }
        }
        if (response.command == "MESSAGE")
        {
            Event event = Event::parseEventFrame(response);
            std::string gameName = event.get_game_name();
            std::string sender = "";
            if (response.headers.count("user"))
            {
                sender = response.headers["user"];
            }
            if (!sender.empty())
            {
                if (sender == username){continue;}
                {
                    std::lock_guard<std::mutex> lock(*reportMutex);
                    gameReports[gameName][sender].push_back(event);
                }
                std::cout << "Received event: " << event.get_name() << std::endl;
                std::cout << "Game: " << gameName << std::endl;
                std::cout << "Sender: " << sender << std::endl;
                std::cout << "Time: " << event.get_time() << std::endl;
                std::cout << "Description: " << event.get_discription() << std::endl;
            }
        }
    }
}
