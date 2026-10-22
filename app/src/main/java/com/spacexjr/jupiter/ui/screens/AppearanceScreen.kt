package com.spacexjr.jupiter.ui.screens

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.Image as ImageView
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Image
import androidx.compose.material.icons.filled.Save
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.FilterChip
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.asImageBitmap
import androidx.compose.ui.layout.ContentScale
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.spacexjr.jupiter.core.config.ButtonStyle
import com.spacexjr.jupiter.core.config.ConfigIni
import com.spacexjr.jupiter.core.config.EffectiveButtonStyle
import com.spacexjr.jupiter.core.config.EffectiveMenuStyle
import com.spacexjr.jupiter.core.config.JbmEntry
import com.spacexjr.jupiter.core.config.MenuGeometry
import com.spacexjr.jupiter.core.config.TextAlign as MenuAlign
import com.spacexjr.jupiter.core.config.ThemeIni
import com.spacexjr.jupiter.core.image.BmpWriter
import com.spacexjr.jupiter.ui.JupiterViewModel
import java.io.File

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun AppearanceScreen(vm: JupiterViewModel) {
    val configState by vm.config.collectAsStateWithLifecycle()
    val entriesState by vm.entries.collectAsStateWithLifecycle()
    val appearance by vm.appearance.collectAsStateWithLifecycle()
    val context = LocalContext.current

    var showNewTheme by remember { mutableStateOf(false) }
    var newThemeName by remember { mutableStateOf("") }
    var editIndex by remember { mutableIntStateOf(-1) }
    var editorImage by remember { mutableStateOf<Bitmap?>(null) }
    var editorName by remember { mutableStateOf("") }
    var editorZoom by remember { mutableFloatStateOf(1f) }
    var editorOffsetX by remember { mutableFloatStateOf(0f) }
    var editorOffsetY by remember { mutableFloatStateOf(0f) }
    var editorRotation by remember { mutableIntStateOf(0) }

    LaunchedEffect(Unit) {
        vm.loadConfig()
        vm.loadEntries()
        vm.loadAppearance()
    }

    val imagePicker = rememberLauncherForActivityResult(ActivityResultContracts.GetContent()) { uri: Uri? ->
        if (uri != null) {
            val decoded = context.contentResolver.openInputStream(uri)?.use(BitmapFactory::decodeStream)
            if (decoded != null) {
                editorImage = decoded
                editorName = "imagem selecionada"
                editorZoom = 1f
                editorOffsetX = 0f
                editorOffsetY = 0f
                editorRotation = 0
            }
        }
    }

    val cfg = configState.config
    val theme = ThemeIni.resolve(appearance.selectedName, appearance.themes)
    val menu = EffectiveMenuStyle.resolve(cfg, theme)
    val entries = entriesState.entries

    Column(Modifier.fillMaxSize()) {
        TopAppBar(
            title = { Text("Aparência") },
            actions = {
                IconButton(onClick = {
                    vm.saveTheme()
                    vm.saveConfig()
                    vm.saveEntries()
                }) {
                    Icon(Icons.Filled.Save, contentDescription = "Salvar aparência")
                }
                IconButton(onClick = { imagePicker.launch("image/*") }) {
                    Icon(Icons.Filled.Image, contentDescription = "Trocar imagem de fundo")
                }
            },
        )

        LazyColumn(
            modifier = Modifier.fillMaxSize(),
            contentPadding = PaddingValues(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp),
        ) {
            item {
                Card {
                    Column(Modifier.padding(12.dp)) {
                        Text("Pré-visualização", fontWeight = FontWeight.SemiBold)
                        Text(
                            "Somente desenho: nenhuma ação de boot é executada aqui.",
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                        Spacer(Modifier.height(10.dp))
                        MenuPreview(cfg, menu, entries, appearance.backgroundPreview)
                    }
                }
            }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Text("Identidade", fontWeight = FontWeight.SemiBold)
                        OutlinedTextField(
                            value = cfg.title,
                            onValueChange = { v -> vm.updateConfig { it.copy(title = v.take(63)) } },
                            label = { Text("Título") },
                            modifier = Modifier.fillMaxWidth(),
                            singleLine = true,
                        )
                        OutlinedTextField(
                            value = cfg.subtitle,
                            onValueChange = { v -> vm.updateConfig { it.copy(subtitle = v.take(127)) } },
                            label = { Text("Subtítulo") },
                            modifier = Modifier.fillMaxWidth(),
                            singleLine = true,
                        )
                        ToggleLine("Mostrar título", cfg.showTitle) { v ->
                            vm.updateTheme { it.copy(showTitle = v) }
                        }
                    }
                }
            }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Text("Botões", fontWeight = FontWeight.SemiBold)
                        StyleChips(menu.buttonStyle) { v ->
                            vm.updateTheme { it.copy(buttonStyle = v) }
                        }
                        SliderRow("Largura", menu.buttonWidth.toFloat(), 120f, 720f) { v ->
                            vm.updateTheme { it.copy(buttonWidth = v.toInt()) }
                        }
                        SliderRow("Altura", menu.buttonHeight.toFloat(), 48f, 240f) { v ->
                            vm.updateTheme { it.copy(buttonHeight = v.toInt()) }
                        }
                        SliderRow("Espaçamento", menu.buttonSpacing.toFloat(), 0f, 96f) { v ->
                            vm.updateTheme { it.copy(buttonSpacing = v.toInt()) }
                        }
                        SliderRow("Raio", menu.buttonRadius.toFloat(), 0f, 80f) { v ->
                            vm.updateTheme { it.copy(buttonRadius = v.toInt()) }
                        }
                        SliderRow("Borda", menu.buttonBorderWidth.toFloat(), 0f, 8f) { v ->
                            vm.updateTheme { it.copy(buttonBorderWidth = v.toInt()) }
                        }
                        SliderRow("Opacidade", menu.buttonAlpha.toFloat(), 0f, 255f) { v ->
                            vm.updateTheme { it.copy(buttonAlpha = v.toInt()) }
                        }
                        SliderRow("Texto", menu.textSize.toFloat(), 10f, 48f) { v ->
                            vm.updateTheme { it.copy(textSize = v.toInt()) }
                        }
                        SliderRow("Ícone", menu.iconScale.toFloat(), 50f, 200f) { v ->
                            vm.updateTheme { it.copy(iconScale = v.toInt()) }
                        }
                        AlignChips(menu.textAlign) { v ->
                            vm.updateTheme { it.copy(textAlign = v) }
                        }
                        ToggleLine("Mostrar descrições", menu.showDescription) { v ->
                            vm.updateTheme { it.copy(showDescription = v) }
                        }
                        ToggleLine("Mostrar ícones", menu.showIcons) { v ->
                            vm.updateTheme { it.copy(showIcons = v) }
                        }
                    }
                }
            }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Text("Cores", fontWeight = FontWeight.SemiBold)
                        ColorRow("Fundo da tela", menu.colorBg) { c ->
                            vm.updateTheme { it.copy(colorBg = c) }
                        }
                        ColorRow("Acento", menu.colorAccent) { c ->
                            vm.updateTheme { it.copy(colorAccent = c) }
                        }
                        ColorRow("Selecionado", menu.colorSelectedBg) { c ->
                            vm.updateTheme { it.copy(colorSelectedBg = c) }
                        }
                        ColorRow("Pressionado", menu.colorPressedBg) { c ->
                            vm.updateTheme { it.copy(colorPressedBg = c) }
                        }
                        ColorRow("Texto", menu.colorText) { c ->
                            vm.updateTheme { it.copy(colorText = c) }
                        }
                        ColorRow("Texto selecionado", menu.colorTextSel) { c ->
                            vm.updateTheme { it.copy(colorTextSel = c) }
                        }
                        ColorRow("Borda", menu.colorBorder) { c ->
                            vm.updateTheme { it.copy(colorBorder = c) }
                        }
                    }
                }
            }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Text("Botões individuais", fontWeight = FontWeight.SemiBold)
                            Spacer(Modifier.weight(1f))
                            Text("${entries.size} entradas", style = MaterialTheme.typography.labelSmall)
                        }
                        entries.take(8).forEachIndexed { index, entry ->
                            val eff = EffectiveButtonStyle.resolve(entry, menu)
                            Row(
                                Modifier.fillMaxWidth().padding(vertical = 4.dp),
                                verticalAlignment = Alignment.CenterVertically,
                            ) {
                                Column(Modifier.weight(1f)) {
                                    Text(entry.name, style = MaterialTheme.typography.bodyMedium)
                                    Text(
                                        "${ButtonStyle.label(eff.style)} · ${eff.width}×${eff.height}",
                                        style = MaterialTheme.typography.labelSmall,
                                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                                    )
                                }
                                OutlinedButton(onClick = { editIndex = index }) { Text("Editar") }
                            }
                        }
                    }
                }
            }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Text("Fundo", fontWeight = FontWeight.SemiBold)
                        Text(
                            if (appearance.backgroundPreview != null) "Imagem + cor de fundo" else "Cor sólida",
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                        Spacer(Modifier.height(8.dp))
                        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                            OutlinedButton(onClick = { imagePicker.launch("image/*") }) {
                                Icon(Icons.Filled.Image, contentDescription = null, modifier = Modifier.size(18.dp))
                                Spacer(Modifier.size(6.dp))
                                Text("Escolher imagem")
                            }
                            if (appearance.backgroundPreview != null) {
                                OutlinedButton(onClick = { vm.loadAppearance() }) { Text("Recarregar") }
                            }
                        }
                        if (appearance.backgroundPreview != null) {
                            Spacer(Modifier.height(8.dp))
                            ImageView(
                                bitmap = appearance.backgroundPreview!!.asImageBitmap(),
                                contentDescription = "Fundo atual",
                                modifier = Modifier.fillMaxWidth().aspectRatio(720f / 1600f)
                                    .clip(RoundedCornerShape(8.dp)),
                                contentScale = ContentScale.Crop,
                            )
                        }
                        Text(
                            "O renderizador aceita BMP 24 bits; a imagem é recortada e exportada como background.bmp.",
                            style = MaterialTheme.typography.labelSmall,
                            color = MaterialTheme.colorScheme.onSurfaceVariant,
                        )
                    }
                }
            }

            item {
                Card {
                    Column(Modifier.padding(16.dp)) {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Text("Tema", fontWeight = FontWeight.SemiBold)
                            Spacer(Modifier.weight(1f))
                            IconButton(onClick = { showNewTheme = true }) {
                                Icon(Icons.Filled.Add, contentDescription = "Novo tema")
                            }
                            IconButton(
                                onClick = { vm.deleteTheme(appearance.selectedName) },
                                enabled = appearance.selectedName != cfg.theme,
                            ) {
                                Icon(Icons.Filled.Delete, contentDescription = "Apagar tema")
                            }
                        }
                        LazyRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                            items(appearance.themes.keys.sorted()) { name ->
                                FilterChip(
                                    selected = appearance.selectedName == name,
                                    onClick = { vm.selectTheme(name) },
                                    label = { Text(name) },
                                )
                            }
                        }
                        OutlinedTextField(
                            value = theme.extends,
                            onValueChange = { v -> vm.updateTheme { it.copy(extends = v.trim().take(63)) } },
                            label = { Text("Herda de (extends)") },
                            modifier = Modifier.fillMaxWidth(),
                            singleLine = true,
                        )
                        Spacer(Modifier.height(8.dp))
                        Text(
                            "Tema selecionado: ${appearance.selectedName}${if (appearance.dirty) " · não salvo" else ""}",
                            style = MaterialTheme.typography.bodySmall,
                        )
                        appearance.message?.let { Text(it, color = MaterialTheme.colorScheme.primary) }
                        appearance.error?.let { Text(it, color = MaterialTheme.colorScheme.error) }
                        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                            Button(onClick = { vm.saveTheme() }) { Text("Salvar tema") }
                            OutlinedButton(onClick = { vm.saveConfig() }) { Text("Salvar config.ini") }
                        }
                    }
                }
            }

            configState.error?.let { error ->
                item { Text(error, color = MaterialTheme.colorScheme.error) }
            }
            entriesState.error?.let { error ->
                item { Text(error, color = MaterialTheme.colorScheme.error) }
            }
        }
    }

    if (showNewTheme) {
        AlertDialog(
            onDismissRequest = { showNewTheme = false },
            title = { Text("Novo tema") },
            text = {
                OutlinedTextField(
                    value = newThemeName,
                    onValueChange = { newThemeName = it },
                    label = { Text("Nome do arquivo") },
                    singleLine = true,
                )
            },
            confirmButton = {
                TextButton(onClick = {
                    vm.createTheme(newThemeName)
                    newThemeName = ""
                    showNewTheme = false
                }) { Text("Criar") }
            },
            dismissButton = {
                TextButton(onClick = { showNewTheme = false }) { Text("Cancelar") }
            },
        )
    }

    if (editIndex >= 0 && editIndex < entries.size) {
        EntryAppearanceDialog(
            entry = entries[editIndex],
            menu = menu,
            onDismiss = { editIndex = -1 },
            onChange = { updated ->
                vm.updateEntry(editIndex) { updated }
            },
            onSave = {
                vm.saveEntries()
                editIndex = -1
            },
        )
    }

    editorImage?.let { bitmap ->
        ImageEditorDialog(
            bitmap = bitmap,
            name = editorName,
            zoom = editorZoom,
            offsetX = editorOffsetX,
            offsetY = editorOffsetY,
            rotation = editorRotation,
            onZoom = { editorZoom = it },
            onOffsetX = { editorOffsetX = it },
            onOffsetY = { editorOffsetY = it },
            onRotation = { editorRotation = it },
            onDismiss = { editorImage = null },
            onExport = {
                val out = File(context.cacheDir, "background.bmp")
                val (tw, th) = BmpWriter.targetSize(720, 1600)
                val ok = BmpWriter.cropToBmp(bitmap, editorZoom, editorOffsetX, editorOffsetY,
                    editorRotation, tw, th, out)
                if (ok) vm.importBackground(out)
                editorImage = null
            },
        )
    }
}

