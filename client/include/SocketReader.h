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
    volatile bool *shouldTerminate;
    std::map<int, std::string> &receiptToMessage;
    std::map<std::string, int> &channelToSubId;
    std::mutex* reportMutex;
    
    // רפרנס למפה הראשית (לשמירת הנתונים עבור ה-Summary)
    std::map<std::string, std::map<std::string, std::vector<Event>>> &gameReports;

public:
    // בנאי: מקבל מצביעים ורפרנסים למשתנים המשותפים ב-main
    SocketReader(ConnectionHandler *handler, 
                 volatile bool *shouldTerminate,
                 std::map<int, std::string> &receipts, 
                 std::map<std::string, int> &subs,
                 std::map<std::string, std::map<std::string, std::vector<Event>>> &reports
                ,std::mutex* mutex);

    // האופרטור שמאפשר למחלקה לרוץ בתוך thread
    void operator()();
};