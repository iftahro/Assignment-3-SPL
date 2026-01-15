package bgu.spl.net.srv;

import java.io.IOException;

public interface Connections<T> {

    boolean send(int connectionId, T msg);

    void send(String channel, T msg);

    void disconnect(int connectionId);

    int addHandler(ConnectionHandler<T> connectionHandler);

    ConnectionHandler<T> getHandler(int id);

    boolean gameExist(String gameName);

    boolean playerSubToGAme(String gameName, int id );

    void SubscribeToGame(String gameName, int userId, int gameId);

    void UnsubscribeFromGame(int userId, int subscriptionId);
}
