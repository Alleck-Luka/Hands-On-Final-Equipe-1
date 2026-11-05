package devtitans.hiperion

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    prefs: UserPreferences,
    onUpdateSuccess: () -> Unit
) {
    var name by remember { mutableStateOf(prefs.userName) }
    var code by remember { mutableStateOf(prefs.connectionCode) }

    Column(modifier = Modifier.fillMaxSize()) {
        // TopBar (Header escuro)
        Surface(
            color = Color(0xFF1E1E1E),
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(
                text = "Senha: ${prefs.connectionCode.ifEmpty { "Nenhuma" }}",
                color = Color.White,
                fontWeight = FontWeight.Bold,
                fontSize = 18.sp,
                modifier = Modifier.padding(16.dp)
            )
        }

        Column(
            modifier = Modifier
                .padding(24.dp)
                .weight(1f)
        ) {
            Text(
                text = "Atualizar",
                fontSize = 28.sp,
                fontWeight = FontWeight.Bold,
                modifier = Modifier.padding(bottom = 24.dp)
            )

            // Campo Nome
            OutlinedTextField(
                value = name,
                onValueChange = { name = it },
                label = { Text("Nome") },
                modifier = Modifier.fillMaxWidth(),
                singleLine = true
            )
            Spacer(modifier = Modifier.height(16.dp))

            // Campo Código de conexão
            OutlinedTextField(
                value = code,
                onValueChange = { code = it },
                label = { Text("Código de conexão") },
                modifier = Modifier.fillMaxWidth(),
                singleLine = true
            )
            Spacer(modifier = Modifier.height(24.dp))

            // Número do dispositivo (Somente leitura)
            Text(text = "Número do dispositivo", fontWeight = FontWeight.Bold, fontSize = 14.sp)
            Text(text = prefs.deviceId, color = Color.Gray, fontSize = 14.sp)

            Spacer(modifier = Modifier.weight(1f))

            // Botão Atualizar
            Button(
                onClick = {
                    if (name.isNotBlank() && code.isNotBlank()) {
                        prefs.userName = name.trim()
                        prefs.connectionCode = code.trim()
                        onUpdateSuccess()
                    }
                },
                modifier = Modifier
                    .fillMaxWidth()
                    .height(50.dp),
                enabled = name.isNotBlank() && code.isNotBlank()
            ) {
                Text("Atualizar", fontSize = 16.sp)
            }
        }
    }
}