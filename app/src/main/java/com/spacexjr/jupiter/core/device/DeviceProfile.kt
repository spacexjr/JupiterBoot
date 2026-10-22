package com.spacexjr.jupiter.core.device

import org.json.JSONObject

/**
 * Perfil de dispositivo para orientação e calibração de toque.
 *
 * Os perfis embarcados só alteram chaves seguras do config.ini. Valores -1
 * significam "detectar automaticamente" e nunca são gravados.
 */
data class DeviceProfile(
    val id: String,
    val name: String,
    val codename: String,
    val orientation: Int,
    val touchCalX0: Int,
    val touchCalY0: Int,
    val touchCalX1: Int,
    val touchCalY1: Int,
    val notes: String,
) {
    fun matches(codename: String): Boolean =
        this.codename.isNotBlank() && this.codename.equals(codename, ignoreCase = true)

    companion object {
        fun parse(json: String): List<DeviceProfile> {
            val root = JSONObject(json)
            val array = root.optJSONArray("profiles") ?: return emptyList()
            return (0 until array.length()).mapNotNull { i ->
                val o = array.optJSONObject(i) ?: return@mapNotNull null
                val id = o.optString("id")
                if (id.isBlank()) return@mapNotNull null
                DeviceProfile(
                    id = id,
                    name = o.optString("name", id),
                    codename = o.optString("codename"),
                    orientation = o.optInt("orientation", 0),
                    touchCalX0 = o.optInt("touchCalX0", -1),
                    touchCalY0 = o.optInt("touchCalY0", -1),
                    touchCalX1 = o.optInt("touchCalX1", -1),
                    touchCalY1 = o.optInt("touchCalY1", -1),
                    notes = o.optString("notes"),
                )
            }
        }

        fun detect(profiles: List<DeviceProfile>, codename: String): DeviceProfile? =
            profiles.firstOrNull { it.matches(codename) }
    }
}
