package devtitans.loramanager;

import android.util.Log;
import android.os.ServiceManager;
import android.os.IBinder;
import android.os.RemoteException;

import devtitans.lora.ILora;                          // Criado pelo AIDL

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
    private static int state = 0; // Substituir por chamado ao service aqui e no getLoraState

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

    public int getLoraState() throws RemoteException {
        Log.d(TAG, "Executando método getLoraState() ...");
        // Chamar aqui o service para pegar o estado
        return state;
    }

    // enabled false desabilita
    public void setLoraState(int enabled) throws RemoteException {
        // Chamar aqui o service para alterar o estado
        Log.d(TAG, "Executando método setState(" + enabled + ") ...");
        state = enabled;
    }

}
