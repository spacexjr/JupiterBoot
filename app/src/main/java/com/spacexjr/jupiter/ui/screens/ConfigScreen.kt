package com.spacexjr.jupiter.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Restore
import androidx.compose.material.icons.filled.Save
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Card
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilterChip
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.spacexjr.jupiter.core.config.ConfigIni
import com.spacexjr.jupiter.core.prefs.ThemeMode
import com.spacexjr.jupiter.ui.JupiterViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ConfigScreen(vm: JupiterViewModel, onFiles: () -> Unit) {
    val state by vm.config.collectAsStateWithLifecycle()
    val profiles by vm.deviceProfiles.collectAsStateWithLifecycle()
    var showBackups by remember { mutableStateOf(false) }
    var infoDialog by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) {
        vm.loadConfig()
        vm.loadDeviceProfiles()
    }

    Column(Modifier.fillMaxSize()) {
        TopAppBar(
            title = { Text("Configurações") },
            actions = {
                IconButton(onClick = { showBackups = true }) {
                    Icon(Icons.Filled.Restore, contentDescription = "Backups")
                }
                IconButton(onClick = { vm.saveConfig() }) {
                    Icon(Icons.Filled.Save, contentDescription = "Salvar")
                }
            },
        )

        LazyColumn(
            modifier = Modifier.fillMaxSize(),
            contentPadding = androidx.compose.foundation.layout.PaddingValues(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Icon(Icons.Filled.Info, contentDescription = null, modifier = Modifier.size(20.dp))
                            Spacer(Modifier.size(8.dp))
                            Column {
                                Text("Raiz de configuração", fontWeight = FontWeight.SemiBold)
                                Text(
                                    if (vm.isLocalTestMode()) "Cópia de teste (armazenamento do app)"
                                    else "/cache/Jupiter (dispositivo, via su)",
                                    style = MaterialTheme.typography.bodySmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                )
                            }
                        }
                        Spacer(Modifier.height(8.dp))
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Switch(checked = vm.isLocalTestMode(), onCheckedChange = { vm.setLocalTestMode(it) })
                            Spacer(Modifier.size(8.dp))
                            Text("Usar cópia de teste local")
                        }
                        Text(
                            "A cópia de teste nunca toca em /cache. Use-a para editar arquivos sem root.",
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                    }
                }
            }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Text("Aparência do app", fontWeight = FontWeight.SemiBold)
                        Spacer(Modifier.height(8.dp))
                        Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            ThemeMode.entries.forEach { mode ->
                                FilterChip(
                                    selected = vm.themeMode() == mode,
                                    onClick = { vm.setThemeMode(mode) },
                                    label = { Text(mode.label) },
                                )
                            }
                        }
                        Spacer(Modifier.height(8.dp))
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Switch(
                                checked = vm.dynamicColor(),
                                onCheckedChange = { vm.setDynamicColor(it) },
                            )
                            Spacer(Modifier.size(8.dp))
                            Text("Cor dinâmica (Android 12+)")
                        }
                    }
                }
            }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Text("Perfil de dispositivo", fontWeight = FontWeight.SemiBold)
                        Text(
                            "O perfil só altera orientação e calibração de toque no config.ini. " +
                                "Valores automáticos (-1) não são sobrescritos.",
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                        Spacer(Modifier.height(8.dp))
                        LazyRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            items(profiles) { profile ->
                                FilterChip(
                                    selected = vm.selectedProfileId() == profile.id,
                                    onClick = { vm.applyDeviceProfile(profile.id) },
                                    label = { Text(profile.name) },
                                )
                            }
                        }
                        val detected = vm.detectedProfile()
                        if (detected != null) {
                            Text(
                                "Detectado: ${detected.name}",
                                style = MaterialTheme.typography.labelSmall,
                                color = MaterialTheme.colorScheme.primary,
                            )
                        }
                        state.message?.let {
                            Text(it, style = MaterialTheme.typography.labelSmall,
                                color = MaterialTheme.colorScheme.primary)
                        }
                    }
                }
            }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Text("JupiterBoot · config.ini", fontWeight = FontWeight.SemiBold)
                            Spacer(Modifier.weight(1f))
                            OutlinedButton(onClick = onFiles) {
                                Icon(Icons.Filled.Folder, contentDescription = null, modifier = Modifier.size(18.dp))
                                Spacer(Modifier.size(6.dp))
                                Text("Arquivos")
                            }
                        }
                        if (state.missing) {
                            Spacer(Modifier.height(8.dp))
                            Text(
                                "O arquivo ainda não existe no dispositivo. Você pode criar com os padrões " +
                                    "(sem sobrescrever um arquivo existente).",
                                style = MaterialTheme.typography.bodySmall,
                                color = MaterialTheme.colorScheme.onSurfaceVariant,
                            )
                            Spacer(Modifier.height(8.dp))
                            OutlinedButton(onClick = { vm.createConfigFromDefaults() }) {
                                Text("Criar config.ini com padrões")
                            }
                        }
                    }
                }
            }

            item { ConfigIntField("Timeout do menu (s)", state.config.menuTimeoutSec.toString()) { v -> vm.updateConfig { it.copy(menuTimeoutSec = v.toIntOrNull() ?: it.menuTimeoutSec) } } }
            item { ConfigIntField("Entrada padrão (-1 = usar flag default)", state.config.defaultEntry.toString()) { v -> vm.updateConfig { it.copy(defaultEntry = v.toIntOrNull() ?: it.defaultEntry) } } }
            item { ConfigTextRow("Tema", state.config.theme) { v -> vm.updateConfig { it.copy(theme = v) } } }
            item { ConfigTextRow("Background (caminho absoluto)", state.config.background) { v -> vm.updateConfig { it.copy(background = v) } } }
            item { ConfigIntField("Orientação (0/90/180/270, -1=auto)", state.config.orientation.toString()) { v -> vm.updateConfig { it.copy(orientation = v.toIntOrNull() ?: it.orientation) } } }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Text("Comportamento", fontWeight = FontWeight.SemiBold)
                        ToggleRow("Mostrar contagem regressiva", state.config.showCountdown) { v -> vm.updateConfig { it.copy(showCountdown = v) } }
                        ToggleRow("Mostrar marca", state.config.showBrand) { v -> vm.updateConfig { it.copy(showBrand = v) } }
                        ToggleRow("Preferir DRM", state.config.preferDrm) { v -> vm.updateConfig { it.copy(preferDrm = v) } }
                        ToggleRow("Fallback para framebuffer", state.config.fallbackToFb) { v -> vm.updateConfig { it.copy(fallbackToFb = v) } }
                        ToggleRow("Gravar log em arquivo", state.config.logFile) { v -> vm.updateConfig { it.copy(logFile = v) } }
                    }
                }
            }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Text("Layout e cores (sobrepõem o tema)", fontWeight = FontWeight.SemiBold)
                        Spacer(Modifier.height(8.dp))
                        ColorRow("#${
                            "%06x".format(state.config.colorBg and 0xFFFFFF)
                        }", "color_bg")
                        ColorRow("%06x".format(state.config.colorAccent and 0xFFFFFF), "color_accent")
                        ColorRow("%06x".format(state.config.colorText and 0xFFFFFF), "color_text")
                        Spacer(Modifier.height(8.dp))
                        Text(
                            "As métricas de botão (width/height/spacing/radius/text_size) são editadas em themes/*.ini.",
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                    }
                }
            }

            item {
                val issues = state.issues
                if (issues.isNotEmpty()) {
                    Card {
                        Column(Modifier.padding(16.dp)) {
                            Text("Validação", fontWeight = FontWeight.SemiBold)
                            issues.forEach {
                                Text(
                                    "• ${it.key}: ${it.message}",
                                    style = MaterialTheme.typography.bodySmall,
                                    color = MaterialTheme.colorScheme.error,
                                )
                            }
                        }
                    }
                }
            }

            item {
                if (state.dirty) {
                    Text(
                        "Há alterações não salvas. O salvamento cria um backup antes de gravar.",
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.tertiary,
                    )
                }
            }

            item {
                Text(
                    "O JupiterBoot lê config.ini, entries.ini, themes/*.ini e background.bmp. " +
                        "Alterar a raiz para a cópia de teste permite editar sem afetar o dispositivo.",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            }
        }
    }

    state.message?.let {
        androidx.compose.material3.Snackbar(modifier = Modifier.padding(16.dp)) { Text(it) }
    }
    state.error?.let {
        androidx.compose.material3.Snackbar(modifier = Modifier.padding(16.dp)) { Text(it) }
    }

    if (showBackups) {
        AlertDialog(
            onDismissRequest = { showBackups = false },
            title = { Text("Backups de config.ini") },
            text = {
                if (state.backups.isEmpty()) {
                    Text("Nenhum backup encontrado.")
                } else {
                    Column {
                        state.backups.forEach { name ->
                            TextButton(onClick = {
                                vm.restoreConfigBackup(name)
                                showBackups = false
                            }) { Text(name) }
                        }
                    }
                }
            },
            confirmButton = { TextButton(onClick = { showBackups = false }) { Text("Fechar") } },
        )
    }
}

@Composable
private fun ConfigIntField(label: String, value: String, onChange: (String) -> Unit) {
    OutlinedTextField(
        value = value,
        onValueChange = { new -> onChange(new.filter { it.isDigit() || it == '-' }.take(8)) },
        label = { Text(label) },
        singleLine = true,
        modifier = Modifier.fillMaxWidth(),
    )
}

@Composable
private fun ConfigTextRow(label: String, value: String, onChange: (String) -> Unit) {
    OutlinedTextField(
        value = value,
        onValueChange = onChange,
        label = { Text(label) },
        singleLine = true,
        modifier = Modifier.fillMaxWidth(),
    )
}

@Composable
private fun ToggleRow(label: String, value: Boolean, onChange: (Boolean) -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(label, modifier = Modifier.weight(1f))
        Switch(checked = value, onCheckedChange = onChange)
    }
}

@Composable
private fun ColorRow(hex: String, key: String) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 2.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(key, style = MaterialTheme.typography.bodySmall)
        Spacer(Modifier.weight(1f))
        Text(hex, style = MaterialTheme.typography.bodySmall, fontWeight = FontWeight.Medium)
    }
}
