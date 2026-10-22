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

    /** Versao do jbm_init instalada por atualizacao (vazio = usar o asset). */
    var jbmInitVersion: String
        get() = prefs.getString(KEY_JBM_INIT_VERSION, "") ?: ""
        set(value) = prefs.edit { putString(KEY_JBM_INIT_VERSION, value) }

    var jbmInitSha256: String
        get() = prefs.getString(KEY_JBM_INIT_SHA, "") ?: ""
        set(value) = prefs.edit { putString(KEY_JBM_INIT_SHA, value) }

    /** Perfil de dispositivo selecionado; vazio = detectar automaticamente. */
    var deviceProfileId: String
        get() = prefs.getString(KEY_DEVICE_PROFILE, "") ?: ""
        set(value) = prefs.edit { putString(KEY_DEVICE_PROFILE, value) }

    companion object {
        private const val KEY_THEME_MODE = "theme_mode"
        private const val KEY_DYNAMIC_COLOR = "dynamic_color"
        private const val KEY_CONFIG_ROOT = "config_root"
        private const val KEY_JBM_INIT_VERSION = "jbm_init_version"
        private const val KEY_JBM_INIT_SHA = "jbm_init_sha"
        private const val KEY_DEVICE_PROFILE = "device_profile"
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
