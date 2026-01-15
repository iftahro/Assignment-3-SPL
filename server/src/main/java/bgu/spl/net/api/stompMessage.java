package bgu.spl.net.api;

import java.util.LinkedHashMap;
import java.util.Map;

public class stompMessage {
    private String command;
    private Map<String, String> headers;
    private String body;

    public stompMessage(String command) {
        this.command = command;
        this.headers = new LinkedHashMap<>();
        this.body = "";
    }

    public String getCommand() {
        return command;
    }

    public void setCommand(String command) {
        this.command = command;
    }

    public void addHeader(String key, String value) {
        headers.put(key, value);
    }

    public String getHeader(String key) {
        return headers.get(key);
    }

    public Map<String, String> getHeaders() {
        return headers;
    }

    public String getBody() {
        return body;
    }

    public void setBody(String body) {
        this.body = body;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(command).append("\n");
        for (Map.Entry<String, String> header : headers.entrySet()) {
            sb.append(header.getKey()).append(":").append(header.getValue()).append("\n");
        }
        sb.append("\n");
        sb.append(body);
        return sb.toString();
    }
}
