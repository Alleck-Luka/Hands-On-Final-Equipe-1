package devtitans.loramanager;

import android.util.Log;
import android.os.ServiceManager;
import android.os.IBinder;
import android.os.RemoteException;

import devtitans.lora.ILora;                          // Criado pelo AIDL
import devtitans.lora.ILoraCallback;                  // Criado pelo AIDL

/*
    ====================================
    Remover os Log.d no final do projeto
    ====================================
 */

public class LoraManager {
    private static final String TAG = "DevTITANS.LoraManager";
    private IBinder binder;
    private ILora service;

    public static final int LORA_STATE_DISABLED = 0;
    public static final int LORA_STATE_ENABLED = 1;
    public static final int LORA_STATE_UNAVAILABLE = 2;

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
        if (service == null) {
            Log.e(TAG, "Erro ao acessar o serviço Lora!");
            return 0;
        }
        Log.d(TAG, "Executando método connect() ...");
        return service.connect();
    }

    public boolean send(String payload) throws RemoteException {
        if (service == null) {
            Log.e(TAG, "Erro ao acessar o serviço Lora!");
            return false;
        }
        Log.d(TAG, "Executando método send(" + payload + ") ...");
        return service.send(payload);
    }

    public boolean ping() throws RemoteException {
        if (service == null) {
            Log.e(TAG, "Erro ao acessar o serviço Lora!");
            return false;
        }
        Log.d(TAG, "Executando método ping() ...");
        return service.ping();
    }

    public boolean sleep() throws RemoteException {
        if (service == null) {
            Log.e(TAG, "Erro ao acessar o serviço Lora!");
            return false;
        }
        Log.d(TAG, "Executando método sleep() ...");
        return service.sleep();
    }

    public boolean wakeup() throws RemoteException {
        if (service == null) {
            Log.e(TAG, "Erro ao acessar o serviço Lora!");
            return false;
        }        
        Log.d(TAG, "Executando método wakeup() ...");
        return service.wakeup();
    }

    public boolean isAwake() throws RemoteException {
        if (service == null) {
            Log.e(TAG, "Erro ao acessar o serviço Lora!");
            return false;
        }
        Log.d(TAG, "Executando método isAwake() ...");
        return service.isAwake();
    }

    public int getAux() throws RemoteException {
        if (service == null) {
            Log.e(TAG, "Erro ao acessar o serviço Lora!");
            return 0;
        }
        Log.d(TAG, "Executando método getAux() ...");
        return service.getAux();
    }

    public long getRxCount() throws RemoteException {
        if (service == null) {
            Log.e(TAG, "Erro ao acessar o serviço Lora!");
            return 0;
        }
        Log.d(TAG, "Executando método getRxCount() ...");
        return service.getRxCount();
    }

    public String getLastRx() throws RemoteException {
        if (service == null) {
            Log.e(TAG, "Erro ao acessar o serviço Lora!");
            return "";
        }
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
                listener.onMessageReceived(message);   // ATENÇÃO: thread do binder!
            }

            // O ILoraCallback é @VintfStability, então o Stub gerado pelo AIDL
            // exige os métodos de versão/hash da interface. Como a interface não
            // tem versão "frozen", usamos os valores padrão.
            @Override
            public int getInterfaceVersion() {
                return 0;
            }

            @Override
            public String getInterfaceHash() {
                return "notfrozen";
            }
        };
        service.registerCallback(callback);
    }

    public void unregisterListener() throws RemoteException {
        if (callback != null)
            service.unregisterCallback(callback);
    }
}
