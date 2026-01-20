#pragma once

#include <string>
#include <map>
#include <vector>

class StompFrame {
public:
    std::string command;
    std::map<std::string, std::string> headers;
    std::string body;

    StompFrame(std::string command);

    void addHeader(const std::string& key, const std::string& value);

    std::string toString() const;

    static StompFrame fromString(const std::string& rawFrame);

    std::string getBody() const;

    void setBody(std::string body);
    
};
