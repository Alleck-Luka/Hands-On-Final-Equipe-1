package devtitans.lora;

import devtitans.lora.ILoraCallback;

@VintfStability
interface ILora {
    int connect();
    boolean send(in String payload);
    boolean ping();
    int getAux();
    long getRxCount();
    String getLastRx();
    String getKey();
    boolean setKey(in String key);
    void registerCallback(in ILoraCallback callback);
    void unregisterCallback(in ILoraCallback callback);
}

