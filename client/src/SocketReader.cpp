#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <mutex>
#include "../include/ConnectionHandler.h"
#include "../include/StompFrame.h"
#include "../include/event.h"

class SocketReader
{
private:
    ConnectionHandler *handler;
    volatile bool *shouldTerminate;
    std::map<int, std::string> &receiptToMessage;
    std::map<std::string, int> &channelToSubId;
    std::map<std::string, std::map<std::string, std::vector<Event>>> &gameReports;
    std::mutex* reportMutex;

public:
    SocketReader(ConnectionHandler *handler, volatile bool *shouldTerminate,
                 std::map<int, std::string> &receipts, std::map<std::string, int> &subs,
                 std::map<std::string, std::map<std::string, std::vector<Event>>> &reports
                ,std::mutex* mutex)
        : handler(handler), shouldTerminate(shouldTerminate),
          receiptToMessage(receipts), channelToSubId(subs),
          gameReports(reports), reportMutex(mutex) {}

    void operator()()
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

            // Print the received message from the server
            // todo (In later stages, you will parse this message)
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
                std::pair<std::string, Event> parsed = Event::parseEventFrame(response);
                std::string gameName = parsed.first;
                Event event = parsed.second;
                std::string sender = "";
                if (response.headers.count("user")){sender = response.headers["user"];}
                if (!sender.empty())
                {
                    {std::lock_guard<std::mutex> lock(*reportMutex);
                    gameReports[gameName][sender].push_back(event);}
                    std::cout << "Received event: " << event.get_name() << std::endl;
                    std::cout << "Game: " << gameName << std::endl;
                    std::cout << "Sender: " << sender << std::endl;
                    std::cout << "Time: " << event.get_time() << std::endl;
                    std::cout << "Description: " << event.get_description() << std::endl;
                }
            }
        }
    }
};