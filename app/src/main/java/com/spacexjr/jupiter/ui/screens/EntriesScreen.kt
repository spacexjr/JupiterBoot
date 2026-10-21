package com.spacexjr.jupiter.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.ArrowDownward
import androidx.compose.material.icons.filled.ArrowUpward
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material.icons.filled.Restore
import androidx.compose.material.icons.filled.Save
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Card
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FloatingActionButton
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Snackbar
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
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.spacexjr.jupiter.core.config.EntryType
import com.spacexjr.jupiter.core.config.EntriesIniSerializer
import com.spacexjr.jupiter.core.config.JbmEntry
import com.spacexjr.jupiter.ui.JupiterViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun EntriesScreen(vm: JupiterViewModel) {
    val state by vm.entries.collectAsStateWithLifecycle()
    var editingIndex by remember { mutableStateOf<Int?>(null) }
    var previewText by remember { mutableStateOf<String?>(null) }

    LaunchedEffect(Unit) { vm.loadEntries() }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Entries") },
                actions = {
                    IconButton(onClick = { vm.restoreEntriesBackup() }) {
                        Icon(Icons.Filled.Restore, contentDescription = "Restaurar backup")
                    }
                    IconButton(onClick = { previewText = EntriesIniSerializer.write(state.entries) }) {
                        Icon(Icons.Filled.Edit, contentDescription = "Pré-visualizar")
                    }
                    IconButton(onClick = { vm.saveEntries() }) {
                        Icon(Icons.Filled.Save, contentDescription = "Salvar")
                    }
                },
            )
        },
        floatingActionButton = {
            FloatingActionButton(onClick = { vm.addEntry() }) {
                Icon(Icons.Filled.Add, contentDescription = "Adicionar entrada")
            }
        },
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding),
        ) {
            val source = state.source
            Column(Modifier.padding(horizontal = 16.dp, vertical = 8.dp)) {
                if (source != null) {
                    Text(
                        "Arquivo: $source/entries.ini",
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
                if (state.dirty) {
                    Text(
                        "Alterações não salvas",
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.tertiary,
                    )
                }
            }

            if (state.entries.isEmpty()) {
                Text(
                    "Nenhuma entrada. Toque em + para criar.",
                    modifier = Modifier.padding(16.dp),
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            } else {
                LazyColumn(
                    contentPadding = PaddingValues(16.dp),
                    verticalArrangement = Arrangement.spacedBy(10.dp),
                ) {
                    itemsIndexed(state.entries) { index, entry ->
                        Card {
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(14.dp),
                                verticalAlignment = Alignment.CenterVertically,
                            ) {
                                Column(Modifier.weight(1f)) {
                                    Row(verticalAlignment = Alignment.CenterVertically) {
                                        Text(
                                            entry.name.ifBlank { "(sem nome)" },
                                            fontWeight = FontWeight.SemiBold,
                                        )
                                        if (entry.isDefault) {
                                            Text(
                                                "  · padrão",
                                                style = MaterialTheme.typography.labelSmall,
                                                color = MaterialTheme.colorScheme.primary,
                                            )
                                        }
                                    }
                                    Text(
                                        "[${entry.id}] · ${entry.type.id} · order=${entry.order}" +
                                            if (entry.type == EntryType.SCRIPT || entry.type == EntryType.BINARY) {
                                                "\n${entry.path}"
                                            } else "",
                                        style = MaterialTheme.typography.bodySmall,
                                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                                    )
                                }
                                Switch(
                                    checked = entry.enabled,
                                    onCheckedChange = { v -> vm.updateEntry(index) { it.copy(enabled = v) } },
                                )
                            }
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(start = 4.dp, end = 8.dp, bottom = 6.dp),
                                horizontalArrangement = Arrangement.End,
                            ) {
                                IconButton(onClick = { vm.moveEntry(index, -1) }) {
                                    Icon(Icons.Filled.ArrowUpward, contentDescription = "Subir")
                                }
                                IconButton(onClick = { vm.moveEntry(index, 1) }) {
                                    Icon(Icons.Filled.ArrowDownward, contentDescription = "Descer")
                                }
                                IconButton(onClick = { editingIndex = index }) {
                                    Icon(Icons.Filled.Edit, contentDescription = "Editar")
                                }
                                IconButton(onClick = { vm.removeEntry(index) }) {
                                    Icon(Icons.Filled.Delete, contentDescription = "Remover")
                                }
                            }
                        }
                    }
                    item {
                        val issues = state.issues
                        if (issues.isNotEmpty()) {
                            Card {
                                Column(Modifier.padding(16.dp)) {
                                    Text("Validação", fontWeight = FontWeight.SemiBold)
                                    issues.forEach { issue ->
                                        Text(
                                            "• [${issue.entryId}] ${issue.message}" +
                                                if (!issue.fatal) " (aviso)" else "",
                                            style = MaterialTheme.typography.bodySmall,
                                            color = if (issue.fatal) MaterialTheme.colorScheme.error
                                            else MaterialTheme.colorScheme.tertiary,
                                        )
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    state.message?.let { msg ->
        Snackbar(
            modifier = Modifier.padding(16.dp),
            action = { TextButton(onClick = { vm.loadEntries() }) { Text("OK") } },
        ) { Text(msg) }
    }

    editingIndex?.let { index ->
        val entry = state.entries.getOrNull(index)
        if (entry != null) {
            EntryEditDialog(
                entry = entry,
                onDismiss = { editingIndex = null },
                onApply = { updated ->
                    vm.updateEntry(index) { updated }
                    editingIndex = null
                },
            )
        }
    }

    previewText?.let { text ->
        AlertDialog(
            onDismissRequest = { previewText = null },
            title = { Text("Pré-visualização de entries.ini") },
            text = {
                Box(
                    Modifier
                        .height(400.dp)
                        .background(MaterialTheme.colorScheme.surfaceVariant)
                        .verticalScroll(rememberScrollState()),
                ) {
                    Text(
                        text,
                        fontFamily = FontFamily.Monospace,
                        style = MaterialTheme.typography.bodySmall,
                        modifier = Modifier.padding(12.dp),
                    )
                }
            },
            confirmButton = {
                TextButton(onClick = { previewText = null }) { Text("Fechar") }
            },
        )
    }
}

@Composable
private fun EntryEditDialog(
    entry: JbmEntry,
    onDismiss: () -> Unit,
    onApply: (JbmEntry) -> Unit,
) {
    var id by remember { mutableStateOf(entry.id) }
    var name by remember { mutableStateOf(entry.name) }
    var type by remember { mutableStateOf(entry.type) }
    var path by remember { mutableStateOf(entry.path) }
    var args by remember { mutableStateOf(entry.args) }
    var isDefault by remember { mutableStateOf(entry.isDefault) }
    var behavior by remember { mutableStateOf(entry.behaviorAfter.toString()) }
    var typeMenu by remember { mutableStateOf(false) }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text("Editar entrada") },
        text = {
            Column(
                modifier = Modifier
                    .height(440.dp)
                    .verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                OutlinedTextField(
                    value = id,
                    onValueChange = { id = it },
                    label = { Text("id (seção)") },
                    singleLine = true,
                )
                OutlinedTextField(
                    value = name,
                    onValueChange = { name = it },
                    label = { Text("name") },
                    singleLine = true,
                )

                Box {
                    OutlinedTextField(
                        value = type.id,
                        onValueChange = {},
                        readOnly = true,
                        enabled = false,
                        label = { Text("type") },
                        modifier = Modifier.fillMaxWidth(),
                    )
                    Box(
                        Modifier
                            .fillMaxWidth()
                            .height(56.dp)
                            .clickable { typeMenu = true },
                    )
                    DropdownMenu(expanded = typeMenu, onDismissRequest = { typeMenu = false }) {
                        EntryType.entries.forEach { t ->
                            DropdownMenuItem(
                                text = { Text(t.id) },
                                onClick = { type = t; typeMenu = false },
                            )
                        }
                    }
                }

                if (type == EntryType.SCRIPT || type == EntryType.BINARY) {
                    OutlinedTextField(
                        value = path,
                        onValueChange = { path = it },
                        label = { Text("path (absoluto)") },
                        singleLine = true,
                    )
                    OutlinedTextField(
                        value = args,
                        onValueChange = { args = it },
                        label = { Text("args") },
                        singleLine = true,
                    )
                }

                OutlinedTextField(
                    value = behavior,
                    onValueChange = { behavior = it.filter { c -> c.isDigit() }.take(1) },
                    label = { Text("behavior_after (0=menu, 1=encadear init, 2=reboot)") },
                    singleLine = true,
                )

                Row(verticalAlignment = Alignment.CenterVertically) {
                    Switch(checked = isDefault, onCheckedChange = { isDefault = it })
                    Spacer(Modifier.size(8.dp))
                    Text("Entrada padrão")
                }
            }
        },
        confirmButton = {
            TextButton(onClick = {
                onApply(
                    entry.copy(
                        id = id,
                        name = name,
                        type = type,
                        path = path,
                        args = args,
                        isDefault = isDefault,
                        behaviorAfter = behavior.toIntOrNull() ?: 0,
                    ),
                )
            }) { Text("Aplicar") }
        },
        dismissButton = { TextButton(onClick = onDismiss) { Text("Cancelar") } },
    )
}
