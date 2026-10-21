package com.spacexjr.jupiter.core.prefs

import android.content.Context
import android.content.SharedPreferences
import androidx.core.content.edit

/** Preferências do app (SharedPreferences). */
class AppPrefs(context: Context) {

    private val prefs: SharedPreferences =
        context.getSharedPreferences("jupiter_prefs", Context.MODE_PRIVATE)

    var themeMode: ThemeMode
        get() = ThemeMode.from(prefs.getString(KEY_THEME_MODE, ThemeMode.SYSTEM.name))
        set(value) = prefs.edit { putString(KEY_THEME_MODE, value.name) }

    var dynamicColor: Boolean
        get() = prefs.getBoolean(KEY_DYNAMIC_COLOR, true)
        set(value) = prefs.edit { putBoolean(KEY_DYNAMIC_COLOR, value) }

    var configRootId: String
        get() = prefs.getString(KEY_CONFIG_ROOT, "device") ?: "device"
        set(value) = prefs.edit { putString(KEY_CONFIG_ROOT, value) }

    companion object {
        private const val KEY_THEME_MODE = "theme_mode"
        private const val KEY_DYNAMIC_COLOR = "dynamic_color"
        private const val KEY_CONFIG_ROOT = "config_root"
    }
}

enum class ThemeMode(val label: String) {
    SYSTEM("Automático"),
    LIGHT("Claro"),
    DARK("Escuro");

    companion object {
        fun from(name: String?): ThemeMode =
            entries.firstOrNull { it.name == name } ?: SYSTEM
    }
}
