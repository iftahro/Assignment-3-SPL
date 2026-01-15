#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include "../include/ConnectionHandler.h"
#include "../include/StompFrame.h"

// This class will handle the socket reading in a separate thread
class SocketReader {
private:
    ConnectionHandler* handler;
    volatile bool* shouldTerminate;

public:
    SocketReader(ConnectionHandler* handler, volatile bool* shouldTerminate) 
        : handler(handler), shouldTerminate(shouldTerminate) {}

    void operator()() {
        while (!(*shouldTerminate)) {
            std::string answer;
            if (!handler->getFrame(answer)) {
                std::cout << "Disconnected from server." << std::endl;
                *shouldTerminate = true;
                break;
            }

            // Print the received message from the server
            // todo (In later stages, you will parse this message)
			StompFrame response = StompFrame::fromString(answer);
			if (response.command == "ERROR") std::cout << response.body << std::endl;
            if (response.command == "CONNECTED") std::cout << "Login successful" << std::endl;
        }
    }
};

int main(int argc, char *argv[]) {
    ConnectionHandler* connectionHandler = nullptr;
    std::thread* socketThread = nullptr;
    volatile bool shouldTerminate = false;
    bool isConnected = false;

    while (!shouldTerminate) {
        const short bufsize = 1024;
        char buf[bufsize];
        std::cin.getline(buf, bufsize);
        std::string line(buf);
        
        // Split input string to words
        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (command == "login") {
            if (isConnected) {
                std::cout << "The client is already logged in, log out before trying again" << std::endl;
                continue;
            }

            std::string hostPort, username, password;
            ss >> hostPort >> username >> password;

            size_t colonPos = hostPort.find(':');
            std::string host = hostPort.substr(0, colonPos);
            short port = (short)stoi(hostPort.substr(colonPos + 1));

            connectionHandler = new ConnectionHandler(host, port);
            if (!connectionHandler->connect()) {
                std::cout << "Could not connect to server" << std::endl;
                delete connectionHandler;
                connectionHandler = nullptr;
                continue;
            }
            isConnected = true;
            // Start the reading thread
            SocketReader reader(connectionHandler, &shouldTerminate);
            socketThread = new std::thread(reader);

            // Construct the CONNECT frame exactly as required
            StompFrame frame("CONNECT");
            frame.addHeader("accept-version", "1.2");
            frame.addHeader("host", "stomp.cs.bgu.ac.il");
            frame.addHeader("login", username);
            frame.addHeader("passcode", password);

            // Send the frame (sendFrame adds the null character \0 automatically)
            if (!connectionHandler->sendFrame(frame.toString())) {
                std::cout << "Failed to send CONNECT frame" << std::endl;
                isConnected = false;
            }
            
        } else if (command == "logout") {
            // Implementation for logout will come later...
            // For now, simple exit logic or just sending the frame if needed
            if (isConnected) {
                 // Logic for sending DISCONNECT frame will be here
            }
        } 
        
        // Handle other commands (join, exit, report, summary)...
    }

    // Cleanup
    if (socketThread) {
        socketThread->join(); // Wait for thread to finish
        delete socketThread;
    }
    if (connectionHandler) {
        delete connectionHandler;
    }

    return 0;
}
