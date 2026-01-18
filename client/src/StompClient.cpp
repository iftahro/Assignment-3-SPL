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
            // Implementation for logout will come later...
            // For now, simple exit logic or just sending the frame if needed
            if (isConnected)
            {
                // Logic for sending DISCONNECT frame will be here
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
            if (channelToSubId.find(channelName) == channelToSubId.end()){continue;}
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
