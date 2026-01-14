package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompMessageEncoderDecoder;
import bgu.spl.net.api.StompMessagingProtocolImpl;
import bgu.spl.net.srv.Reactor;
import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        Server.threadPerClient(
                3000,
                () -> new StompMessagingProtocolImpl(),
                () -> new StompMessageEncoderDecoder()
        ).serve();

    }
}
