package bgu.spl.net.srv;

import java.util.Map;

public interface Connections<T> {

    boolean send(int connectionId, T msg);

    void send(String channel, T msg);

    void disconnect(int connectionId);

    int addHandler(ConnectionHandler<T> connectionHandler);

    ConnectionHandler<T> getHandler(int id);

    boolean isPlayerSubToGame(String gameName, int id );

    void subscribeToGame(String gameName, int userId, int gameId);

    void unsubscribeFromGame(int userId, int subscriptionId);

    Map<Integer, Integer> getSubscribers(String channel);

    boolean checkPassword(String username, String password);

    boolean checkUserLoggedIn(String username, int connectionId);
}
