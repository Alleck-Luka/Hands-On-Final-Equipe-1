package devtitans.loraapp;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import devtitans.lora.ILora;                          // Criado pelo AIDL

public class MainActivity extends Activity {
    private static final String TAG = "DevTITANS.LoraApp";
    private static final long POLL_MS = 1000;        // intervalo de atualização da recepção

    private TextView textStatus, textRxCount, textLastRx, textKey;
    private EditText editMessage, editKey;

    private IBinder binder;
    private ILora service;

    // Atualiza a recepção periodicamente (efeito de "receber mensagem" no app)
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Runnable poller = new Runnable() {
        @Override
        public void run() {
            updateRx();
            handler.postDelayed(this, POLL_MS);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textStatus  = findViewById(R.id.textStatus);      // Acessa os componentes da tela
        textRxCount = findViewById(R.id.textRxCount);
        textLastRx  = findViewById(R.id.textLastRx);
        textKey     = findViewById(R.id.textKey);
        editMessage = findViewById(R.id.editMessage);
        editKey     = findViewById(R.id.editKey);

        binder = ServiceManager.getService("devtitans.lora.ILora/default");   // Acessa e consulta o binder
        if (binder != null) {
            service = ILora.Stub.asInterface(binder);                         // Acessa o serviço Lora
            if (service != null)
                Toast.makeText(this, "Serviço Lora acessado com sucesso.", Toast.LENGTH_LONG).show();
            else
                Toast.makeText(this, "Erro ao acessar o serviço Lora!", Toast.LENGTH_LONG).show();
        }
        else
            Toast.makeText(this, "Erro ao acessar o Binder!", Toast.LENGTH_LONG).show();

        updateAll(null);
    }

    @Override
    protected void onResume() {
        super.onResume();
        handler.postDelayed(poller, POLL_MS);
    }

    @Override
    protected void onPause() {
        super.onPause();
        handler.removeCallbacks(poller);
    }

    // Verifica se o binder e o serviço foram acessados com sucesso
    private boolean checkService() {
        if (binder == null) {
            textStatus.setText("Erro no Binder");
            textStatus.setTextColor(Color.parseColor("#73312f"));
            return false;
        }
        if (service == null) {
            textStatus.setText("Erro no Serviço");
            textStatus.setTextColor(Color.parseColor("#73312f"));
            return false;
        }
        return true;
    }

    // Executado ao clicar no botão "Atualizar"
    public void updateAll(View view) {
        Log.d(TAG, "Atualizando dados do dispositivo ...");
        if (!checkService()) return;

        textStatus.setText("Atualizando ...");
        textStatus.setTextColor(Color.parseColor("#c47e00"));

        try {
            int status = service.connect();                      // connect() via IPC
            if (status == 1) {
                textStatus.setText("Conectado");
                textStatus.setTextColor(Color.parseColor("#6d790c"));
            }
            else {
                textStatus.setText("Desconectado");
                textStatus.setTextColor(Color.parseColor("#73312f"));
            }

            textKey.setText(service.getKey());                   // getKey() via IPC
            updateRx();
        } catch (android.os.RemoteException e) {
            Toast.makeText(this, "Erro ao acessar o Binder!", Toast.LENGTH_LONG).show();
            Log.e(TAG, "Erro atualizando dados:", e);
        }
    }

    // Lê a recepção (contador + última mensagem recebida)
    private void updateRx() {
        if (!checkService()) return;
        try {
            long count = service.getRxCount();                   // getRxCount() via IPC
            textRxCount.setText(String.valueOf(count));

            String last = service.getLastRx();                   // getLastRx() via IPC
            textLastRx.setText(last == null || last.isEmpty() ? "(nenhuma)" : last);
        } catch (RemoteException e) {
            Log.e(TAG, "Erro lendo a recepção:", e);
        }
    }

    // Executado ao clicar no botão "Enviar"
    public void sendMessage(View view) {
        if (!checkService()) return;

        String message = editMessage.getText().toString().trim();
        if (message.isEmpty()) {
            Toast.makeText(this, "Digite uma mensagem!", Toast.LENGTH_SHORT).show();
            return;
        }

        try {
            boolean ok = service.send(message);                  // send() via IPC
            Toast.makeText(this, ok ? "Mensagem enviada!" : "Falha ao enviar mensagem",
                           Toast.LENGTH_SHORT).show();
            if (ok)
                editMessage.setText("");
        } catch (RemoteException e) {
            Toast.makeText(this, "Erro ao enviar mensagem!", Toast.LENGTH_LONG).show();
            Log.e(TAG, "Erro enviando mensagem:", e);
        }
    }

    // Executado ao clicar no botão "Trocar senha"
    public void setKey(View view) {
        if (!checkService()) return;

        String newKey = editKey.getText().toString().trim();
        if (newKey.isEmpty()) {
            Toast.makeText(this, "Digite a nova senha!", Toast.LENGTH_SHORT).show();
            return;
        }

        try {
            boolean ok = service.setKey(newKey);                 // setKey() via IPC
            Toast.makeText(this, ok ? "Senha alterada!" : "Falha ao alterar a senha",
                           Toast.LENGTH_SHORT).show();
            if (ok) {
                editKey.setText("");
                textKey.setText(service.getKey());
            }
        } catch (RemoteException e) {
            Toast.makeText(this, "Erro ao trocar a senha!", Toast.LENGTH_LONG).show();
            Log.e(TAG, "Erro trocando a senha:", e);
        }
    }
}
