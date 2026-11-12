package devtitans.hiperion

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.ui.graphics.Color

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LoginScreen(
    prefs: UserPreferences,
    onLoginSuccess: () -> Unit,
    onNavigateSettings: () -> Unit
) {
    var name by remember { mutableStateOf(prefs.userName) }
    var code by remember { mutableStateOf(prefs.connectionCode) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        // Mock do Logo / Título
        Text(
            text = "DevTITANS",
            fontSize = 14.sp,
            color = MaterialTheme.colorScheme.secondary
        )
        Text(
            text = "Hipérion",
            fontSize = 28.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(bottom = 48.dp)
        )

        // Campo Nome
        OutlinedTextField(
            value = name,
            onValueChange = { name = it },
            label = { Text("Nome") },
            modifier = Modifier.fillMaxWidth(),
            singleLine = true
        )
        Text(
            text = "Escolha um nome pelo qual possa ser reconhecido.\nDica: o dispositivo sempre terá um número identificador interno",
            fontSize = 12.sp,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 4.dp, bottom = 24.dp)
        )

        // Campo Código de conexão
        OutlinedTextField(
            value = code,
            onValueChange = { code = it },
            label = { Text("Código de conexão") },
            modifier = Modifier.fillMaxWidth(),
            singleLine = true
        )
        Text(
            text = "Insira o código de conexão\nDica: esse código conecta você com dispositivos que compartilham tal código.\nDica: você poderá trocar o código depois em \"configurações\"",
            fontSize = 12.sp,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
            modifier = Modifier
                .fillMaxWidth()
                .padding(top = 4.dp, bottom = 48.dp)
        )

        // Botão Entrar
        Button(
            onClick = {
                if (name.isNotBlank() && code.isNotBlank()) {
                    prefs.userName = name.trim()
                    prefs.connectionCode = code.trim()
                    onLoginSuccess()
                }
            },
            modifier = Modifier
                .fillMaxWidth(0.6f)
                .height(50.dp),
            enabled = name.isNotBlank() && code.isNotBlank()
        ) {
            Text("Entrar", fontSize = 16.sp)
        }
    }
}