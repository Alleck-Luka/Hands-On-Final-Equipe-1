package devtitans.loraapp;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.RemoteException;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import devtitans.loramanager.LoraManager;             // Manager

public class MainActivity extends Activity {
    private static final String TAG = "DevTITANS.LoraApp";

    private static final String CHANNEL_ID = "lora_messages";
    private static final int NOTIFICATION_ID = 1;
    private static final int PERMISSION_REQUEST_CODE = 100;

    private TextView textStatus, textRxCount, textLastRx, textKey;
    private EditText editMessage, editKey;

    private LoraManager loraManager;
    private NotificationManager notificationManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textStatus  = findViewById(R.id.textStatus);
        textRxCount = findViewById(R.id.textRxCount);
        textLastRx  = findViewById(R.id.textLastRx);
        textKey     = findViewById(R.id.textKey);
        editMessage = findViewById(R.id.editMessage);
        editKey     = findViewById(R.id.editKey);

        notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

        loraManager = LoraManager.getInstance();      // 1º uso conecta ao binder

        createNotificationChannel();
        requestNotificationPermission();
        registerListener();
        updateAll(null);
    }

    // ---------- Nível 1.5: notificações ----------

    private void createNotificationChannel() {
        NotificationChannel channel = new NotificationChannel(
                CHANNEL_ID, "Mensagens LoRa", NotificationManager.IMPORTANCE_HIGH);
        channel.setDescription("Notificações de mensagens LoRa recebidas");
        notificationManager.createNotificationChannel(channel);
    }

    private void requestNotificationPermission() {
        if (Build.VERSION.SDK_INT >= 33 &&
                checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS)
                        != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(
                    new String[]{android.Manifest.permission.POST_NOTIFICATIONS},
                    PERMISSION_REQUEST_CODE);
        }
    }

    private void showNotification(String message) {
        Intent intent = new Intent(this, MainActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        PendingIntent pendingIntent = PendingIntent.getActivity(
                this, 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        Notification notification = new Notification.Builder(this, CHANNEL_ID)
                .setSmallIcon(R.drawable.ic_lora)
                .setContentTitle("Mensagem LoRa recebida")
                .setContentText(message == null || message.isEmpty() ? "(sem conteúdo)" : message)
                .setAutoCancel(true)
                .setContentIntent(pendingIntent)
                .build();

        notificationManager.notify(NOTIFICATION_ID, notification);
    }

    // ---------- Nível 1: listener ----------

    private void registerListener() {
        try {
            loraManager.registerListener(new LoraManager.Listener() {
                @Override
                public void onMessageReceived(String message) {
                    runOnUiThread(new Runnable() {          // callback vem de thread do binder
                        @Override
                        public void run() {
                            textLastRx.setText(message == null || message.isEmpty() ? "(nenhuma)" : message);
                            try {
                                textRxCount.setText(String.valueOf(loraManager.getRxCount()));
                            } catch (RemoteException e) {
                                Log.e(TAG, "Erro lendo o contador:", e);
                            }
                            showNotification(message);
                        }
                    });
                }
            });
        } catch (RemoteException e) {
            Log.e(TAG, "Erro ao registrar listener:", e);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            if (loraManager != null)
                loraManager.unregisterListener();
        } catch (RemoteException e) {
            Log.e(TAG, "Erro ao remover listener:", e);
        }
    }

    // Verifica se o Manager foi acessado com sucesso
    private boolean checkService() {
        if (loraManager == null) {
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
            int status = loraManager.connect();
            if (status == 1) {
                textStatus.setText("Conectado");
                textStatus.setTextColor(Color.parseColor("#6d790c"));
            }
            else {
                textStatus.setText("Desconectado");
                textStatus.setTextColor(Color.parseColor("#73312f"));
            }

            textKey.setText(loraManager.getKey());
            updateRx();
        } catch (RemoteException e) {
            Toast.makeText(this, "Erro ao acessar o serviço!", Toast.LENGTH_LONG).show();
            Log.e(TAG, "Erro atualizando dados:", e);
        }
    }

    // Lê a recepção (contador + última mensagem recebida)
    private void updateRx() {
        if (!checkService()) return;
        try {
            long count = loraManager.getRxCount();
            textRxCount.setText(String.valueOf(count));

            String last = loraManager.getLastRx();
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
            boolean ok = loraManager.send(message);
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
            boolean ok = loraManager.setKey(newKey);
            Toast.makeText(this, ok ? "Senha alterada!" : "Falha ao alterar a senha",
                           Toast.LENGTH_SHORT).show();
            if (ok) {
                editKey.setText("");
                textKey.setText(loraManager.getKey());
            }
        } catch (RemoteException e) {
            Toast.makeText(this, "Erro ao trocar a senha!", Toast.LENGTH_LONG).show();
            Log.e(TAG, "Erro trocando a senha:", e);
        }
    }
}
