package bgu.spl.net.srv;

import java.io.IOException;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import bgu.spl.net.api.stompMessage;

public class ConnectionsImpl<T> implements Connections<T> {
    int counter = 1;
    private final Map<Integer, ConnectionHandler<T>> connectionsMap = new ConcurrentHashMap<>();
    private final Map<String, Map<Integer,Integer>> gamesMap = new ConcurrentHashMap<>();

    @Override
    public boolean send(int connectionId, T msg) {
        // todo check if missing.
        ConnectionHandler<T> connectionHandler = connectionsMap.get(connectionId);
        connectionHandler.send(msg);
        return true;
    }

    @Override
    public void send(String channel, T msg) {
        Map<Integer, Integer> subscribers = gamesMap.get(channel);
        if (subscribers != null && msg instanceof stompMessage) {
            stompMessage originalFrame = (stompMessage) msg;
            for (Map.Entry<Integer, Integer> entry : subscribers.entrySet()) {
                int connectId = entry.getKey();
                int subId = entry.getValue();
                stompMessage personalizedFrame = new stompMessage("MESSAGE");
                personalizedFrame.addHeader("subscription", String.valueOf(subId));
                personalizedFrame.addHeader("destination", channel);
                personalizedFrame.addHeader("message-id", "msg_" + (counter++));
                personalizedFrame.setBody(originalFrame.getBody());
                send(connectId, (T) personalizedFrame);
            }
        }
    }

    @Override
    public void disconnect(int connectionId) {
        // todo check if missing.
        for (Map.Entry<String, Map<Integer, Integer>> entry : gamesMap.entrySet()) {
            entry.getValue().remove(connectionId);
        }
        try {
            connectionsMap.get(connectionId).close();
        } catch (IOException ex) {
            ex.printStackTrace();
        }
        connectionsMap.remove(connectionId);
    }

    @Override
    public int addHandler(ConnectionHandler<T> connectionHandler) {
        connectionsMap.put(counter, connectionHandler);
        counter++;
        return counter -1;
    }
    public void SubscribeToGame(String gameName, int userId, int gameId)
    {
        gamesMap.putIfAbsent(gameName, new ConcurrentHashMap<>());
        gamesMap.get(gameName).put(userId, gameId);
    }

    public void UnsubscribeFromGame(int userId, int subscriptionId) {
        for (Map.Entry<String, Map<Integer, Integer>> entry : gamesMap.entrySet()) {
            Map<Integer, Integer> subscribers = entry.getValue();
            if (subscribers.containsKey(userId) && subscribers.get(userId) == subscriptionId) {
                subscribers.remove(userId);
            }
        }
    }

    public boolean gameExist(String gameName){
        return gamesMap.containsKey(gameName);
    }

    public boolean playerSubToGAme(String gameName, int id )
    {
        if (gameExist(gameName)){
            return gamesMap.get(gameName).containsKey(id);
        }
        return false;
    }

    @Override
    public ConnectionHandler<T> getHandler(int id) {
        return connectionsMap.get(id);
    }
}
