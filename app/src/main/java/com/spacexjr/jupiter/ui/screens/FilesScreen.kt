package com.spacexjr.jupiter.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
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
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.ArrowForward
import androidx.compose.material.icons.filled.CreateNewFolder
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Description
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.Lock
import androidx.compose.material.icons.filled.Save
import androidx.compose.material.icons.filled.Terminal
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Card
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
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
import com.spacexjr.jupiter.core.store.StoreEntry
import com.spacexjr.jupiter.ui.JupiterViewModel

object FilesScreen {
    const val ROUTE = "files"
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun FilesScreen(vm: JupiterViewModel, onBack: () -> Unit) {
    val state by vm.files.collectAsStateWithLifecycle()
    var editing by remember { mutableStateOf<Pair<String, String>?>(null) }
    var creatingFolder by remember { mutableStateOf(false) }
    var newFolderName by remember { mutableStateOf("") }

    LaunchedEffect(Unit) {
        if (state.path.isEmpty() && state.entries.isEmpty()) vm.openDirectory("")
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Arquivos") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Voltar")
                    }
                },
                actions = {
                    IconButton(onClick = { creatingFolder = true }) {
                        Icon(Icons.Filled.CreateNewFolder, contentDescription = "Nova pasta")
                    }
                },
            )
        },
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding),
        ) {
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 16.dp, vertical = 8.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                if (state.path.isNotEmpty()) {
                    IconButton(onClick = { vm.openDirectory(state.path.substringBeforeLast('/', "")) }) {
                        Icon(Icons.Filled.ArrowForward, contentDescription = "Subir")
                    }
                }
                Text(
                    "Raiz: /" + state.path,
                    style = MaterialTheme.typography.labelMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    modifier = Modifier.weight(1f),
                )
            }

            LazyColumn(
                contentPadding = PaddingValues(16.dp),
                verticalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                if (state.entries.isEmpty()) {
                    item {
                        Text(
                            "Diretório vazio ou inacessível.",
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                    }
                }
                items(state.entries) { entry ->
                    FileRow(
                        entry = entry,
                        onClick = {
                            val child = if (state.path.isEmpty()) entry.name else "${state.path}/${entry.name}"
                            if (entry.isDirectory) vm.openDirectory(child) else vm.openFile(child)
                        },
                    )
                }
            }
        }
    }

    state.message?.let {
        androidx.compose.material3.Snackbar(modifier = Modifier.padding(16.dp)) { Text(it) }
    }
    state.error?.let {
        androidx.compose.material3.Snackbar(modifier = Modifier.padding(16.dp)) { Text(it) }
    }

    state.selectedPath?.let { path ->
        val isText = state.selectedContent != null
        if (isText) {
            editing = editing ?: (path to state.selectedContent!!)
        }
        AlertDialog(
            onDismissRequest = { editing = null; vm.openDirectory(state.path) },
            title = { Text(path.substringAfterLast('/')) },
            text = {
                Column {
                    Text(
                        state.selectedDetail ?: "",
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                    Spacer(Modifier.height(8.dp))
                    OutlinedTextField(
                        value = editing?.second ?: state.selectedContent.orEmpty(),
                        onValueChange = { editing = path to it },
                        modifier = Modifier
                            .height(360.dp)
                            .fillMaxWidth(),
                        textStyle = MaterialTheme.typography.bodySmall.copy(fontFamily = FontFamily.Monospace),
                    )
                }
            },
            confirmButton = {
                TextButton(onClick = {
                    val (p, content) = editing ?: (path to state.selectedContent.orEmpty())
                    vm.saveFile(p, content)
                    editing = null
                }) { Text("Salvar") }
            },
            dismissButton = {
                Row {
                    TextButton(onClick = {
                        vm.chmodExecutable(path)
                        editing = null
                    }) { Text("+x") }
                    TextButton(onClick = {
                        vm.deleteFile(path)
                        editing = null
                    }) { Text("Excluir") }
                    TextButton(onClick = {
                        editing = null
                    }) { Text("Fechar") }
                }
            },
        )
    }

    if (creatingFolder) {
        AlertDialog(
            onDismissRequest = { creatingFolder = false; newFolderName = "" },
            title = { Text("Nova pasta") },
            text = {
                OutlinedTextField(
                    value = newFolderName,
                    onValueChange = { newFolderName = it },
                    label = { Text("nome") },
                    singleLine = true,
                )
            },
            confirmButton = {
                TextButton(onClick = {
                    val name = newFolderName.trim()
                    val base = state.path
                    if (name.isNotEmpty()) {
                        val target = if (base.isEmpty()) name else "$base/$name"
                        vm.makeDirs(target)
                    }
                    creatingFolder = false
                    newFolderName = ""
                }) { Text("Criar") }
            },
            dismissButton = {
                TextButton(onClick = { creatingFolder = false; newFolderName = "" }) { Text("Cancelar") }
            },
        )
    }
}

@Composable
private fun FileRow(entry: StoreEntry, onClick: () -> Unit) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .clickable(onClick = onClick),
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(14.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Icon(
                if (entry.isDirectory) Icons.Filled.Folder else iconFor(entry.name),
                contentDescription = null,
                tint = MaterialTheme.colorScheme.primary,
            )
            Spacer(Modifier.size(12.dp))
            Column(Modifier.weight(1f)) {
                Text(entry.name, fontWeight = FontWeight.Medium)
            }
        }
    }
}

private fun iconFor(name: String) = when {
    name.endsWith(".sh") -> Icons.Filled.Terminal
    name.endsWith(".ini") || name.endsWith(".bmp") -> Icons.Filled.Description
    else -> Icons.Filled.Description
}