@Composable
private fun MenuPreview(
    cfg: ConfigIni,
    menu: EffectiveMenuStyle,
    entries: List<JbmEntry>,
    background: Bitmap?,
) {
    BoxWithConstraints(
        Modifier.fillMaxWidth(),
        contentAlignment = Alignment.Center,
    ) {
        val previewHeight = (maxWidth.value * 1600f / 720f).coerceAtMost(520f)
        val previewWidth = previewHeight * 720f / 1600f
        val k = previewWidth / 720f
        Box(
            Modifier.width(previewWidth.dp).height(previewHeight.dp)
                .clip(RoundedCornerShape(10.dp))
                .background(argb(menu.colorBg)),
        ) {
            if (background != null) {
                ImageView(
                    bitmap = background.asImageBitmap(),
                    contentDescription = null,
                    modifier = Modifier.fillMaxSize(),
                    contentScale = ContentScale.Crop,
                )
            }
            if (!menu.showTitle || !cfg.showBrand) {
                Spacer(Modifier.height((1600f * 0.09f * k).dp))
            } else {
                Column(Modifier.padding(top = (1600f * 0.09f * k).dp, start = (720f * menu.layoutMargin / 100f * k).dp)) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Box(
                            Modifier.size((38f * k).dp)
                                .clip(RoundedCornerShape(50))
                                .border((2f * k).dp, argb(menu.colorBorder), RoundedCornerShape(50)),
                        )
                        Spacer(Modifier.width((14f * k).dp))
                        Text(cfg.title, color = Color(0xFFEEEEEE), fontSize = (28f * k).sp,
                            fontWeight = FontWeight.SemiBold)
                        Text(cfg.subtitle, color = argb(menu.colorAccent), fontSize = (28f * k).sp)
                    }
                    Spacer(Modifier.height((5f * k).dp))
                }
            }
            Column(
                Modifier.padding(
                    start = (720f * menu.layoutMargin / 100f * k).dp,
                    end = (720f * menu.layoutMargin / 100f * k).dp,
                    top = (45f * k).dp,
                ),
                verticalArrangement = Arrangement.spacedBy((menu.buttonSpacing * k).dp),
            ) {
                entries.forEachIndexed { index, entry ->
                    val eff = EffectiveButtonStyle.resolve(entry, menu)
                    val selected = index == 0
                    val shape = RoundedCornerShape((eff.radius * k).dp)
                    val bg = if (selected) argb(eff.colorBg).copy(alpha = menu.buttonAlpha / 255f)
                    else argb(eff.colorBg).copy(alpha = 0.06f)
                    val borderColor = if (selected) argb(eff.colorBorder) else argb(eff.colorBorder).copy(alpha = 0.35f)
                    val width = (eff.width * k).coerceAtMost(720f * k - 2 * (720f * menu.layoutMargin / 100f * k))
                    Row(
                        Modifier.width(width.dp)
                            .height((eff.height * k).dp)
                            .clip(shape)
                            .background(
                                if (selected && eff.style != ButtonStyle.OUTLINE)
                                    Brush.horizontalGradient(listOf(bg, bg.copy(alpha = bg.alpha / 4f)))
                                else Brush.horizontalGradient(listOf(bg, bg)),
                            )
                            .border((eff.border.coerceAtLeast(1) * k).dp, borderColor, shape)
                            .padding(horizontal = (14f * k).dp),
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        if (eff.showIcon) {
                            Box(
                                Modifier.size((eff.height * k * 0.3f).dp)
                                    .clip(RoundedCornerShape(50))
                                    .background(argb(if (selected) eff.colorTextSel else eff.colorText).copy(alpha = 0.5f)),
                            )
                            Spacer(Modifier.width((10f * k).dp))
                        }
                        Column(Modifier.weight(1f)) {
                            Text(
                                entry.name,
                                color = argb(if (selected) eff.colorTextSel else eff.colorText),
                                fontSize = (menu.textSize * k).sp,
                                textAlign = when (eff.textAlign) {
                                    MenuAlign.CENTER -> TextAlign.Center
                                    MenuAlign.RIGHT -> TextAlign.End
                                    else -> TextAlign.Start
                                },
                            )
                            if (menu.showDescription && entry.description.isNotEmpty()) {
                                Text(
                                    entry.description,
                                    color = argb(if (selected) eff.colorTextSel else eff.colorText).copy(alpha = 0.7f),
                                    fontSize = (menu.textSize * 0.72f * k).sp,
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun EntryAppearanceDialog(
    entry: JbmEntry,
    menu: EffectiveMenuStyle,
    onDismiss: () -> Unit,
    onChange: (JbmEntry) -> Unit,
    onSave: () -> Unit,
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text("Aparência · ${entry.name}") },
        text = {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                OutlinedTextField(
                    value = entry.description,
                    onValueChange = { v -> onChange(entry.copy(description = v.take(95))) },
                    label = { Text("Descrição") },
                    modifier = Modifier.fillMaxWidth(),
                )
                OutlinedTextField(
                    value = entry.icon,
                    onValueChange = { v -> onChange(entry.copy(icon = v.take(63))) },
                    label = { Text("Ícone (system, recovery, fastboot…)") },
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true,
                )
                StyleChips(if (entry.buttonStyle >= 0) entry.buttonStyle else menu.buttonStyle) { v ->
                    onChange(entry.copy(buttonStyle = v))
                }
                TextButton(onClick = { onChange(entry.copy(buttonStyle = -1)) }) { Text("Herdar estilo do tema") }
                AlignChips(if (entry.textAlign >= 0) entry.textAlign else menu.textAlign) { v ->
                    onChange(entry.copy(textAlign = v))
                }
                SliderRow("Largura", (if (entry.buttonWidth > 0) entry.buttonWidth else menu.buttonWidth).toFloat(),
                    80f, 720f) { v -> onChange(entry.copy(buttonWidth = v.toInt())) }
                SliderRow("Altura", (if (entry.buttonHeight > 0) entry.buttonHeight else menu.buttonHeight).toFloat(),
                    40f, 240f) { v -> onChange(entry.copy(buttonHeight = v.toInt())) }
                SliderRow("Raio", (if (entry.buttonRadius >= 0) entry.buttonRadius else menu.buttonRadius).toFloat(),
                    0f, 120f) { v -> onChange(entry.copy(buttonRadius = v.toInt())) }
                ToggleLine("Mostrar ícone", if (entry.showIcon >= 0) entry.showIcon == 1 else menu.showIcons) { v ->
                    onChange(entry.copy(showIcon = if (v) 1 else 0))
                }
                TextButton(onClick = { onChange(entry.copy(showIcon = -1)) }) { Text("Herdar ícone do tema") }
                TextButton(onClick = {
                    onChange(entry.copy(buttonStyle = -1, buttonWidth = 0, buttonHeight = 0,
                        buttonRadius = -1, buttonBorderWidth = -1, textAlign = -1,
                        iconScale = 0, showIcon = -1, colorBg = 0, colorBorder = 0,
                        colorText = 0, colorTextSel = 0))
                }) { Text("Restaurar herança") }
            }
        },
        confirmButton = { TextButton(onClick = onSave) { Text("Salvar entries.ini") } },
        dismissButton = { TextButton(onClick = onDismiss) { Text("Fechar") } },
    )
}

@Composable
private fun ImageEditorDialog(
    bitmap: Bitmap,
    name: String,
    zoom: Float,
    offsetX: Float,
    offsetY: Float,
    rotation: Int,
    onZoom: (Float) -> Unit,
    onOffsetX: (Float) -> Unit,
    onOffsetY: (Float) -> Unit,
    onRotation: (Int) -> Unit,
    onDismiss: () -> Unit,
    onExport: () -> Unit,
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text("Editar fundo") },
        text = {
            Column {
                Text(name, style = MaterialTheme.typography.labelSmall)
                Box(
                    Modifier.fillMaxWidth().height(260.dp)
                        .clip(RoundedCornerShape(8.dp))
                        .background(Color.Black),
                    contentAlignment = Alignment.Center,
                ) {
                    ImageView(
                        bitmap = bitmap.asImageBitmap(),
                        contentDescription = null,
                        modifier = Modifier.fillMaxSize()
                            .alpha(0.9f)
                            .graphicsLayer {
                                rotationZ = rotation.toFloat()
                                scaleX = zoom
                                scaleY = zoom
                                translationX = offsetX * size.width * 0.3f
                                translationY = offsetY * size.height * 0.3f
                            },
                        contentScale = ContentScale.Crop,
                    )
                }
                SliderRow("Zoom", zoom, 1f, 4f) { onZoom(it) }
                SliderRow("Horizontal", offsetX, -1f, 1f) { onOffsetX(it) }
                SliderRow("Vertical", offsetY, -1f, 1f) { onOffsetY(it) }
                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    listOf(0, 90, 180, 270).forEach { deg ->
                        FilterChip(
                            selected = rotation == deg,
                            onClick = { onRotation(deg) },
                            label = { Text("$deg°") },
                        )
                    }
                }
                Text(
                    "O recorte acompanha a proporção 720×1600 do menu.",
                    style = MaterialTheme.typography.labelSmall,
                )
            }
        },
        confirmButton = { TextButton(onClick = onExport) { Text("Exportar BMP") } },
        dismissButton = { TextButton(onClick = onDismiss) { Text("Cancelar") } },
    )
}

@Composable
private fun StyleChips(selected: Int, onSelect: (Int) -> Unit) {
    LazyRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
        items(ButtonStyle.names) { label ->
            val index = ButtonStyle.names.indexOf(label)
            FilterChip(
                selected = selected == index,
                onClick = { onSelect(index) },
                label = { Text(label) },
            )
        }
    }
}

