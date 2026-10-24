package devtitans.loramanager;

import android.util.Log;
import android.os.ServiceManager;
import android.os.IBinder;
import android.os.RemoteException;

import devtitans.lora.ILora;                          // Criado pelo AIDL
import devtitans.lora.ILoraCallback;                  // Criado pelo AIDL

public class LoraManager {
    private static final String TAG = "DevTITANS.LoraManager";
    private IBinder binder;
    private ILora service;

    private static LoraManager instance;

    // Construtor. Configura a "instância da classe" (objeto) recém-criada.
    // Note o "private" no construtor. Essa classe só pode ser instanciada dentro desse arquivo.
    private LoraManager() {
        Log.d(TAG, "Nova (única) instância do LoraManager ...");

        binder = ServiceManager.getService("devtitans.lora.ILora/default");
        if (binder != null) {
            service = ILora.Stub.asInterface(binder);
            if (service != null)
                Log.d(TAG, "Serviço Lora acessado com sucesso.");
            else
                Log.e(TAG, "Erro ao acessar o serviço Lora!");
        }
        else
            Log.e(TAG, "Erro ao acessar o Binder!");
    }

    // Acessa a (única) instância dessa classe. Se ela não existir ainda, cria.
    // Note o "static" no método. Podemos executá-lo sem precisar instanciar um objeto.
    public static LoraManager getInstance() {
        if (instance == null)
            instance = new LoraManager();

        return instance;
    }

    public int connect() throws RemoteException {
        Log.d(TAG, "Executando método connect() ...");
        return service.connect();
    }

    public boolean send(String payload) throws RemoteException {
        Log.d(TAG, "Executando método send(" + payload + ") ...");
        return service.send(payload);
    }

    public boolean ping() throws RemoteException {
        Log.d(TAG, "Executando método ping() ...");
        return service.ping();
    }

    public int getAux() throws RemoteException {
        Log.d(TAG, "Executando método getAux() ...");
        return service.getAux();
    }

    public long getRxCount() throws RemoteException {
        Log.d(TAG, "Executando método getRxCount() ...");
        return service.getRxCount();
    }

    public String getLastRx() throws RemoteException {
        Log.d(TAG, "Executando método getLastRx() ...");
        return service.getLastRx();
    }

    public String getKey() throws RemoteException {
        Log.d(TAG, "Executando método getKey() ...");
        return service.getKey();
    }

    public boolean setKey(String key) throws RemoteException {
        Log.d(TAG, "Executando método setKey(" + key + ") ...");
        return service.setKey(key);
    }

    // Callback que o app implementa (sem conhecer AIDL)
    public interface Listener {
        void onMessageReceived(String message);
    }

    private ILoraCallback callback;   // callback AIDL interno

    public void registerListener(final Listener listener) throws RemoteException {
        callback = new ILoraCallback.Stub() {
            @Override
            public void onMessageReceived(String message) {
                // Your callback handling logic here
            }

            @Override
            public int getInterfaceVersion() {
                return ILoraCallback.VERSION;
            }

            @Override
            public String getInterfaceHash() {
                return ILoraCallback.HASH;
            }
        };
        service.registerCallback(callback);
    }

    public void unregisterListener() throws RemoteException {
        if (callback != null)
            service.unregisterCallback(callback);
    }
}
