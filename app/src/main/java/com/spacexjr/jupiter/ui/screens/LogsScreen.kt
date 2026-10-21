package com.spacexjr.jupiter.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Clear
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.FileDownload
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material3.Card
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilterChip
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalClipboardManager
import androidx.compose.ui.text.AnnotatedString
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.spacexjr.jupiter.core.log.LogLevel
import com.spacexjr.jupiter.core.log.LogSource
import com.spacexjr.jupiter.ui.JupiterViewModel
import com.spacexjr.jupiter.ui.applyFilter
import java.io.File

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LogsScreen(vm: JupiterViewModel) {
    val logs by vm.logs.collectAsStateWithLifecycle()
    val filter by vm.logFilter.collectAsStateWithLifecycle()
    val clipboard = LocalClipboardManager.current
    val context = androidx.compose.ui.platform.LocalContext.current

    val filtered = logs.applyFilter(filter)

    Column(Modifier.fillMaxSize()) {
        TopAppBar(
            title = { Text("Logs") },
            actions = {
                IconButton(onClick = { vm.clearLogs() }) {
                    Icon(Icons.Filled.Clear, contentDescription = "Limpar")
                }
                IconButton(onClick = { vm.refreshLogs() }) {
                    Icon(Icons.Filled.Refresh, contentDescription = "Atualizar")
                }
                IconButton(onClick = {
                    val target = File(context.getExternalFilesDir(null), "jupiter-logs.txt")
                    vm.exportLogs(target, filtered) {
                        android.widget.Toast.makeText(context, it, android.widget.Toast.LENGTH_LONG).show()
                    }
                }) {
                    Icon(Icons.Filled.FileDownload, contentDescription = "Exportar")
                }
            },
        )

        Column(Modifier.padding(horizontal = 16.dp)) {
            OutlinedTextField(
                value = filter.query,
                onValueChange = { q -> vm.setLogFilter { it.copy(query = q) } },
                label = { Text("Pesquisar") },
                modifier = Modifier.fillMaxWidth(),
                singleLine = true,
            )
            Spacer(Modifier.size(8.dp))
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .horizontalScroll(rememberScrollState()),
                horizontalArrangement = Arrangement.spacedBy(6.dp),
            ) {
                LogLevel.entries.forEach { level ->
                    FilterChip(
                        selected = level in filter.levels,
                        onClick = {
                            vm.setLogFilter {
                                it.copy(
                                    levels = if (level in it.levels) it.levels - level else it.levels + level,
                                )
                            }
                        },
                        label = { Text(level.name) },
                    )
                }
            }
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .horizontalScroll(rememberScrollState()),
                horizontalArrangement = Arrangement.spacedBy(6.dp),
            ) {
                LogSource.entries.forEach { source ->
                    FilterChip(
                        selected = source in filter.sources,
                        onClick = {
                            vm.setLogFilter {
                                it.copy(
                                    sources = if (source in it.sources) it.sources - source else it.sources + source,
                                )
                            }
                        },
                        label = { Text(source.label) },
                    )
                }
            }
        }

        if (filtered.isEmpty()) {
            Text(
                "Nenhum registro para os filtros atuais.",
                modifier = Modifier.padding(16.dp),
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
        } else {
            LazyColumn(
                contentPadding = PaddingValues(16.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                items(filtered) { entry ->
                    Card {
                        Column(Modifier.padding(12.dp)) {
                            Row(Modifier.fillMaxWidth()) {
                                Text(
                                    entry.level.name,
                                    fontWeight = FontWeight.Bold,
                                    style = MaterialTheme.typography.labelMedium,
                                    color = when (entry.level) {
                                        LogLevel.ERROR -> MaterialTheme.colorScheme.error
                                        LogLevel.WARN -> MaterialTheme.colorScheme.tertiary
                                        else -> MaterialTheme.colorScheme.primary
                                    },
                                )
                                Spacer(Modifier.size(8.dp))
                                Text(
                                    entry.source.label,
                                    style = MaterialTheme.typography.labelMedium,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                )
                                Spacer(Modifier.weight(1f))
                                Text(
                                    entry.formattedTime(),
                                    style = MaterialTheme.typography.labelSmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                )
                                IconButton(
                                    onClick = {
                                        clipboard.setText(
                                            AnnotatedString(
                                                buildString {
                                                    appendLine(entry.message)
                                                    entry.stdout?.let { appendLine("stdout:\n$it") }
                                                    entry.stderr?.let { appendLine("stderr:\n$it") }
                                                    entry.exitCode?.let { appendLine("exit: $it") }
                                                },
                                            ),
                                        )
                                    },
                                    modifier = Modifier.size(28.dp),
                                ) {
                                    Icon(Icons.Filled.ContentCopy, contentDescription = "Copiar", modifier = Modifier.size(16.dp))
                                }
                            }
                            Text(
                                entry.message,
                                style = MaterialTheme.typography.bodySmall,
                                fontFamily = FontFamily.Monospace,
                            )
                            entry.exitCode?.let {
                                Text(
                                    "exit=$it",
                                    style = MaterialTheme.typography.labelSmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                )
                            }
                            entry.stdout?.takeIf { it.isNotBlank() }?.let {
                                Text(
                                    it.take(2000),
                                    style = MaterialTheme.typography.labelSmall,
                                    fontFamily = FontFamily.Monospace,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                    modifier = Modifier
                                        .padding(top = 4.dp)
                                        .background(MaterialTheme.colorScheme.surfaceVariant)
                                        .padding(6.dp),
                                )
                            }
                            entry.stderr?.takeIf { it.isNotBlank() }?.let {
                                Text(
                                    it.take(2000),
                                    style = MaterialTheme.typography.labelSmall,
                                    fontFamily = FontFamily.Monospace,
                                    color = MaterialTheme.colorScheme.error,
                                    modifier = Modifier
                                        .padding(top = 4.dp)
                                        .background(MaterialTheme.colorScheme.surfaceVariant)
                                        .padding(6.dp),
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}
