package bgu.spl.net.api;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;

public class StompMessageEncoderDecoder implements MessageEncoderDecoder<StompMessage> {
    private byte[] bytes = new byte[1024];
    private int len = 0;

    @Override
    public StompMessage decodeNextByte(byte nextByte) {
        if (nextByte == '\u0000') {
            return parseFrame(popString());
        }
        pushByte(nextByte);
        return null;
    }

    private StompMessage parseFrame(String frameString) {
        String[] lines = frameString.split("\n");
        if (lines.length == 0) return null;
        StompMessage message = new StompMessage(lines[0].trim());
        int i = 1;
        while (i < lines.length && !lines[i].trim().isEmpty()) {
            String line = lines[i];
            int colonIndex = line.indexOf(':');
            if (colonIndex != -1) {
                String key = line.substring(0, colonIndex).trim();
                String value = line.substring(colonIndex + 1).trim();
                message.addHeader(key, value);
            }
            i++;
        }
        StringBuilder body = new StringBuilder();
        i++;
        while (i < lines.length) {
            body.append(lines[i]).append("\n");
            i++;
        }
        message.setBody(body.toString().trim());
        return message;
    }

    @Override
    public byte[] encode(StompMessage message) {
        return (message.toString() + "\u0000").getBytes(StandardCharsets.UTF_8);
    }

    private void pushByte(byte nextByte) {
        if (len >= bytes.length) {
            bytes = Arrays.copyOf(bytes, len * 2);
        }
        bytes[len++] = nextByte;
    }

    private String popString() {
        String result = new String(bytes, 0, len, StandardCharsets.UTF_8);
        len = 0;
        return result;
    }
}