package devtitans.hiperion

import android.util.Log
import devtitans.loramanager.LoraManager
import kotlinx.coroutines.*
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update

data class ChatMessage(
    val deviceId: String,
    val senderName: String,
    val text: String,
    val isMe: Boolean
)

class ChatManager(private val prefs: UserPreferences) {
    private val loraManager = LoraManager.getInstance()
    
    private val _incomingMessages = MutableStateFlow<List<ChatMessage>>(emptyList())
    val incomingMessages = _incomingMessages.asStateFlow()

    private var pollingJob: Job? = null
    private var lastRxCount = 0L

    fun startListening() {
        if (pollingJob?.isActive == true) return

        pollingJob = CoroutineScope(Dispatchers.IO).launch {
            try {
                if (loraManager.isAwake) {
                    lastRxCount = loraManager.rxCount
                }

                while (isActive) {
                    if (loraManager.isAwake) {
                        val currentCount = loraManager.rxCount
                        if (currentCount > lastRxCount) {
                            lastRxCount = currentCount
                            val rawMessage = loraManager.lastRx
                            processRawMessage(rawMessage)
                        }
                    }
                    delay(2000)
                }
            } catch (e: Exception) {
                Log.e("ChatManager", "Erro no polling", e)
            }
        }
    }

    fun stopListening() {
        pollingJob?.cancel()
    }

    fun sendMessage(text: String): Boolean {
        return try {
            if (!loraManager.isAwake) return false

            val payload = "${prefs.deviceId}|${prefs.userName}|$text"
            val success = loraManager.send(payload)

            if (success) {
                _incomingMessages.update { currentList ->
                    currentList + ChatMessage(prefs.deviceId, prefs.userName, text, true)
                }
            }
            success
        } catch (e: Exception) {
            Log.e("ChatManager", "Erro ao enviar", e)
            false
        }
    }

    private fun processRawMessage(raw: String) {
        val parts = raw.split("|", limit = 3)
        if (parts.size == 3) {
            val deviceId = parts[0]
            val name = parts[1]
            val text = parts[2]
            
            if (deviceId != prefs.deviceId) {
                _incomingMessages.update { currentList ->
                    currentList + ChatMessage(deviceId, name, text, false)
                }
            }
        }
    }
}