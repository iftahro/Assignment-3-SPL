package bgu.spl.net.api;

import bgu.spl.net.impl.data.Database;
import bgu.spl.net.impl.data.LoginStatus;
import bgu.spl.net.srv.Connections;

import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<StompMessage> {
    private int connectionId;
    private Connections<StompMessage> connections;
    private boolean shouldTerminate = false;
    private final Database database = Database.getInstance();

    @Override
    public void start(int connectionId, Connections<StompMessage> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public void process(StompMessage message) {
        String command = message.getCommand();
        if (!command.equals("CONNECT") && !database.isUserLoggedIn(connectionId)) {
            return;
        }
        switch (command) {
            case "CONNECT":
                handleConnect(message);
                break;
            case "SEND":
                handleSend(message);
                break;
            case "DISCONNECT":
                handleDisconnect(message);
                break;
            case "SUBSCRIBE":
                handleSubscribe(message);
                break;
            case "UNSUBSCRIBE":
                handleUnsubscribe(message);
                break;
            default:
                return;
        }
        if (!shouldTerminate) {
            checkAndSendReceipt(message);
        }
    }
    private void handleSubscribe(StompMessage msg) {
        String destination = msg.getHeader("destination");
        String tempSubId = msg.getHeader("id");
        if (destination == null || destination.isEmpty() ||
        tempSubId == null || tempSubId.isEmpty()) {
            sendError("Malformed SUBSCRIBE frame: missing destination or ID", msg);
            return;
        }
        if (!tempSubId.matches("\\d+")) {
            sendError("Malformed SUBSCRIBE frame: ID must be a number", msg);
            return;
        }
        int subId = Integer.parseInt(tempSubId);
        database.subscribeToGame(destination, connectionId, subId);
    }

    private void handleUnsubscribe(StompMessage msg) {
        String idStr = msg.getHeader("id");
        if (idStr == null || idStr.isEmpty()) {return;}
        int subId = Integer.parseInt(idStr);
        database.unsubscribeFromGame(connectionId, subId);
    }
    private void handleSend(StompMessage msg) {
        String destination = msg.getHeader("destination");
        if (destination == null || destination.isEmpty()) {
            sendError("Malformed SEND frame: missing destination", msg);
            return;
        }
        ConcurrentHashMap<Integer, Integer> subscribers = database.getSubscribers(destination);
        if (subscribers == null) sendError("Channel does not exists", msg);
        if (subscribers.get(connectionId) == null) sendError("User not subscribed to channel", msg);
        for (ConcurrentHashMap.Entry<Integer, Integer> entry : subscribers.entrySet()) {
            int connectId = entry.getKey();
            int subId = entry.getValue();
            StompMessage personalizedFrame = new StompMessage("MESSAGE");
            personalizedFrame.addHeader("subscription", String.valueOf(subId));
            personalizedFrame.addHeader("destination", destination);
            String uniqueID = UUID.randomUUID().toString();
            personalizedFrame.addHeader("message-id", "msg-" + uniqueID);
            personalizedFrame.setBody(msg.getBody());
            connections.send(connectId, personalizedFrame);
        }
    }
    private void handleConnect(StompMessage msg) {
        String login = msg.getHeader("login");
        String passcode = msg.getHeader("passcode");
        LoginStatus loginStatus = database.login(connectionId, login, passcode);
        if (loginStatus.equals(LoginStatus.WRONG_PASSWORD)) {
            sendError("Wrong password", msg);
            return;
        } else if (loginStatus.equals(LoginStatus.ALREADY_LOGGED_IN)) {
            sendError("User already logged in", msg);
            return;
        } else if (loginStatus.equals(LoginStatus.CLIENT_ALREADY_CONNECTED)) {
            sendError("The client is already logged in, log out before trying again", msg);
            return;
        }
        StompMessage response = new StompMessage("CONNECTED");
        response.addHeader("version", "1.2");
        connections.send(connectionId, response);
    }

    private void handleDisconnect(StompMessage message) {
        checkAndSendReceipt(message);
        shouldTerminate = true;
    }

    private void checkAndSendReceipt(StompMessage message) {
        String receiptId = message.getHeader("receipt");
        if (receiptId != null) {
            StompMessage receiptFrame = new StompMessage("RECEIPT");
            receiptFrame.addHeader("receipt-id", receiptId);
            connections.send(connectionId, receiptFrame);
        }
    }

    private void sendError(String errorMsg, StompMessage originalMessage) {
        StompMessage errorFrame = new StompMessage("ERROR");
        errorFrame.addHeader("message", errorMsg);
        StringBuilder body = new StringBuilder();
        if (originalMessage != null) {
            if (originalMessage.getHeader("receipt") != null)
                errorFrame.addHeader("receipt-id", originalMessage.getHeader("receipt"));
            body.append("Input frame:\n-----\n");
            body.append(originalMessage);
            body.append("-----");
        }
        errorFrame.setBody(body.toString());
        connections.send(connectionId, errorFrame);
        shouldTerminate = true;
    }

    @Override
    public void terminateConnection() {
        database.unsubscribeFromAll(connectionId);
        database.logout(connectionId);
        connections.disconnect(connectionId);
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }
}
