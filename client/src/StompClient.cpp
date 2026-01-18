#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include "../include/ConnectionHandler.h"
#include "../include/StompFrame.h"

// This class will handle the socket reading in a separate thread
class SocketReader
{
private:
    ConnectionHandler *handler;
    volatile bool *shouldTerminate;
    std::map<int, std::string> &receiptToMessage;
    std::map<std::string, int> &channelToSubId;

public:
    SocketReader(ConnectionHandler *handler, volatile bool *shouldTerminate,
                 std::map<int, std::string> &receipts, std::map<std::string, int> &subs)
        : handler(handler), shouldTerminate(shouldTerminate),
          receiptToMessage(receipts), channelToSubId(subs) {}

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
                int rId = std::stoi(response.headers["receipt-id"]);
                if (receiptToMessage.count(rId))
                {
                    std::cout << receiptToMessage[rId] << std::endl;
                    receiptToMessage.erase(rId);
                }
            }
            if (response.command == "")
            {
                /* code */
            }
        }
    }
};

int main(int argc, char *argv[])
{
    ConnectionHandler *connectionHandler = nullptr;
    std::thread *socketThread = nullptr;
    volatile bool shouldTerminate = false;
    bool isConnected = false;
    std::map<std::string, int> channelToSubId;
    std::map<int, std::string> receiptToMessage;
    int counterSubId = 1;
    int counterRecipt = 1;

    while (!shouldTerminate)
    {
        const short bufsize = 1024;
        char buf[bufsize];
        std::cin.getline(buf, bufsize);
        std::string line(buf);

        // Split input string to words
        std::stringstream ss(line);
        std::string command;
        ss >> command;
        if (command == "login")
        {
            if (isConnected)
            {
                std::cout << "The client is already logged in, log out before trying again" << std::endl;
                continue;
            }

            std::string hostPort, username, password;
            ss >> hostPort >> username >> password;

            size_t colonPos = hostPort.find(':');
            std::string host = hostPort.substr(0, colonPos);
            short port = (short)stoi(hostPort.substr(colonPos + 1));

            connectionHandler = new ConnectionHandler(host, port);
            if (!connectionHandler->connect())
            {
                std::cout << "Could not connect to server" << std::endl;
                delete connectionHandler;
                connectionHandler = nullptr;
                continue;
            }
            isConnected = true;
            // Start the reading thread
            SocketReader reader(connectionHandler, &shouldTerminate,
                                receiptToMessage, channelToSubId);
            socketThread = new std::thread(reader);

            // Construct the CONNECT frame exactly as required
            StompFrame frame("CONNECT");
            frame.addHeader("accept-version", "1.2");
            frame.addHeader("host", "stomp.cs.bgu.ac.il");
            frame.addHeader("login", username);
            frame.addHeader("passcode", password);

            // Send the frame (sendFrame adds the null character \0 automatically)
            if (!connectionHandler->sendFrame(frame.toString()))
            {
                std::cout << "Failed to send CONNECT frame" << std::endl;
                isConnected = false;
            }
        }
        else if (command == "logout")
        {
            if (!isConnected)
            {
                std::cout << "Not connected." << std::endl;
                continue;
            }
            int receiptId = counterRecipt;
            receiptToMessage[receiptId] = "Logout successful";
            StompFrame frame("DISCONNECT");
            frame.addHeader("receipt", std::to_string(receiptId));
            counterRecipt++;
            if (!connectionHandler->sendFrame(frame.toString()))
            {
                std::cout << "Network Error: Could not send DISCONNECT frame" << std::endl;
                isConnected = false;
            }
            else
            {
                isConnected = false;
            }
        }
        else if (command == "exit")
        {
            std::string channelName;
            if (!isConnected)
            {
                std::cout << "Please login first" << std::endl;
                continue;
            }
            ss >> channelName;
            if (channelToSubId.find(channelName) == channelToSubId.end())
            {
                continue;
            }
            int subId = channelToSubId[channelName];
            int receiptId = counterRecipt;
            receiptToMessage[receiptId] = "Exited channel " + channelName;
            StompFrame frame("UNSUBSCRIBE");
            frame.addHeader("id", std::to_string(subId));
            frame.addHeader("receipt", std::to_string(receiptId));
            counterRecipt++;
            if (!connectionHandler->sendFrame(frame.toString()))
            {
                std::cout << "Network Error: Could not send UNSUBSCRIBE frame to server" << std::endl;
                isConnected = false;
            }
            else
            {
                channelToSubId.erase(channelName);
            }
        }
        else if (command == "join")
        {
            std::string channelName;
            if (!isConnected)
            {
                std::cout << "Please login first" << std::endl;
                continue;
            }
            ss >> channelName;
            channelToSubId[channelName] = counterSubId;
            receiptToMessage[counterRecipt] = "Joined channel " + channelName;
            StompFrame frame("SUBSCRIBE");
            frame.addHeader("destination", "/" + channelName);
            frame.addHeader("id", std::to_string(counterSubId));
            frame.addHeader("receipt", std::to_string(counterRecipt));
            counterSubId++;
            counterRecipt++;
            if (!connectionHandler->sendFrame(frame.toString()))
            {
                std::cout << "Network Error: Could not send SUBSCRIBE frame to server" << std::endl;
                isConnected = false;
            }
        }
        else if (command == "report")
        {
            if (!isConnected)
            {
                std::cout << "Please login first" << std::endl;
                continue;
            }
            std::string filePath;
            ss >> filePath;
            msgData parsedData;
            try
            {
                parsedData = parseEventsFile(filePath);
            }
            catch (const std::exception &e)
            {
                std::cout << "Error parsing file: " << e.what() << std::endl;
                continue;
            }
            std::string channelName = parsedData.team_a_name + "_" + parsedData.team_b_name;
            if (channelToSubId.find(channelName) == channelToSubId.end())
            {
                continue;
            }
            for (const auto &event : parsedData.events)
            {
                StompFrame frame("SEND");
                frame.addHeader("destination", "/" + channelName);
                std::string body = "user: " + username + "\n";
                body += "team a: " + parsedData.team_a_name + "\n";
                body += "team b: " + parsedData.team_b_name + "\n";
                body += "event name: " + event.get_name() + "\n";
                body += "time: " + std::to_string(event.get_time()) + "\n";
                body += "general game updates:\n";
                for (const auto &pair : event.get_game_updates())
                {
                    body += pair.first + ": " + pair.second + "\n";
                }
                body += "team a updates:\n";
                for (const auto &pair : event.get_team_a_updates())
                {
                    body += pair.first + ": " + pair.second + "\n"
                }
                body += "team b updates:\n";
                for (const auto &pair : event.get_team_b_updates())
                {
                    body += pair.first + ": " + pair.second + "\n"
                }
                body += "description:\n" + event.get_description() + "\n";
                frame.setBody(body);
                if (!connectionHandler->sendFrame(frame.toString()))
                {
                    std::cout << "Network Error: Could not send event" << std::endl;
                    isConnected = false;
                    break;
                }
            }
        }
        else if (command == "summary")
        {
        }

        // Handle other commands (join, exit, report, summary)...
    }

    // Cleanup
    if (socketThread)
    {
        socketThread->join(); // Wait for thread to finish
        delete socketThread;
    }
    if (connectionHandler)
    {
        delete connectionHandler;
    }

    return 0;
}
