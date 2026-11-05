package devtitans.hiperion

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.systemBarsPadding
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier

// Enum para controlar em qual tela estamos
enum class AppScreen { LOGIN, CHAT, SETTINGS }

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        val prefs = UserPreferences(this)
        val chatManager = ChatManager(prefs)

        setContent {
            MaterialTheme {
                // systemBarsPadding() corrige o problema da Top Bar e dos botões do Android
                Surface(
                    modifier = Modifier.fillMaxSize().systemBarsPadding(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    var currentScreen by remember { 
                        mutableStateOf(if (prefs.isConfigured()) AppScreen.CHAT else AppScreen.LOGIN) 
                    }

                    when (currentScreen) {
                        AppScreen.LOGIN -> {
                            LoginScreen(
                                prefs = prefs,
                                onLoginSuccess = { currentScreen = AppScreen.CHAT },
                                onNavigateSettings = { currentScreen = AppScreen.SETTINGS }
                            )
                        }
                        AppScreen.CHAT -> {
                            ChatScreen(
                                prefs = prefs,
                                chatManager = chatManager,
                                onNavigateSettings = { currentScreen = AppScreen.SETTINGS }
                            )
                        }
                        AppScreen.SETTINGS -> {
                            SettingsScreen(
                                prefs = prefs,
                                onUpdateSuccess = { 
                                    // Retorna pro chat se já tem nome/código, senão pro login
                                    currentScreen = if (prefs.isConfigured()) AppScreen.CHAT else AppScreen.LOGIN 
                                }
                            )
                        }
                    }
                }
            }
        }
    }
}