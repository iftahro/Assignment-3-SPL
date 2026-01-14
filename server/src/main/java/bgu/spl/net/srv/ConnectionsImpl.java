package bgu.spl.net.srv;

import java.io.IOException;
import java.lang.reflect.Array;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class ConnectionsImpl<T> implements Connections<T> {
    int counter = 1;
    private final Map<Integer, ConnectionHandler<T>> connectionsMap = new ConcurrentHashMap<>();
    private final Map<String, Integer[]> gamesMap = new ConcurrentHashMap<>();

    @Override
    public boolean send(int connectionId, T msg) {
        // todo check if missing.
        ConnectionHandler<T> connectionHandler = connectionsMap.get(connectionId);
        connectionHandler.send(msg);
        return true;
    }

    @Override
    public void send(String channel, T msg) {
        for (int tempId : gamesMap.get(channel))
        {
            //todo return val
            send(tempId,msg);
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

    @Override
    public ConnectionHandler<T> getHandler(int id) {
        return connectionsMap.get(id);
    }
}
