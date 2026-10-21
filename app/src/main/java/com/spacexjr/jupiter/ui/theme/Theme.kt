package com.spacexjr.jupiter.ui.theme

import android.os.Build
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.dynamicDarkColorScheme
import androidx.compose.material3.dynamicLightColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import com.spacexjr.jupiter.core.prefs.ThemeMode

// Paleta propria Jupiter: bordo, rosa e tons suaves.
private val JupiterLight = lightColorScheme(
    primary = Color(0xFF8E2A4E),
    onPrimary = Color(0xFFFFFFFF),
    primaryContainer = Color(0xFFFFD9E2),
    onPrimaryContainer = Color(0xFF3F0018),
    secondary = Color(0xFFB5436A),
    onSecondary = Color(0xFFFFFFFF),
    secondaryContainer = Color(0xFFFFD9E2),
    onSecondaryContainer = Color(0xFF3F0018),
    tertiary = Color(0xFF7C5635),
    onTertiary = Color(0xFFFFFFFF),
    tertiaryContainer = Color(0xFFFFDCC2),
    onTertiaryContainer = Color(0xFF2C1600),
    background = Color(0xFFFFF8F8),
    onBackground = Color(0xFF22191B),
    surface = Color(0xFFFFF8F8),
    onSurface = Color(0xFF22191B),
    surfaceVariant = Color(0xFFF3DDE2),
    onSurfaceVariant = Color(0xFF514347),
    outline = Color(0xFF847377),
    error = Color(0xFFBA1A1A),
    onError = Color(0xFFFFFFFF),
)

private val JupiterDark = darkColorScheme(
    primary = Color(0xFFFFB0C7),
    onPrimary = Color(0xFF55112F),
    primaryContainer = Color(0xFF722746),
    onPrimaryContainer = Color(0xFFFFD9E2),
    secondary = Color(0xFFFFB0C7),
    onSecondary = Color(0xFF55112F),
    secondaryContainer = Color(0xFF722746),
    onSecondaryContainer = Color(0xFFFFD9E2),
    tertiary = Color(0xFFEFBD94),
    onTertiary = Color(0xFF452B12),
    tertiaryContainer = Color(0xFF60411D),
    onTertiaryContainer = Color(0xFFFFDCC2),
    background = Color(0xFF1A1113),
    onBackground = Color(0xFFF0DEE0),
    surface = Color(0xFF1A1113),
    onSurface = Color(0xFFF0DEE0),
    surfaceVariant = Color(0xFF514347),
    onSurfaceVariant = Color(0xFFD7C1C6),
    outline = Color(0xFF9F8C90),
    error = Color(0xFFFFB4AB),
    onError = Color(0xFF690005),
)

@Composable
fun JupiterTheme(
    themeMode: ThemeMode = ThemeMode.SYSTEM,
    dynamicColor: Boolean = true,
    content: @Composable () -> Unit,
) {
    val darkTheme = when (themeMode) {
        ThemeMode.SYSTEM -> isSystemInDarkTheme()
        ThemeMode.LIGHT -> false
        ThemeMode.DARK -> true
    }
    val context = LocalContext.current
    val colorScheme = when {
        dynamicColor && Build.VERSION.SDK_INT >= Build.VERSION_CODES.S -> {
            if (darkTheme) dynamicDarkColorScheme(context) else dynamicLightColorScheme(context)
        }
        darkTheme -> JupiterDark
        else -> JupiterLight
    }
    MaterialTheme(colorScheme = colorScheme, content = content)
}
