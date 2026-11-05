package devtitans.hiperion

import android.content.Context
import android.content.SharedPreferences
import java.util.UUID

class UserPreferences(context: Context) {
    private val prefs: SharedPreferences = context.getSharedPreferences("hiperion_prefs", Context.MODE_PRIVATE)

    var userName: String
        get() = prefs.getString("user_name", "") ?: ""
        set(value) = prefs.edit().putString("user_name", value).apply()

    var connectionCode: String
        get() = prefs.getString("connection_code", "") ?: ""
        set(value) = prefs.edit().putString("connection_code", value).apply()

    // O Device ID é gerado apenas uma vez e salvo para sempre
    val deviceId: String
        get() {
            var id = prefs.getString("device_id", null)
            if (id == null) {
                // Gera um ID simples (ex: hiperion_a1b2c3d)
                val randomPart = UUID.randomUUID().toString().substring(0, 8)
                id = "hiperion_$randomPart"
                prefs.edit().putString("device_id", id).apply()
            }
            return id
        }

    fun isConfigured(): Boolean {
        return userName.isNotEmpty() && connectionCode.isNotEmpty()
    }
}