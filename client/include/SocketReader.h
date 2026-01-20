#pragma once
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"

class SocketReader
{
private:
    ConnectionHandler *handler;
    volatile bool *shouldTerminate;
    StompProtocol protocol;

public:
    SocketReader(ConnectionHandler *handler, volatile bool *shouldTerminate, StompProtocol protocol);

    void operator()();
};