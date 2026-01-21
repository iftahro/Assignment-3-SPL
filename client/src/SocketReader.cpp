#include "../include/SocketReader.h"

SocketReader::SocketReader(ConnectionHandler *handler, volatile bool *shouldTerminate, StompProtocol protocol)
    : handler(handler), shouldTerminate(shouldTerminate), protocol(protocol) 
{}

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
        protocol.process(response);
    }
}