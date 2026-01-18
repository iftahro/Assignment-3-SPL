#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include "../include/ConnectionHandler.h"
#include "../include/StompFrame.h"
#include "../include/SocketReader.h"

// This class will handle the socket reading in a separate thread
bool eventComparator(const Event &a, const Event &b)
{
    if (a.get_before_half_time() && !b.get_before_half_time())
        return true;
    if (!a.get_before_half_time() && b.get_before_half_time())
        return false;
    return a.get_time() < b.get_time();
}

void calculateGameStats(const std::vector<Event> &events,
                        std::map<std::string, std::string> &generalStats,
                        std::map<std::string, std::string> &teamAStats,
                        std::map<std::string, std::string> &teamBStats)
{
    for (const auto &event : events)
    {
        for (const auto &pair : event.get_game_updates())
            generalStats[pair.first] = pair.second;

        for (const auto &pair : event.get_team_a_updates())
            teamAStats[pair.first] = pair.second;

        for (const auto &pair : event.get_team_b_updates())
            teamBStats[pair.first] = pair.second;
    }
}

void writeSummaryToFile(const std::string &filePath, const std::vector<Event> &events,
                        const std::map<std::string, std::string> &generalStats,
                        const std::map<std::string, std::string> &teamAStats,
                        const std::map<std::string, std::string> &teamBStats)
{
    std::ofstream outFile(filePath);
    if (!outFile.is_open())
    {
        std::cout << "Error: Could not open file " << filePath << std::endl;
        return;
    }
    std::string teamA = events[0].get_team_a_name();
    std::string teamB = events[0].get_team_b_name();
    outFile << teamA << " vs " << teamB << "\n";
    outFile << "Game stats:\n";
    outFile << "General stats:\n";
    for (const auto &pair : generalStats)
        outFile << pair.first << ": " << pair.second << "\n";
    outFile << teamA << " stats:\n";
    for (const auto &pair : teamAStats)
        outFile << pair.first << ": " << pair.second << "\n";
    outFile << teamB << " stats:\n";
    for (const auto &pair : teamBStats)
        outFile << pair.first << ": " << pair.second << "\n";
    outFile << "Game event reports:\n";
    for (const auto &event : events)
    {
        outFile << event.get_time() << " - " << event.get_name() << ":\n\n";
        outFile << event.get_description() << "\n\n";
    }
    outFile.close();
    std::cout << "Summary created successfully at " << filePath << std::endl;
}

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
    std::map<std::string, std::map<std::string, std::vector<Event>>> gameReports;

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
                                receiptToMessage, channelToSubId, gameReports);
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
            receiptToMessage[0] = "Logout successful";
            StompFrame frame("DISCONNECT");
            frame.addHeader("receipt", "0");
            if (!connectionHandler->sendFrame(frame.toString()))
            {
                std::cout << "Network Error: Could not send DISCONNECT frame" << std::endl;
                isConnected = false;
            }
            else
            {
                socketThread->join();
                delete socketThread;
                socketThread = nullptr;
                delete connectionHandler;
                connectionHandler = nullptr;
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
            if (!isConnected){
                std::cout << "Please login first" << std::endl;
                continue; }
            std::string filePath;
            ss >> filePath;
            names_and_events parsedData; 
            try{
                parsedData = parseEventsFile(filePath);
            }
            catch (const std::exception &e){
                std::cout << "Error parsing file: " << e.what() << std::endl;
                continue;
            }
            std::string channelName = parsedData.team_a_name + "_" + parsedData.team_b_name;
            if (channelToSubId.find(channelName) == channelToSubId.end()){
                std::cout << "Error: User is not subscribed to channel " << channelName << std::endl;
                continue;
            }
            for (Event &event : parsedData.events)
            {
                event.setSender(userName);
                gameReports[channelName][userName].push_back(event);
                StompFrame frame("SEND");
                frame.addHeader("destination", "/" + channelName);

                json j;
                j["event name"] = event.get_name();
                j["time"] = event.get_time();
                j["description"] = event.get_description();
                j["general game updates"] = event.get_game_updates();
                j["team a updates"] = event.get_team_a_updates();
                j["team b updates"] = event.get_team_b_updates();

                j["team a"] = parsedData.team_a_name;
                j["team b"] = parsedData.team_b_name;

                frame.setBody(j.dump());
                connectionHandler->sendFrame(frame.toString());
            }
        }
        else if (command == "summary")
        {
            std::string gameName, userName, filePath;
            ss >> gameName >> userName >> filePath;
            std::vector<Event> eventsCopy;
            if (gameReports.find(gameName) == gameReports.end() ||
                gameReports[gameName].find(userName) == gameReports[gameName].end())
            {
                std::cout << "Error: No reports found for " << gameName << " from user " << userName << std::endl;
                continue;
            }
            eventsCopy = gameReports[gameName][userName];
            if (eventsCopy.empty())
            {
                std::cout << "No events to report." << std::endl;
                continue;
            }
            std::sort(eventsCopy.begin(), eventsCopy.end(), eventComparator);
            std::map<std::string, std::string> generalStats;
            std::map<std::string, std::string> teamAStats;
            std::map<std::string, std::string> teamBStats;
            calculateGameStats(eventsCopy, generalStats, teamAStats, teamBStats);
            writeSummaryToFile(filePath, eventsCopy, generalStats, teamAStats, teamBStats);
        }
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
