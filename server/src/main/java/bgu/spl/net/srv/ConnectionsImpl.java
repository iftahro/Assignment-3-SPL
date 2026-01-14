package bgu.spl.net.srv;

import java.io.IOException;
import java.lang.reflect.Array;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

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
        Map<Integer,Integer> tempMap = gamesMap.get(channel);
        for (Map.Entry<Integer, Integer> entry : tempMap.entrySet()) {
            int connectId = entry.getKey();
            int gameId = entry.getValue();
            String tempMsg = (String) msg;
            String finalMsg = tempMsg.replaceFirst("MESSAGE\n",
                    "MESSAGE\nsubscription:" + gameId + "\n");
            send(connectId,(T) finalMsg);
        }
    }

    @Override
    public void disconnect(int connectionId) {
        // todo check if missing.
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
