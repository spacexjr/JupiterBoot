package com.spacexjr.jupiter.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
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
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.ChevronRight
import androidx.compose.material.icons.filled.FileDownload
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.RadioButtonChecked
import androidx.compose.material.icons.filled.RadioButtonUnchecked
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Card
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.RadioButton
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
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
import com.spacexjr.jupiter.core.patch.Confidence
import com.spacexjr.jupiter.core.patch.TargetKind
import com.spacexjr.jupiter.ui.JupiterViewModel
import com.spacexjr.jupiter.ui.components.InfoRow
import com.spacexjr.jupiter.ui.components.StatusTone
import java.io.File

object PatchScreen {
    const val ROUTE = "patch"
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun PatchScreen(vm: JupiterViewModel, onBack: () -> Unit) {
    val patch by vm.patch.collectAsStateWithLifecycle()
    var confirmRun by remember { mutableStateOf(false) }
    var exportTarget by remember { mutableStateOf<File?>(null) }

    val context = androidx.compose.ui.platform.LocalContext.current

    androidx.activity.compose.BackHandler(enabled = patch.busy) { /* bloqueia saída durante operação */ }

    androidx.compose.material3.Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Patch de boot") },
                navigationIcon = {
                    IconButton(onClick = onBack, enabled = !patch.busy) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Voltar")
                    }
                },
            )
        },
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .verticalScroll(rememberScrollState())
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(14.dp),
        ) {
            Text(
                "Fluxo guiado. O app nunca grava a partição: a imagem final é gerada no " +
                    "armazenamento privado e você decide quando exportá-la.",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )

            if (patch.busy) {
                Card {
                    Column(Modifier.padding(20.dp)) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            CircularProgressIndicator(modifier = Modifier.size(22.dp), strokeWidth = 2.dp)
                            Spacer(Modifier.size(14.dp))
                            Column {
                                Text("Executando", fontWeight = FontWeight.SemiBold)
                                Text(
                                    patch.stepLabel ?: "…",
                                    style = MaterialTheme.typography.bodySmall,
                                )
                            }
                        }
                        Spacer(Modifier.height(14.dp))
                        LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
                    }
                }
            }

            // Etapa 1: scan
            StepCard(
                number = 1,
                title = "Identificar a partição",
                subtitle = "Usa su para inspecionar by-name, slot ativo e escolher o alvo.",
            ) {
                OutlinedButton(
                    onClick = { vm.scanPartitions() },
                    enabled = !patch.busy,
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Icon(Icons.Filled.PlayArrow, contentDescription = null)
                    Spacer(Modifier.size(8.dp))
                    Text(if (patch.scan == null) "Escanear partições" else "Escanear novamente")
                }

                patch.scan?.let { scan ->
                    Spacer(Modifier.height(10.dp))
                    InfoRow("Slot ativo", scan.slotSuffix.ifEmpty { "(nenhum)" })
                    InfoRow("Dispositivo", "${scan.device} · ${scan.model}")
                    InfoRow("Plataforma", scan.platform.ifEmpty { "—" })
                    if (scan.partitions.isNotEmpty()) {
                        Spacer(Modifier.height(10.dp))
                        Text("Partições encontradas", fontWeight = FontWeight.SemiBold)
                        scan.partitions.forEach { p ->
                            val selected = scan.chosen?.partition?.name == p.name
                            Row(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .clickable(enabled = !patch.busy) { vm.selectTarget(p.name) }
                                    .padding(vertical = 6.dp),
                                verticalAlignment = Alignment.CenterVertically,
                            ) {
                                RadioButton(selected = selected, onClick = { vm.selectTarget(p.name) })
                                Column(Modifier.weight(1f)) {
                                    Text(p.name, fontWeight = FontWeight.Medium)
                                    Text(
                                        p.linkPath,
                                        style = MaterialTheme.typography.bodySmall,
                                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                                    )
                                }
                                Icon(Icons.Filled.ChevronRight, contentDescription = null)
                            }
                        }
                    }
                    scan.chosen?.let { chosen ->
                        Spacer(Modifier.height(8.dp))
                        HorizontalDivider()
                        Spacer(Modifier.height(8.dp))
                        Text(
                            "Alvo: ${chosen.partition.name} (${chosen.kind}) — confiança ${confidenceLabel(chosen.confidence)}",
                            fontWeight = FontWeight.SemiBold,
                        )
                        Text(
                            chosen.rationale,
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                        if (chosen.confidence != Confidence.HIGH) {
                            Text(
                                "Confiança não é alta: confirme no aparelho que é o alvo correto.",
                                style = MaterialTheme.typography.bodySmall,
                                color = MaterialTheme.colorScheme.tertiary,
                            )
                        }
                        if (chosen.kind == TargetKind.VENDOR_BOOT) {
                            Text(
                                "vendor_boot não é suportado como alvo do JupiterBoot.",
                                style = MaterialTheme.typography.bodySmall,
                                color = MaterialTheme.colorScheme.error,
                            )
                        }
                    }
                    scan.notes.filter { it.isNotBlank() }.forEach { note ->
                        Text(
                            "• $note",
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                    }
                }
            }

            // Etapa 2: executar
            StepCard(
                number = 2,
                title = "Executar o patch",
                subtitle = "dd → magiskboot unpack → /init → /init.system → repack verificado.",
            ) {
                androidx.compose.material3.Button(
                    onClick = { confirmRun = true },
                    enabled = !patch.busy && patch.scan?.chosen != null,
                    modifier = Modifier.fillMaxWidth(),
                ) {
                    Icon(Icons.Filled.PlayArrow, contentDescription = null)
                    Spacer(Modifier.size(8.dp))
                    Text("Preparar imagem patchada")
                }
                Text(
                    "Requer root. Nada é gravado na partição.",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            }

            // Relatório
            patch.displayReport?.let { report ->
                StepCard(
                    number = 3,
                    title = "Resultado",
                    subtitle = if (report.success) "Concluído e verificado." else "Falhou — veja o motivo.",
                ) {
                    InfoRow(
                        "Status",
                        if (report.success) "sucesso" else "falha em '${report.failedStep}'",
                    )
                    report.failureReason?.let {
                        Text(it, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.error)
                    }
                    report.image?.let {
                        InfoRow("Imagem original", vm.formatBytes(it.sizeBytes))
                        InfoRow("SHA-256 original", it.sha256.take(20) + "…")
                    }
                    report.repack?.let {
                        InfoRow("Imagem gerada", vm.formatBytes(it.outputSizeBytes))
                        InfoRow("SHA-256 gerada", it.outputSha256.take(20) + "…")
                        InfoRow("Tamanho preservado", if (it.sizeUnchanged) "sim" else "não")
                        InfoRow("Ramdisk confere", if (it.ramdiskMatchesStaged) "sim" else "não")
                        InfoRow("/init = jbm_init", if (it.initMatchesJbm) "sim" else "não")
                    }
                    report.warnings.forEach {
                        Text("• $it", style = MaterialTheme.typography.labelSmall, color = MaterialTheme.colorScheme.tertiary)
                    }
                    if (report.install != null) {
                        Spacer(Modifier.height(6.dp))
                        Text(
                            report.install.detail,
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                    }
                    if (patch.imageReady) {
                        Spacer(Modifier.height(10.dp))
                        androidx.compose.material3.Button(
                            onClick = {
                                exportTarget = File(
                                    context.getExternalFilesDir(null),
                                    "boot-jbm-${System.currentTimeMillis()}.img",
                                )
                            },
                            modifier = Modifier.fillMaxWidth(),
                        ) {
                            Icon(Icons.Filled.FileDownload, contentDescription = null)
                            Spacer(Modifier.size(8.dp))
                            Text("Exportar imagem patchada")
                        }
                    }
                }
            }

            patch.message?.let {
                Card {
                    Text(it, Modifier.padding(16.dp), color = MaterialTheme.colorScheme.primary)
                }
            }
            patch.error?.let {
                Card {
                    Text(it, Modifier.padding(16.dp), color = MaterialTheme.colorScheme.error)
                }
            }
        }
    }

    if (confirmRun) {
        val chosen = patch.scan?.chosen
        AlertDialog(
            onDismissRequest = { confirmRun = false },
            title = { Text("Confirmar patch") },
            text = {
                Text(
                    "Alvo: ${chosen?.partition?.name}\n\n" +
                        "O app vai extrair a partição, instalar o JupiterBoot como /init " +
                        "(preservando o original em /init.system) e gerar uma imagem " +
                        "verificada. Nenhuma partição será gravada.\n\n" +
                        "Uma imagem incorreta pode impedir o aparelho de iniciar. " +
                        "Mantenha uma imagem stock e o Odin disponíveis.",
                )
            },
            confirmButton = {
                TextButton(onClick = {
                    confirmRun = false
                    vm.runPatch()
                }) { Text("Preparar") }
            },
            dismissButton = {
                TextButton(onClick = { confirmRun = false }) { Text("Cancelar") }
            },
        )
    }

    exportTarget?.let { target ->
        AlertDialog(
            onDismissRequest = { exportTarget = null },
            title = { Text("Exportar imagem") },
            text = { Text("A imagem será salva em:\n\n${target.absolutePath}") },
            confirmButton = {
                TextButton(onClick = {
                    val t = target
                    exportTarget = null
                    vm.exportPatchedImage(t) { }
                    android.widget.Toast.makeText(context, "Exportação iniciada: ${t.name}", android.widget.Toast.LENGTH_LONG).show()
                }) { Text("Exportar") }
            },
            dismissButton = {
                TextButton(onClick = { exportTarget = null }) { Text("Cancelar") }
            },
        )
    }
}

@Composable
private fun StepCard(
    number: Int,
    title: String,
    subtitle: String,
    content: @Composable () -> Unit,
) {
    Card {
        Column(Modifier.padding(16.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                androidx.compose.material3.Surface(
                    shape = RoundedCornerShape(50),
                    color = MaterialTheme.colorScheme.secondaryContainer,
                    modifier = Modifier.size(28.dp),
                ) {
                    androidx.compose.foundation.layout.Box(contentAlignment = Alignment.Center) {
                        Text(
                            number.toString(),
                            fontWeight = FontWeight.Bold,
                            color = MaterialTheme.colorScheme.onSecondaryContainer,
                        )
                    }
                }
                Spacer(Modifier.size(12.dp))
                Column {
                    Text(title, fontWeight = FontWeight.SemiBold)
                    Text(
                        subtitle,
                        style = MaterialTheme.typography.bodySmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                    )
                }
            }
            Spacer(Modifier.height(12.dp))
            content()
        }
    }
}

private fun confidenceLabel(c: Confidence): String = when (c) {
    Confidence.HIGH -> "alta"
    Confidence.MEDIUM -> "média"
    Confidence.LOW -> "baixa"
}
