package devtitans.loraapp;

public class Message {
    public static final boolean SENT = true;
    public static final boolean RECEIVED = false;

    public final String text;
    public final long timestamp;   // millis desde 1970
    public final boolean sent;     // true = enviada por este dispositivo

    public Message(String text, long timestamp, boolean sent) {
        this.text = text;
        this.timestamp = timestamp;
        this.sent = sent;
    }
}
