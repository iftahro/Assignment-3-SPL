#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include "../include/ConnectionHandler.h"
#include "../include/StompFrame.h"

class SocketReader
{
private:
    ConnectionHandler *handler;
    volatile bool *shouldTerminate;
    std::map<int, std::string> &receiptToMessage;
    std::map<std::string, int> &channelToSubId;
    std::map<std::string, std::map<std::string, std::vector<Event>>> &gameReports;

    std::pair<std::string, Event> parseEventFrame(const StompFrame &response)
    {
        std::string gameName = response.headers.at("destination");
        if (gameName.length() > 0 && gameName[0] == '/')
            gameName = gameName.substr(1);

        Event event(gameName);
        std::string user = "";
        std::string body = response.body;
        std::stringstream ss(body);
        std::string line;
        std::string currentSection = "";

        while (std::getline(ss, line))
        {
            if (line == "general game updates:")
            {
                currentSection = "general";
                continue;
            }
            if (line == "team a updates:")
            {
                currentSection = "team_a";
                continue;
            }
            if (line == "team b updates:")
            {
                currentSection = "team_b";
                continue;
            }
            if (line == "description:")
            {
                currentSection = "description";
                continue;
            }

            if (currentSection == "description")
            {
                std::string currentDesc = event.get_description();
                if (!currentDesc.empty())
                    currentDesc += "\n";
                event.set_description(currentDesc + line);
                continue;
            }

            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos)
            {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                if (value.length() > 0 && value[0] == ' ')
                    value = value.substr(1);
                if (currentSection == "")
                {
                    if (key == "user")
                        user = value;
                    else if (key == "event name")
                        event.set_name(value);
                    else if (key == "time")
                        event.set_time(std::stoi(value));
                }
                else if (currentSection == "general")
                    event.get_game_updates()[key] = value;
                else if (currentSection == "team_a")
                    event.get_team_a_updates()[key] = value;
                else if (currentSection == "team_b")
                    event.get_team_b_updates()[key] = value;
            }
        }
        return {user, event};
    }

public:
    SocketReader(ConnectionHandler *handler, volatile bool *shouldTerminate,
                 std::map<int, std::string> &receipts, std::map<std::string, int> &subs,
                 std::map<std::string, std::map<std::string, std::vector<Event>>> &reports)
        : handler(handler), shouldTerminate(shouldTerminate),
          receiptToMessage(receipts), channelToSubId(subs),
          gameReports(reports) {}

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
                    gameReports[gameName][sender].push_back(event);
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