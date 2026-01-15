package bgu.spl.net.api;

import bgu.spl.net.srv.Connections;

import java.util.Map;
import java.util.UUID;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<StompMessage> {
    private int connectionId;
    private Connections<StompMessage> connections;
    private boolean shouldTerminate = false;

    @Override
    public void start(int connectionId, Connections<StompMessage> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public void process(StompMessage message) {
        String command = message.getCommand();

        if (command == null || command.isEmpty()) {
            sendError("Malformed frame: missing command", null);
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
        connections.subscribeToGame(destination,connectionId,subId);
    }

    private void handleUnsubscribe(StompMessage msg) {
        String idStr = msg.getHeader("id");
        if (idStr == null || idStr.isEmpty()) {return;}
        int subId = Integer.parseInt(idStr);
        connections.unsubscribeFromGame(connectionId, subId);
    }
    private void handleSend(StompMessage msg) {
        String destination = msg.getHeader("destination");
        if (destination == null || destination.isEmpty()) {
            sendError("Malformed SEND frame: missing destination", msg);
            return;
        }
        if (connections.isPlayerSubToGame(destination, connectionId)) {
            Map<Integer, Integer> subscribers = connections.getSubscribers(destination);
            if (subscribers != null) {
                for (Map.Entry<Integer, Integer> entry : subscribers.entrySet()) {
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
        } else {
            sendError("User not subscribed to channel " + destination, msg);
        }
    }
    private void handleConnect(StompMessage msg) {
        String login = msg.getHeader("login");
        String passcode = msg.getHeader("passcode");
        if (!connections.checkPassword(login, passcode)) {
            sendError("Wrong password", msg);
            return;
        } else if (connections.checkUserLoggedIn(login)) {
            sendError("User already logged in", msg);
            return;
        }
        StompMessage response = new StompMessage("CONNECTED");
        response.addHeader("version", "1.2");
        connections.send(connectionId, response);
    }

private void handleDisconnect(StompMessage message) {
    checkAndSendReceipt(message);
    terminateConnection();
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
        body.append(originalMessage.toString());
        body.append("-----");
    }
    errorFrame.setBody(body.toString());
    connections.send(connectionId, errorFrame);
    terminateConnection();
    shouldTerminate = true;
}
    @Override
    public void terminateConnection() {
        connections.disconnect(connectionId);
    }
    @Override
    public boolean shouldTerminate() {
        // todo check when needed.
        return shouldTerminate;
    }
}
