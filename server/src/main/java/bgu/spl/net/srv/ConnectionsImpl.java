package bgu.spl.net.srv;

import java.io.IOException;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class ConnectionsImpl<T> implements Connections<T> {
    private final AtomicInteger connectionIdCounter = new AtomicInteger(0);
    private final Map<Integer, ConnectionHandler<T>> handlerMap = new ConcurrentHashMap<>();

    @Override
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> connectionHandler = handlerMap.get(connectionId);
        connectionHandler.send(msg);
        return true;
    }

    @Override
    public void send(String channel, T msg) {

    }

    @Override
    public void disconnect(int connectionId) {
        ConnectionHandler<T> handler = handlerMap.remove(connectionId);
        if (handler != null) {
            try {
                handler.close();
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }
    }

    @Override
    public int addHandler(ConnectionHandler<T> connectionHandler) {
        int connectionId = connectionIdCounter.getAndIncrement();
        handlerMap.put(connectionId, connectionHandler);
        return connectionId;
    }

    @Override
    public ConnectionHandler<T> getHandler(int id) {
        return handlerMap.get(id);
    }
}
