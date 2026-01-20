#include "../include/StompFrame.h"
#include <sstream>
#include <iostream>

StompFrame::StompFrame(std::string command) : command(command), headers(), body("") {}

void StompFrame::addHeader(const std::string &key, const std::string &value)
{
    headers[key] = value;
}

std::string StompFrame::toString() const
{
    std::stringstream frame_str;
    frame_str << command << "\n";

    for (const auto &pair : headers)
    {
        frame_str << pair.first << ":" << pair.second << "\n";
    }

    frame_str << "\n";
    frame_str << body;

    return frame_str.str();
}
void StompFrame::setBody(std::string body)
{
    this->body = body;
}

std::string StompFrame::getBody() const
{
    return body;
}

StompFrame StompFrame::fromString(const std::string &rawFrame)
{
    std::stringstream frame_str(rawFrame);
    std::string line;

    if (!std::getline(frame_str, line))
    {
        return StompFrame("ERROR");
    }
    StompFrame frame(line);

    while (std::getline(frame_str, line) && !line.empty())
    {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            frame.addHeader(key, value);
        }
    }
    std::string bodyAccumulator;
    while (std::getline(frame_str, line)){
        bodyAccumulator += line + "\n";
    }

    if (!bodyAccumulator.empty() && bodyAccumulator.back() == '\n')
    {
        bodyAccumulator.pop_back();
    }

    frame.body = bodyAccumulator;

    return frame;
}
