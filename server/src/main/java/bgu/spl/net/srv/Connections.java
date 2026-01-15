package bgu.spl.net.srv;

public interface Connections<T> {

    boolean send(int connectionId, T msg);

    void send(String channel, T msg);

    void disconnect(int connectionId);

    int addHandler(ConnectionHandler<T> connectionHandler);

    ConnectionHandler<T> getHandler(int id);

    boolean isPlayerSubToGame(String gameName, int id );

    void subscribeToGame(String gameName, int userId, int gameId);

    void unsubscribeFromGame(int userId, int subscriptionId);
}
