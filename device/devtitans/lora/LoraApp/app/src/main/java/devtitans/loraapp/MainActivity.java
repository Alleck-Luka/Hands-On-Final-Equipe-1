package devtitans.loraapp;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.RemoteException;
import android.text.InputType;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

import devtitans.loramanager.LoraManager;

public class MainActivity extends Activity {
    private static final String TAG = "DevTITANS.LoraApp";

    private static final String CHANNEL_ID = "lora_messages";
    private static final int NOTIFICATION_ID = 1;
    private static final int PERMISSION_REQUEST_CODE = 100;

    private static final String PREFS_NAME   = "lora_chat";
    private static final String PREF_HISTORY = "chat_history";

    private TextView textStatus;
    private EditText editMessage;
    private RecyclerView recyclerMessages;

    private MessageAdapter adapter;
    private final List<Message> messages = new ArrayList<>();

    private LoraManager loraManager;
    private NotificationManager notificationManager;
    private SharedPreferences prefs;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        textStatus       = findViewById(R.id.textStatus);
        editMessage      = findViewById(R.id.editMessage);
        recyclerMessages = findViewById(R.id.recyclerMessages);

        notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);

        // RecyclerView + adapter do chat
        adapter = new MessageAdapter(messages);
        LinearLayoutManager layoutManager = new LinearLayoutManager(this);
        layoutManager.setStackFromEnd(true);          // novas mensagens no final
        recyclerMessages.setLayoutManager(layoutManager);
        recyclerMessages.setAdapter(adapter);

        loadHistory();

        loraManager = LoraManager.getInstance();      // 1º uso conecta ao binder

        createNotificationChannel();
        requestNotificationPermission();
        registerListener();
        updateStatus();
    }

    // ---------- Histórico (persistência) ----------

    private void loadHistory() {
        String json = prefs.getString(PREF_HISTORY, null);
        if (json == null)
            return;

        try {
            JSONArray arr = new JSONArray(json);
            for (int i = 0; i < arr.length(); i++) {
                JSONObject o = arr.getJSONObject(i);
                messages.add(new Message(
                        o.optString("text", ""),
                        o.optLong("ts", 0),
                        o.optBoolean("sent", false)));
            }
            adapter.notifyDataSetChanged();
            scrollToBottom();
        } catch (JSONException e) {
            Log.e(TAG, "Erro carregando histórico:", e);
        }
    }

    private void saveHistory() {
        JSONArray arr = new JSONArray();
        for (Message m : messages) {
            JSONObject o = new JSONObject();
            try {
                o.put("text", m.text);
                o.put("ts",   m.timestamp);
                o.put("sent", m.sent);
            } catch (JSONException e) {
                Log.e(TAG, "Erro salvando mensagem:", e);
            }
            arr.put(o);
        }
        prefs.edit().putString(PREF_HISTORY, arr.toString()).apply();
    }

    private void addMessage(String text, boolean sent) {
        messages.add(new Message(text, System.currentTimeMillis(), sent));
        adapter.notifyItemInserted(messages.size() - 1);
        scrollToBottom();
        saveHistory();
    }

    private void scrollToBottom() {
        recyclerMessages.post(new Runnable() {
            @Override
            public void run() {
                if (adapter.getItemCount() > 0)
                    recyclerMessages.scrollToPosition(adapter.getItemCount() - 1);
            }
        });
    }

    // ---------- Menu (⋮) ----------

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_refresh) {
            updateStatus();
            return true;
        }
        if (id == R.id.action_set_key) {
            showSetKeyDialog();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void showSetKeyDialog() {
        final EditText input = new EditText(this);
        input.setInputType(InputType.TYPE_CLASS_TEXT);
        input.setHint("nova senha");

        new AlertDialog.Builder(this)
                .setTitle("Trocar senha (key)")
                .setView(input)
                .setPositiveButton("Salvar", new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        String newKey = input.getText().toString().trim();
                        if (newKey.isEmpty()) {
                            Toast.makeText(MainActivity.this, "Digite a nova senha!", Toast.LENGTH_SHORT).show();
                            return;
                        }
                        try {
                            boolean ok = loraManager.setKey(newKey);
                            Toast.makeText(MainActivity.this,
                                    ok ? "Senha alterada!" : "Falha ao alterar a senha",
                                    Toast.LENGTH_SHORT).show();
                        } catch (RemoteException e) {
                            Toast.makeText(MainActivity.this, "Erro ao trocar a senha!", Toast.LENGTH_LONG).show();
                            Log.e(TAG, "Erro trocando a senha:", e);
                        }
                    }
                })
                .setNegativeButton("Cancelar", null)
                .show();
    }

    // ---------- Status ----------

    private void updateStatus() {
        try {
            int status = loraManager.connect();
            if (status == 1) {
                textStatus.setText("● Conectado");
                textStatus.setTextColor(Color.parseColor("#B9F6CA"));
            } else {
                textStatus.setText("● Desconectado");
                textStatus.setTextColor(Color.parseColor("#FFCDD2"));
            }
        } catch (RemoteException e) {
            textStatus.setText("● Erro ao acessar o serviço");
            textStatus.setTextColor(Color.parseColor("#FFCDD2"));
            Log.e(TAG, "Erro atualizando status:", e);
        }
    }

    // ---------- Envio ----------

    public void sendMessage(View view) {
        String message = editMessage.getText().toString().trim();
        if (message.isEmpty()) {
            Toast.makeText(this, "Digite uma mensagem!", Toast.LENGTH_SHORT).show();
            return;
        }

        try {
            boolean ok = loraManager.send(message);
            if (ok) {
                addMessage(message, Message.SENT);
                editMessage.setText("");
            } else {
                Toast.makeText(this, "Falha ao enviar mensagem", Toast.LENGTH_SHORT).show();
            }
        } catch (RemoteException e) {
            Toast.makeText(this, "Erro ao enviar mensagem!", Toast.LENGTH_LONG).show();
            Log.e(TAG, "Erro enviando mensagem:", e);
        }
    }

    // ---------- Recepção (nível 1: listener) ----------

    private void registerListener() {
        try {
            loraManager.registerListener(new LoraManager.Listener() {
                @Override
                public void onMessageReceived(String message) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            addMessage(message, Message.RECEIVED);
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

    // ---------- Notificação (nível 1.5) ----------

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
}
