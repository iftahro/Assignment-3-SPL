package bgu.spl.net.srv;

import java.io.IOException;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class ConnectionsImpl<T> implements Connections<T> {
    int counter = 1;
    private final Map<Integer, ConnectionHandler<T>> handlerMap = new ConcurrentHashMap<>();
    // Map for {username: [password,isLoggedIn]}
    private final Map<String, Object[]> usersMap = new ConcurrentHashMap<>();
    //The map below represents all the games in the system,
    //with each game we will maintain an additional map,
    //where for each registered user we will save their ID for the specific game.
    private final Map<String, Map<Integer,Integer>> gamesMap = new ConcurrentHashMap<>();

    @Override
    public boolean send(int connectionId, T msg) {
        // todo check if missing.
        ConnectionHandler<T> connectionHandler = handlerMap.get(connectionId);
        connectionHandler.send(msg);
        return true;
    }

    @Override
    public void send(String channel, T msg) {
        Map<Integer, Integer> subscribers = getSubscribers(channel);
        for (Map.Entry<Integer, Integer> entry : subscribers.entrySet()) {
            send(entry.getKey(), msg);
        }
    }

    @Override
    public void disconnect(int connectionId) {
        // todo check if missing.
        for (Map.Entry<String, Map<Integer, Integer>> entry : gamesMap.entrySet()) {
            entry.getValue().remove(connectionId);
        }
        try {
            handlerMap.get(connectionId).close();
        } catch (IOException ex) {
            ex.printStackTrace();
        }
        handlerMap.remove(connectionId);
    }

    @Override
    public int addHandler(ConnectionHandler<T> connectionHandler) {
        handlerMap.put(counter, connectionHandler);
        counter++;
        return counter -1;
    }
    public void subscribeToGame(String gameName, int userId, int subId) {
        gamesMap.putIfAbsent(gameName, new ConcurrentHashMap<>());
        gamesMap.get(gameName).put(userId, subId);
    }

    public void unsubscribeFromGame(int userId, int subscriptionId) {
        for (Map.Entry<String, Map<Integer, Integer>> entry : gamesMap.entrySet()) {
            Map<Integer, Integer> subscribers = entry.getValue();
            if (subscribers.containsKey(userId) && subscribers.get(userId) == subscriptionId) {
                subscribers.remove(userId);
            }
        }
    }

    @Override
    public Map<Integer, Integer> getSubscribers(String channel) {
        return gamesMap.get(channel);
    }

    @Override
    public boolean checkPassword(String username, String password) {
        usersMap.putIfAbsent(username, new Object[]{password, false});
        return usersMap.get(username)[0].equals(password);
    }

    @Override
    public boolean checkUserLoggedIn(String username) {
        if ((boolean) usersMap.get(username)[1]) return true;
        usersMap.get(username)[1] = true;
        return false;
    }

    public boolean gameExist(String gameName){
        return gamesMap.containsKey(gameName);
    }

    public boolean isPlayerSubToGame(String gameName, int id ) {
        if (gameExist(gameName)){
            return gamesMap.get(gameName).containsKey(id);
        }
        return false;
    }

    @Override
    public ConnectionHandler<T> getHandler(int id) {
        return handlerMap.get(id);
    }
}