@Composable
private fun AlignChips(selected: Int, onSelect: (Int) -> Unit) {
    LazyRow(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
        items(MenuAlign.names) { label ->
            val index = MenuAlign.names.indexOf(label)
            FilterChip(
                selected = selected == index,
                onClick = { onSelect(index) },
                label = { Text(label) },
            )
        }
    }
}

@Composable
private fun SliderRow(label: String, value: Float, min: Float, max: Float, onChange: (Float) -> Unit) {
    Column(Modifier.fillMaxWidth()) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(label, style = MaterialTheme.typography.bodySmall, modifier = Modifier.weight(1f))
            Text(value.toInt().toString(), style = MaterialTheme.typography.labelSmall)
        }
        Slider(value = value.coerceIn(min, max), onValueChange = onChange, valueRange = min..max)
    }
}

@Composable
private fun ToggleLine(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(
        Modifier.fillMaxWidth().padding(vertical = 4.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Text(label, modifier = Modifier.weight(1f))
        Switch(checked = checked, onCheckedChange = onChange)
    }
}

@Composable
private fun ColorRow(label: String, color: Int, onChange: (Int) -> Unit) {
    var hex by remember(color) { mutableStateOf(String.format("#%06X", color and 0xFFFFFF)) }
    Row(verticalAlignment = Alignment.CenterVertically) {
        Box(Modifier.size(28.dp).clip(RoundedCornerShape(6.dp)).background(argb(color)))
        Spacer(Modifier.width(10.dp))
        OutlinedTextField(
            value = hex,
            onValueChange = { v ->
                hex = v
                val parsed = v.removePrefix("#").toIntOrNull(16)
                if (parsed != null && v.removePrefix("#").length == 6) {
                    onChange(0xFF000000.toInt() or parsed)
                }
            },
            label = { Text(label) },
            singleLine = true,
            modifier = Modifier.weight(1f),
        )
    }
}

private fun argb(value: Int): Color = Color(value)
