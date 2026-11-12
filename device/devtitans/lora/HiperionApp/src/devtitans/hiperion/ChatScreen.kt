package devtitans.hiperion

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import java.text.SimpleDateFormat
import java.util.*

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ChatScreen(
    prefs: UserPreferences,
    chatManager: ChatManager,
    onNavigateSettings: () -> Unit
) {
    val messages by chatManager.incomingMessages.collectAsState(initial = emptyList())
    var inputText by remember { mutableStateOf("") }

    // Inicia a escuta da porta serial/LoRa
    LaunchedEffect(Unit) {
        chatManager.startListening()
    }

    Column(modifier = Modifier.fillMaxSize()) {
        // TopBar (Header escuro)
        Surface(
            color = Color(0xFF1E1E1E),
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(
                text = "Código: ${prefs.connectionCode}",
                color = Color.White,
                fontWeight = FontWeight.Bold,
                fontSize = 18.sp,
                modifier = Modifier.padding(16.dp)
            )
        }

        // Lista de Mensagens
        LazyColumn(
            modifier = Modifier
                .weight(1f)
                .fillMaxWidth()
                .padding(16.dp),
            reverseLayout = false
        ) {
            items(messages) { msg ->
                MessageBubble(msg)
                Spacer(modifier = Modifier.height(8.dp))
            }
        }

        // Input de Texto
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            OutlinedTextField(
                value = inputText,
                onValueChange = { inputText = it },
                modifier = Modifier.weight(1f),
                placeholder = { Text("Mensagem") }
            )
            Spacer(modifier = Modifier.width(8.dp))
            Button(
                onClick = {
                    if (inputText.isNotBlank()) {
                        chatManager.sendMessage(inputText)
                        inputText = ""
                    }
                }
            ) {
                Text("Enviar")
            }
        }

        HorizontalDivider(thickness = 1.dp, color = Color.LightGray)

        // --- RODAPÉ DE CONFIGURAÇÕES ---
        Column(
            modifier = Modifier.fillMaxWidth().padding(bottom = 8.dp),
            horizontalAlignment = androidx.compose.ui.Alignment.CenterHorizontally
        ) {
            IconButton(onClick = onNavigateSettings) {
                Icon(
                    imageVector = Icons.Filled.Settings,
                    contentDescription = "Configurações",
                    tint = Color.Gray,
                    modifier = Modifier.size(32.dp)
                )
            }
            Text("Configurações", fontSize = 10.sp, color = Color.Gray)
        }
    }
}

@Composable
fun MessageBubble(message: ChatMessage) {
    val alignment = if (message.isMe) Alignment.CenterEnd else Alignment.CenterStart
    val bgColor = if (message.isMe) Color(0xFF1E88E5) else Color(0xFF81D4FA)
    val textColor = if (message.isMe) Color.White else Color.Black
    val timeFormatter = SimpleDateFormat("HH:mm", Locale.getDefault())
    val timeStr = timeFormatter.format(Date())

    Box(
        modifier = Modifier.fillMaxWidth(),
        contentAlignment = alignment
    ) {
        Column(horizontalAlignment = if (message.isMe) Alignment.End else Alignment.Start) {
            Text(
                text = if (message.isMe) "${message.senderName} (Eu)" else message.senderName,
                fontSize = 12.sp,
                fontWeight = FontWeight.Bold,
                modifier = Modifier.padding(bottom = 2.dp)
            )
            Surface(
                shape = RoundedCornerShape(8.dp),
                color = bgColor
            ) {
                Row(
                    modifier = Modifier.padding(12.dp),
                    verticalAlignment = Alignment.Bottom
                ) {
                    if (!message.isMe) {
                        Text(text = timeStr, fontSize = 10.sp, color = Color.DarkGray, modifier = Modifier.padding(end = 8.dp))
                    }
                    Text(text = message.text, color = textColor)
                    if (message.isMe) {
                        Text(text = timeStr, fontSize = 10.sp, color = Color.LightGray, modifier = Modifier.padding(start = 8.dp))
                    }
                }
            }
            Text(
                text = "Dispositivo: ${message.deviceId}",
                fontSize = 10.sp,
                color = Color.Gray,
                modifier = Modifier.padding(top = 2.dp)
            )
        }
    }
}