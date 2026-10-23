package devtitans.lora;

@VintfStability
interface ILora {
    int connect();
    boolean send(in String payload);
    boolean ping();
    boolean sleep();
    boolean wakeup();
    boolean isAwake();
    int getAux();
    long getRxCount();
    String getLastRx();
    String getKey();
    boolean setKey(in String key);
}

