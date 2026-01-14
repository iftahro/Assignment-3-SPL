package bgu.spl.net.api;

import bgu.spl.net.srv.Connections;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String> {
    private int connectionId;
    private Connections<String> connections;
    private boolean shouldTerminate = false;

    @Override
    public void start(int connectionId, Connections<String> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    @Override
    public void process(String message) {
        String[] lines = message.split("\n");
        if (lines.length == 0) {
            return;
        }

        String command = lines[0].trim();

        if (command.equals("CONNECT")) {
            handleConnect(lines);
        }
    }

    private void handleConnect(String[] lines) {
        String login = null;
        String passcode = null;

        for (String line : lines) {
            if (line.startsWith("login:")) {
                login = line.substring(6).trim();
            } else if (line.startsWith("passcode:")) {
                passcode = line.substring(9).trim();
            }
        }
        // todo - check password.
        if (login != null && passcode != null) {
            String response = "CONNECTED\n" +
                    "version:1.2\n";

            connections.send(connectionId, response);
        } else {
            sendError("Missing login or passcode");
        }
    }

    private void sendError(String errorMsg) {
        // todo write errors.
        String errorFrame = "ERROR\n" +
                "message: " + errorMsg + "\n";
        connections.send(connectionId, errorFrame);
        connections.disconnect(connectionId);
        shouldTerminate = true;
    }

    @Override
    public boolean shouldTerminate() {
        // todo check when needed.
        return shouldTerminate;
    }
}
