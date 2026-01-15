package bgu.spl.net.api;

import bgu.spl.net.srv.Connections;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<stompMessage> {
    private int connectionId;
    private Connections<stompMessage> connections;
    private boolean shouldTerminate = false;

    @Override
    public void start(int connectionId, Connections<stompMessage> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public void process(stompMessage message) {
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
    private void handleSubscribe(stompMessage msg) {
        String destination = msg.getHeader("destination");
        String sGameId = msg.getHeader("id");
        if (destination == null || destination.isEmpty() ||
        sGameId == null || sGameId.isEmpty()) {
            sendError("Malformed SUBSCRIBE frame: missing destination or ID", msg);
            return;
        }
        if (!sGameId.matches("\\d+")) {
            sendError("Malformed SUBSCRIBE frame: ID must be a number", msg);
            return;
        }
        int gameId = Integer.parseInt(sGameId);
        connections.SubscribeToGame(destination,connectionId,gameId);
    }

    private void handleUnsubscribe(stompMessage msg) {
        String idStr = msg.getHeader("id");
        if (idStr == null || idStr.isEmpty()) {
            sendError("Malformed UNSUBSCRIBE frame: missing id header", msg);
            return;
        }
        int gameId = Integer.parseInt(idStr);
            connections.UnsubscribeFromGame(connectionId, gameId);
    }
    private void handleSend(stompMessage msg) {
        String destination = msg.getHeader("destination");
        if (destination == null || destination.isEmpty()) {
            sendError("Malformed SEND frame: missing destination", msg);
            return;
        }
        if (connections.playerSubToGAme(destination, connectionId)) {
            stompMessage messageFrame = new stompMessage("MESSAGE");
            messageFrame.addHeader("destination", destination);
            messageFrame.addHeader("message-id", "msg_" + System.currentTimeMillis());
            messageFrame.setBody(msg.getBody());
            connections.send(destination, messageFrame);
        } else {
            sendError("User not subscribed to channel " + destination, msg);
        }
    }
    private void handleConnect(stompMessage msg) {
        String login = msg.getHeader("login");
        String passcode = msg.getHeader("passcode");
        if (login != null && passcode != null) {
            stompMessage response = new stompMessage("CONNECTED");
            response.addHeader("version", "1.2");
            connections.send(connectionId, response);
        } else {
            sendError("Missing login or passcode", msg);
        }
    }

private void handleDisconnect(stompMessage message) {
    checkAndSendReceipt(message);
    connections.disconnect(connectionId);
    shouldTerminate = true;
}

private void checkAndSendReceipt(stompMessage message) {
    String receiptId = message.getHeader("receipt");
    if (receiptId != null) {
        stompMessage receiptFrame = new stompMessage("RECEIPT");
        receiptFrame.addHeader("receipt-id", receiptId);
        connections.send(connectionId, receiptFrame);
    }
}

private void sendError(String errorMsg, stompMessage originalMessage) {
    stompMessage errorFrame = new stompMessage("ERROR");
    errorFrame.addHeader("message", errorMsg);
    StringBuilder body = new StringBuilder();
    if (originalMessage != null) {
        if (originalMessage.getHeader("receipt") != null)
            errorFrame.addHeader("receipt-id", originalMessage.getHeader("receipt"));
        body.append("The message:\n-----\n");
        body.append(originalMessage.toString());

    }
    else {
        body.append("(No original message)");
    }
    errorFrame.setBody(body.toString());
    connections.send(connectionId, errorFrame);
    connections.disconnect(connectionId);
    shouldTerminate = true;
}
    @Override
    public void connectionTerminated() {
        connections.disconnect(connectionId);
    }
    @Override
    public boolean shouldTerminate() {
        // todo check when needed.
        return shouldTerminate;
    }
}
