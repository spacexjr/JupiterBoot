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
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Build
import androidx.compose.material.icons.filled.Folder
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material.icons.filled.Rocket
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.FilledTonalButton
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.spacexjr.jupiter.core.binaries.BinaryState
import com.spacexjr.jupiter.core.update.UpdateCheckResult
import com.spacexjr.jupiter.ui.JupiterViewModel
import com.spacexjr.jupiter.ui.components.InfoRow
import com.spacexjr.jupiter.ui.components.StatusCard
import com.spacexjr.jupiter.ui.components.StatusTone

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HomeScreen(
    vm: JupiterViewModel,
    onPatch: () -> Unit,
    onFiles: () -> Unit,
) {
    val home by vm.home.collectAsStateWithLifecycle()
    val patch by vm.patch.collectAsStateWithLifecycle()

    Column(modifier = Modifier.fillMaxSize()) {
        TopAppBar(
            title = {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Icon(Icons.Filled.Rocket, contentDescription = null)
                    Spacer(Modifier.size(12.dp))
                    Column {
                        Text("Jupiter", fontWeight = FontWeight.Bold)
                        Text(
                            "Gerenciador de boot · v${home.appVersion}",
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                    }
                }
            },
            actions = {
                androidx.compose.material3.IconButton(onClick = { vm.refreshAll() }) {
                    Icon(Icons.Filled.Refresh, contentDescription = "Atualizar")
                }
            },
            colors = TopAppBarDefaults.topAppBarColors(
                containerColor = MaterialTheme.colorScheme.surface,
            ),
        )

        LazyColumn(
            modifier = Modifier.fillMaxSize(),
            contentPadding = androidx.compose.foundation.layout.PaddingValues(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            item {
                Card(
                    colors = CardDefaults.cardColors(
                        containerColor = MaterialTheme.colorScheme.primaryContainer,
                    ),
                ) {
                    Column(Modifier.padding(20.dp)) {
                        Text(
                            "Patch de boot",
                            style = MaterialTheme.typography.titleLarge,
                            fontWeight = FontWeight.Bold,
                            color = MaterialTheme.colorScheme.onPrimaryContainer,
                        )
                        Spacer(Modifier.height(6.dp))
                        Text(
                            "Integra o JupiterBoot à imagem de boot: extrai a partição, " +
                                "instala o menu como /init e preserva o init original em /init.system.",
                            style = MaterialTheme.typography.bodyMedium,
                            color = MaterialTheme.colorScheme.onPrimaryContainer,
                        )
                        Spacer(Modifier.height(14.dp))
                        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                            androidx.compose.material3.Button(onClick = onPatch) {
                                Icon(Icons.Filled.Build, contentDescription = null)
                                Spacer(Modifier.size(8.dp))
                                Text("Abrir patch")
                            }
                            FilledTonalButton(onClick = onFiles) {
                                Icon(Icons.Filled.Folder, contentDescription = null)
                                Spacer(Modifier.size(8.dp))
                                Text("Arquivos")
                            }
                        }
                    }
                }
            }

            item { SectionTitle("Estado do sistema") }

            item {
                StatusCard(
                    title = "Acesso root",
                    value = when (home.rootAvailable) {
                        true -> "Disponível (su · uid=0)"
                        false -> "Indisponível"
                        null -> "Verificando…"
                    },
                    tone = when (home.rootAvailable) {
                        true -> StatusTone.OK
                        false -> StatusTone.ERROR
                        null -> StatusTone.UNKNOWN
                    },
                    subtitle = if (home.rootAvailable == false) {
                        "Operações de patch e /cache/Jupiter exigem root."
                    } else null,
                )
            }

            item {
                StatusCard(
                    title = "JupiterBoot",
                    value = home.jupiterBootRunning ?: "Não verificável",
                    tone = when {
                        home.jupiterBootRunning?.startsWith("Detectado") == true -> StatusTone.OK
                        home.jupiterBootRunning == null -> StatusTone.UNKNOWN
                        else -> StatusTone.WARN
                    },
                    subtitle = "Versão de binário detectada: ${home.jupiterBootVersion ?: "desconhecida"}",
                )
            }

            item {
                StatusCard(
                    title = "Imagem de boot",
                    value = home.bootImageState ?: "Verificando…",
                    tone = when {
                        home.bootImageState?.startsWith("Compatível") == true -> StatusTone.OK
                        home.bootImageState == null -> StatusTone.UNKNOWN
                        else -> StatusTone.WARN
                    },
                    subtitle = home.bootImageDetail,
                )
            }

            item {
                val summary = home.lastPatchSummary
                StatusCard(
                    title = "Último patch",
                    value = summary ?: "Nenhuma operação registrada",
                    tone = when {
                        summary == null -> StatusTone.UNKNOWN
                        summary.startsWith("Sucesso") -> StatusTone.OK
                        else -> StatusTone.ERROR
                    },
                )
            }

            item {
                StatusCard(
                    title = "Atualizações (GitHub)",
                    value = when (val u = home.update) {
                        is UpdateCheckResult.UpdateAvailable -> "Atualização disponível: ${u.remote}"
                        is UpdateCheckResult.UpToDate -> "Atualizado (${u.remote})"
                        is UpdateCheckResult.NoReleases -> "Nenhum release publicado"
                        is UpdateCheckResult.NetworkError -> "Erro de conexão"
                        is UpdateCheckResult.UnknownLocalVersion -> "Versão local desconhecida"
                        null -> "Consultando…"
                    },
                    tone = when (home.update) {
                        is UpdateCheckResult.UpdateAvailable -> StatusTone.INFO
                        is UpdateCheckResult.UpToDate -> StatusTone.OK
                        is UpdateCheckResult.NetworkError -> StatusTone.ERROR
                        is UpdateCheckResult.NoReleases -> StatusTone.WARN
                        else -> StatusTone.UNKNOWN
                    },
                    subtitle = (home.update as? UpdateCheckResult.UpdateAvailable)
                        ?.release?.publishedAt?.take(10),
                )
            }

            item { SectionTitle("Dispositivo") }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        InfoRow("Modelo", home.deviceModel ?: "—")
                        InfoRow("Codename", home.deviceCodename ?: "—")
                        InfoRow("Kernel", home.kernelVersion ?: "—")
                        InfoRow("Binários", vm.binariesSummary())
                    }
                }
            }

            item { SectionTitle("Binários incorporados") }

            if (home.binaries.isEmpty()) {
                item {
                    Text(
                        "Nenhum binário verificado ainda.",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
            } else {
                items(home.binaries) { status ->
                    Card {
                        Column(Modifier.padding(16.dp)) {
                            Row(verticalAlignment = Alignment.CenterVertically) {
                                Surface(
                                    shape = CircleShape,
                                    color = when (status.state) {
                                        BinaryState.OK, BinaryState.INSTALLED_NOW ->
                                            MaterialTheme.colorScheme.primary
                                        BinaryState.MODIFIED_LOCALLY -> MaterialTheme.colorScheme.tertiary
                                        else -> MaterialTheme.colorScheme.error
                                    },
                                    modifier = Modifier.size(36.dp),
                                ) {
                                    Icon(
                                        Icons.Filled.Build,
                                        contentDescription = null,
                                        tint = MaterialTheme.colorScheme.onPrimary,
                                        modifier = Modifier.padding(8.dp),
                                    )
                                }
                                Spacer(Modifier.size(12.dp))
                                Column(Modifier.fillMaxWidth()) {
                                    Text(status.spec.name, fontWeight = FontWeight.SemiBold)
                                    Text(
                                        "${status.spec.version} · ${status.spec.size} bytes",
                                        style = MaterialTheme.typography.bodySmall,
                                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                                    )
                                }
                            }
                            Spacer(Modifier.height(8.dp))
                            Text(
                                "Estado: ${statusLabel(status.state)}",
                                style = MaterialTheme.typography.bodySmall,
                            )
                            Text(
                                status.detail,
                                style = MaterialTheme.typography.bodySmall,
                                color = MaterialTheme.colorScheme.onSurfaceVariant,
                            )
                            status.elfDescription?.let {
                                Text(
                                    "ELF: $it",
                                    style = MaterialTheme.typography.bodySmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                )
                            }
                            status.spec.license.takeIf { it.isNotBlank() }?.let {
                                Spacer(Modifier.height(4.dp))
                                Text(
                                    "Licença: $it",
                                    style = MaterialTheme.typography.labelSmall,
                                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                                )
                            }
                            if (status.state != BinaryState.OK && status.state != BinaryState.INSTALLED_NOW) {
                                Spacer(Modifier.height(8.dp))
                                Text(
                                    "O app não substitui automaticamente arquivos divergentes. " +
                                        "Remova o arquivo em " + (status.path?.parent ?: "") +
                                        " para reinstalar na próxima abertura.",
                                    style = MaterialTheme.typography.labelSmall,
                                    color = MaterialTheme.colorScheme.error,
                                )
                            }
                        }
                    }
                }
            }

            item {
                Text(
                    "O Jupiter é uma ferramenta de desenvolvimento. Gravar uma imagem de boot " +
                        "incorreta pode impedir o aparelho de inicializar. Mantenha sempre uma " +
                        "imagem stock íntegra e um método de recuperação (por exemplo, Odin).",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            }
        }
    }
}

@Composable
private fun SectionTitle(text: String) {
    Text(
        text,
        style = MaterialTheme.typography.titleSmall,
        fontWeight = FontWeight.Bold,
        color = MaterialTheme.colorScheme.primary,
        modifier = Modifier.padding(top = 8.dp),
    )
}

private fun statusLabel(state: BinaryState): String = when (state) {
    BinaryState.OK -> "verificado"
    BinaryState.INSTALLED_NOW -> "instalado agora"
    BinaryState.MODIFIED_LOCALLY -> "modificado localmente (preservado)"
    BinaryState.ERROR -> "erro de validação"
    BinaryState.MISSING -> "ausente"
}
