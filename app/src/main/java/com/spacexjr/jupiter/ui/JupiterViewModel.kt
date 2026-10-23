package com.spacexjr.jupiter.ui

import android.app.Application
import android.content.ContentValues
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.os.Build
import android.os.Environment
import android.provider.MediaStore
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.spacexjr.jupiter.core.binaries.BinaryInstaller
import com.spacexjr.jupiter.core.binaries.BinaryState
import com.spacexjr.jupiter.core.binaries.BinaryStatus
import com.spacexjr.jupiter.core.binaries.TrustedBinary
import com.spacexjr.jupiter.core.config.ConfigIni
import com.spacexjr.jupiter.core.config.ConfigIniParser
import com.spacexjr.jupiter.core.config.ConfigIniSerializer
import com.spacexjr.jupiter.core.config.ConfigIniValidator
import com.spacexjr.jupiter.core.config.EntriesIniParser
import com.spacexjr.jupiter.core.config.EntriesIniSerializer
import com.spacexjr.jupiter.core.config.EntriesValidator
import com.spacexjr.jupiter.core.config.JbmEntry
import com.spacexjr.jupiter.core.config.ThemeIni
import com.spacexjr.jupiter.core.device.DeviceProfile
import com.spacexjr.jupiter.core.log.LogEntry
import com.spacexjr.jupiter.core.log.LogLevel
import com.spacexjr.jupiter.core.log.LogSource
import com.spacexjr.jupiter.core.log.OperationLog
import com.spacexjr.jupiter.core.patch.PatchEngine
import com.spacexjr.jupiter.core.patch.PatchException
import com.spacexjr.jupiter.core.patch.PatchExecutors
import com.spacexjr.jupiter.core.patch.PatchReport
import com.spacexjr.jupiter.core.patch.PatchStateStore
import com.spacexjr.jupiter.core.patch.RepackResult
import com.spacexjr.jupiter.core.patch.ScanResult
import com.spacexjr.jupiter.core.patch.TargetKind
import com.spacexjr.jupiter.core.prefs.AppPrefs
import com.spacexjr.jupiter.core.prefs.ThemeMode
import com.spacexjr.jupiter.core.shell.CommandRunner
import com.spacexjr.jupiter.core.shell.ProcessCommandRunner
import com.spacexjr.jupiter.core.shell.RootService
import com.spacexjr.jupiter.core.shell.runOrError
import com.spacexjr.jupiter.core.shell.toRunnerResult
import com.spacexjr.jupiter.core.store.ConfigRoot
import com.spacexjr.jupiter.core.store.DeviceConfigRoot
import com.spacexjr.jupiter.core.store.LocalConfigRoot
import com.spacexjr.jupiter.core.store.StoreEntry
import com.spacexjr.jupiter.core.update.GitHubReleaseClient
import com.spacexjr.jupiter.core.update.UpdateCheckResult
import com.spacexjr.jupiter.core.update.UpdateInstallResult
import com.spacexjr.jupiter.core.update.UpdateInstaller
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.text.DecimalFormat
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

enum class HomeState { LOADING, ERROR }

data class HomeUiState(
    val appVersion: String = "1.2",
    val jupiterBootVersion: String? = null,
    val rootAvailable: Boolean? = null,
    val kernelVersion: String? = null,
    val deviceModel: String? = null,
    val deviceCodename: String? = null,
    val bootImageState: String? = null,
    val bootImageDetail: String? = null,
    val jupiterBootRunning: String? = null,
    val lastPatchSummary: String? = null,
    val binaries: List<BinaryStatus> = emptyList(),
    val update: UpdateCheckResult? = null,
    val updateInstalling: Boolean = false,
    val updateMessage: String? = null,
    val updateError: String? = null,
    val error: String? = null,
    val loading: Boolean = false,
)

data class PatchUiState(
    val scan: ScanResult? = null,
    val busy: Boolean = false,
    val stepLabel: String? = null,
    val displayReport: PatchReport? = null,
    val report: PatchReport? = null,
    val message: String? = null,
    val error: String? = null,
    val imageReady: Boolean = false,
)

data class EntriesUiState(
    val entries: List<JbmEntry> = emptyList(),
    val source: String? = null,
    val missing: Boolean = false,
    val dirty: Boolean = false,
    val message: String? = null,
    val error: String? = null,
    val issues: List<EntriesValidator.Issue> = emptyList(),
)

data class ConfigUiState(
    val config: ConfigIni = ConfigIni.defaults(),
    val rawText: String? = null,
    val missing: Boolean = false,
    val dirty: Boolean = false,
    val issues: List<ConfigIniValidator.Issue> = emptyList(),
    val message: String? = null,
    val error: String? = null,
    val backups: List<String> = emptyList(),
)

data class AppearanceUiState(
    val themes: Map<String, ThemeIni> = emptyMap(),
    val selectedName: String = ConfigIni.defaults().theme,
    val backgroundPreview: Bitmap? = null,
    val backgroundApplied: Boolean = false,
    val dirty: Boolean = false,
    val message: String? = null,
    val error: String? = null,
) {
    val selectedTheme: ThemeIni
        get() = themes[selectedName] ?: ThemeIni.defaults().copy(name = selectedName)
}

data class FilesUiState(
    val path: String = "",
    val entries: List<StoreEntry> = emptyList(),
    val selectedPath: String? = null,
    val selectedContent: String? = null,
    val selectedDetail: String? = null,
    val selectedIsBinary: Boolean = false,
    val message: String? = null,
    val error: String? = null,
)

class JupiterViewModel(app: Application) : AndroidViewModel(app) {

    private val appDir: File = app.filesDir
    private val workspaceDir: File = File(appDir, "workspace")
    private val localTestRootDir: File = File(appDir, "testdata/Jupiter")
    private val binariesDir: File = File(appDir, "bin")
    private val logsDir: File = File(appDir, "logs")
    private val patchesDir: File = File(appDir, "patches")

    private val operationLog = OperationLog(File(logsDir, "operations.log"))
    private val updateInstaller = UpdateInstaller(operationLog)
    private val runner: CommandRunner = ProcessCommandRunner()
    private val rootService = RootService(runner, operationLog)
    private val prefs = AppPrefs(app)

    private val binaryInstaller = BinaryInstaller(
        assets = object : BinaryInstaller.AssetReader {
            override fun open(assetPath: String) = app.assets.open(assetPath)
        },
        targetDir = binariesDir,
        runner = runner,
        log = operationLog,
    )

    private val patchStore = PatchStateStore(File(patchesDir, "last-patch.json"))
    private val githubClient = GitHubReleaseClient(operationLog)

    val logs: StateFlow<List<LogEntry>> = operationLog.entries

    private val patchExecutors = object : PatchExecutors {
        override suspend fun direct(argv: List<String>, workdir: File?, timeoutMs: Long) =
            runner.runOrError(argv, timeoutMs, workdir)

        override suspend fun root(argv: List<String>, workdir: File?, timeoutMs: Long) =
            rootService.execRoot(argv, timeoutMs, workdir).toRunnerResult()
    }

    private val patchEngine = PatchEngine(
        executors = patchExecutors,
        log = operationLog,
        magiskboot = { File(binariesDir, "magiskboot") },
        jbmInit = { File(binariesDir, "jbm_init") },
    )

    private val _home = MutableStateFlow(HomeUiState())
    val home: StateFlow<HomeUiState> = _home.asStateFlow()

    private val _patch = MutableStateFlow(PatchUiState())
    val patch: StateFlow<PatchUiState> = _patch.asStateFlow()

    private val _entries = MutableStateFlow(EntriesUiState())
    val entries: StateFlow<EntriesUiState> = _entries.asStateFlow()

    private val _config = MutableStateFlow(ConfigUiState())
    val config: StateFlow<ConfigUiState> = _config.asStateFlow()

    private val _appearance = MutableStateFlow(AppearanceUiState())
    val appearance: StateFlow<AppearanceUiState> = _appearance.asStateFlow()

    private val _deviceProfiles = MutableStateFlow<List<DeviceProfile>>(emptyList())
    val deviceProfiles: StateFlow<List<DeviceProfile>> = _deviceProfiles.asStateFlow()

    private val _files = MutableStateFlow(FilesUiState())
    val files: StateFlow<FilesUiState> = _files.asStateFlow()

    private val _logsQuery = MutableStateFlow(LogFilter())
    val logFilter: StateFlow<LogFilter> = _logsQuery.asStateFlow()

    private var manifestJsonCache: String? = null

    // ---------------------------------------------------------------- root
    fun rootAvailable(): Boolean = _home.value.rootAvailable == true

    private fun currentRoot(): ConfigRoot = when (prefs.configRootId) {
        ConfigRoot.LOCAL_ID -> LocalConfigRoot(localTestRootDir)
        else -> DeviceConfigRoot(rootService)
    }

    fun isLocalTestMode(): Boolean = prefs.configRootId == ConfigRoot.LOCAL_ID

    fun setLocalTestMode(local: Boolean) {
        prefs.configRootId = if (local) ConfigRoot.LOCAL_ID else ConfigRoot.DEVICE_ID
        refreshAll()
    }

    // ---------------------------------------------------------------- tema
    fun themeMode(): ThemeMode = prefs.themeMode
    fun dynamicColor(): Boolean = prefs.dynamicColor
    fun setThemeMode(mode: ThemeMode) {
        prefs.themeMode = mode
        _home.value = _home.value.copy()
    }

    fun setDynamicColor(enabled: Boolean) {
        prefs.dynamicColor = enabled
        _home.value = _home.value.copy()
    }

    // --------------------------------------------------------------- inicio
    fun refreshAll() {
        viewModelScope.launch {
            _home.value = _home.value.copy(loading = true, error = null)
            loadDeviceProfiles()
            installBinaries()
            loadDeviceInfo()
            loadLastPatch()
            checkUpdate()
            _home.value = _home.value.copy(loading = false)
        }
    }

    private suspend fun installBinaries() {
        val trusted = if (prefs.jbmInitSha256.isNotBlank() && prefs.jbmInitVersion.isNotBlank()) {
            listOf(TrustedBinary("jbm_init", prefs.jbmInitVersion, prefs.jbmInitSha256))
        } else {
            emptyList()
        }
        val statuses = binaryInstaller.ensureInstalled(trusted)
        manifestJsonCache = withContext(Dispatchers.IO) {
            try {
                getApplication<Application>().assets.open("binaries.json").use {
                    it.readBytes().decodeToString()
                }
            } catch (_: Exception) {
                null
            }
        }
        val bundledVersion = manifestJsonCache?.let { BinaryInstaller.jupiterBootVersion(it) }
        val installedVersion = prefs.jbmInitVersion.ifBlank { bundledVersion }
        val okCount = statuses.count {
            it.state == BinaryState.OK || it.state == BinaryState.INSTALLED_NOW ||
                it.state == BinaryState.UPDATED
        }
        _home.value = _home.value.copy(
            binaries = statuses,
            jupiterBootVersion = installedVersion,
            error = if (okCount != statuses.size) {
                "Alguns binários não passaram na validação (veja a tela inicial)."
            } else null,
        )
    }

    private suspend fun loadDeviceInfo() {
        val model = runner.runOrError(listOf("getprop", "ro.product.model"), 5_000).stdout.trim()
        val device = runner.runOrError(listOf("getprop", "ro.product.device"), 5_000).stdout.trim()
        val kernel = runner.runOrError(listOf("uname", "-r"), 5_000).stdout.trim()
        val bootState = describeBootImage()
        val root = rootService.isRootAvailable(refresh = true)
        val running = if (root) detectJupiterRunning() else null
        _home.value = _home.value.copy(
            rootAvailable = root,
            deviceModel = model.ifBlank { android.os.Build.MODEL },
            deviceCodename = device.ifBlank { android.os.Build.DEVICE },
            kernelVersion = kernel.ifBlank { System.getProperty("os.version") ?: "desconhecido" },
            bootImageState = bootState.first,
            bootImageDetail = bootState.second,
            jupiterBootRunning = running,
        )
    }

    private suspend fun describeBootImage(): Pair<String, String?> {
        if (!rootService.isRootAvailable()) {
            return "Não verificável sem root" to "O app não consegue inspecionar a partição sem su."
        }
        val scan = try {
            patchEngine.scanPartitions()
        } catch (e: Exception) {
            return "Erro ao inspecionar" to (e.message ?: "falha no scan")
        }
        val chosen = scan.chosen
            ?: return "Arquitetura não reconhecida" to
                "Nenhuma partição boot/init_boot compatível encontrada."
        val detail = "${chosen.partition.name} · ${chosen.kind} · confiança ${chosen.confidence}"
        return "Compatível ($detail)" to chosen.rationale
    }

    /**
     * Detecta evidências de JupiterBoot em execução:
     * - /init.system presente no sistema montado (indício de patch ativo);
     * - rotas do JupiterBoot em /dev/kmsg (best-effort).
     * A ausência de evidência NÃO é tratada como falha.
     */
    private suspend fun detectJupiterRunning(): String {
        val initSystem = rootService.rootStat("/init.system")
        // dmesg termina; ler /dev/kmsg diretamente pode bloquear esperando
        // novas mensagens do kernel e travar a detecção de root na Home.
        val kmsg = rootService.execRoot(
            listOf("sh", "-c", "dmesg 2>/dev/null | grep -m1 -a jbm || true"),
            8_000,
        ).stdout.trim()
        return when {
            initSystem != null -> "Detectado (init.system presente)"
            kmsg.isNotBlank() -> "Detectado (log do menu)"
            else -> "Execução não confirmada"
        }
    }

    private suspend fun loadLastPatch() {
        val report = patchStore.load() ?: return
        val f = SimpleDateFormat("dd/MM HH:mm", Locale.getDefault()).format(Date(report.finishedAtMs))
        val summary = if (report.success) {
            "Sucesso · $f · ${report.target.partition.name} · ${report.repack?.outputSha256?.take(12)}…"
        } else {
            "Falhou · $f · ${report.failedStep}: ${report.failureReason}"
        }
        _patch.value = _patch.value.copy(report = report, displayReport = report, imageReady = report.success)
        _home.value = _home.value.copy(lastPatchSummary = summary)
    }

    fun checkUpdate() {
        viewModelScope.launch {
            val local = _home.value.jupiterBootVersion
            val result = githubClient.checkForUpdate(local)
            _home.value = _home.value.copy(update = result)
        }
    }

    fun installUpdate() {
        val available = _home.value.update as? UpdateCheckResult.UpdateAvailable ?: return
        viewModelScope.launch {
            _home.value = _home.value.copy(
                updateInstalling = true,
                updateMessage = null,
                updateError = null,
            )
            val result = updateInstaller.install(available.release, binariesDir)
            when (result) {
                is UpdateInstallResult.Installed -> {
                    prefs.jbmInitVersion = result.version.removePrefix("v")
                    prefs.jbmInitSha256 = result.sha256
                    operationLog.append(
                        LogLevel.INFO, LogSource.UPDATE,
                        "jbm_init ${result.version} instalado em ${result.path} (nada executado)",
                    )
                    installBinaries()
                    _home.value = _home.value.copy(
                        updateInstalling = false,
                        updateMessage = "jbm_init ${result.version} instalado. " +
                            "O patch continua sendo uma ação separada e confirmada.",
                        updateError = null,
                    )
                }
                is UpdateInstallResult.Failed -> {
                    _home.value = _home.value.copy(
                        updateInstalling = false,
                        updateError = result.message,
                    )
                }
            }
        }
    }

    // ------------------------------------------------------------- profiles
    fun loadDeviceProfiles() {
        viewModelScope.launch(Dispatchers.IO) {
            val profiles = try {
                getApplication<Application>().assets.open("device_profiles.json").use {
                    DeviceProfile.parse(it.readBytes().decodeToString())
                }
            } catch (_: Exception) {
                emptyList()
            }
            withContext(Dispatchers.Main) { _deviceProfiles.value = profiles }
        }
    }

    fun selectedProfileId(): String = prefs.deviceProfileId

    fun detectedProfile(): DeviceProfile? {
        val codename = _home.value.deviceCodename ?: return null
        return DeviceProfile.detect(_deviceProfiles.value, codename)
    }

    fun selectDeviceProfile(id: String) {
        prefs.deviceProfileId = id
        _deviceProfiles.value = _deviceProfiles.value
    }

    fun applyDeviceProfile(id: String) {
        val profile = _deviceProfiles.value.firstOrNull { it.id == id } ?: return
        prefs.deviceProfileId = id
        updateConfig { cfg ->
            cfg.copy(
                orientation = profile.orientation,
                touchCalX0 = if (profile.touchCalX0 >= 0) profile.touchCalX0 else cfg.touchCalX0,
                touchCalY0 = if (profile.touchCalY0 >= 0) profile.touchCalY0 else cfg.touchCalY0,
                touchCalX1 = if (profile.touchCalX1 >= 0) profile.touchCalX1 else cfg.touchCalX1,
                touchCalY1 = if (profile.touchCalY1 >= 0) profile.touchCalY1 else cfg.touchCalY1,
            )
        }
        _config.value = _config.value.copy(
            message = "Perfil ${profile.name} aplicado ao config.ini; salve para gravar.",
        )
    }

    // ---------------------------------------------------------------- patch
    fun scanPartitions() {
        viewModelScope.launch {
            // Se o último probe falhou (diálogo negado/timeout), tenta de novo
            // antes de bloquear o scan.
            val root = rootService.isRootAvailable() || rootService.isRootAvailable(refresh = true)
            if (!root) {
                _home.value = _home.value.copy(rootAvailable = false)
                _patch.value = _patch.value.copy(
                    error = "Root indisponível: conceda acesso ao su quando o diálogo aparecer.",
                )
                return@launch
            }
            _home.value = _home.value.copy(rootAvailable = true)
            _patch.value = _patch.value.copy(busy = true, stepLabel = "Identificando partições…", error = null)
            try {
                val scan = patchEngine.scanPartitions()
                _patch.value = _patch.value.copy(scan = scan, busy = false, stepLabel = null)
            } catch (e: Exception) {
                _patch.value = _patch.value.copy(busy = false, stepLabel = null, error = e.message)
            }
        }
    }

    fun selectTarget(partitionName: String) {
        val scan = _patch.value.scan ?: return
        val partition = scan.partitions.firstOrNull { it.name == partitionName } ?: return
        val kind = when {
            partitionName.startsWith("init_boot") -> TargetKind.INIT_BOOT
            partitionName.startsWith("vendor_boot") -> TargetKind.VENDOR_BOOT
            else -> TargetKind.BOOT
        }
        val target = com.spacexjr.jupiter.core.patch.PatchTarget(
            partition = partition,
            kind = kind,
            rationale = "Selecionado manualmente pelo usuário.",
            confidence = com.spacexjr.jupiter.core.patch.Confidence.LOW,
        )
        _patch.value = _patch.value.copy(scan = scan.copy(chosen = target), error = null)
    }

    /** Executa todas as etapas do patch com confirmação já dada pela UI. */
    fun runPatch() {
        val scan = _patch.value.scan
        val target = scan?.chosen
        if (target == null) {
            _patch.value = _patch.value.copy(error = "Nenhum alvo selecionado.")
            return
        }
        if (target.kind == TargetKind.VENDOR_BOOT) {
            _patch.value = _patch.value.copy(
                error = "vendor_boot não é suportado como alvo do JupiterBoot.",
            )
            return
        }
        val binariesProblem = binariesProblem()
        if (binariesProblem != null) {
            _patch.value = _patch.value.copy(error = binariesProblem)
            return
        }
        viewModelScope.launch(Dispatchers.IO) {
            val started = System.currentTimeMillis()
            val warnings = mutableListOf<String>()
            val workDir = File(workspaceDir, "patch-${System.currentTimeMillis()}").apply { mkdirs() }
            var step = "início"
            var image: com.spacexjr.jupiter.core.patch.ExtractedImage? = null
            var unpack: com.spacexjr.jupiter.core.patch.UnpackResult? = null
            var ramdisk: com.spacexjr.jupiter.core.patch.RamdiskInfo? = null
            var install: com.spacexjr.jupiter.core.patch.InstallResult? = null
            var repack: RepackResult? = null
            var failure: String? = null

            try {
                _patch.value = _patch.value.copy(busy = true, stepLabel = "Lendo a partição com dd…", error = null)
                step = "dd"
                image = patchEngine.extractImage(target, workDir)

                _patch.value = _patch.value.copy(stepLabel = "Extraindo ramdisk (magiskboot)…")
                step = "unpack"
                unpack = patchEngine.unpack(workDir, image)

                _patch.value = _patch.value.copy(stepLabel = "Inspecionando o ramdisk…")
                step = "inspeção"
                ramdisk = patchEngine.inspectRamdisk(workDir)
                if (ramdisk.hasInitSystem) {
                    warnings += "O ramdisk já continha /init.system; um patch anterior pode ter sido aplicado."
                }
                if (scan.slotSuffix.isNotEmpty()) {
                    warnings += "Slot ativo: ${scan.slotSuffix}. Outras variantes A/B não foram tocadas."
                }
                warnings += "A imagem final NÃO foi gravada na partição; exporte e aplique manualmente."

                _patch.value = _patch.value.copy(stepLabel = "Integrando o JupiterBoot (/init -> /init.system)…")
                step = "integração"
                install = patchEngine.installJupiter(workDir)

                _patch.value = _patch.value.copy(stepLabel = "Reempacotando e verificando…")
                step = "repack"
                repack = patchEngine.repack(workDir, image)
            } catch (e: PatchException) {
                failure = e.message
            } catch (e: Exception) {
                failure = e.message ?: e.javaClass.simpleName
            }

            val report = PatchReport(
                startedAtMs = started,
                finishedAtMs = System.currentTimeMillis(),
                target = target,
                image = image,
                unpack = unpack,
                ramdisk = ramdisk,
                install = install,
                repack = repack,
                warnings = warnings,
                failedStep = if (failure == null) null else step,
                failureReason = failure,
            )
            patchStore.save(report)
            val f = SimpleDateFormat("dd/MM HH:mm", Locale.getDefault()).format(Date(report.finishedAtMs))
            _home.value = _home.value.copy(
                lastPatchSummary = if (report.success) {
                    "Sucesso · $f · ${target.partition.name}"
                } else {
                    "Falhou · $f · $step: $failure"
                },
            )
            withContext(Dispatchers.Main) {
                _patch.value = _patch.value.copy(
                    busy = false,
                    stepLabel = null,
                    report = report,
                    displayReport = report,
                    imageReady = report.success,
                    error = failure,
                    message = if (report.success) "Patch concluído e verificado." else null,
                )
            }
        }
    }

    /** Nome usado na exportação: jupiter-MMddyyyy-HHmm.img. */
    fun exportFileName(now: Date = Date()): String =
        "jupiter-" + SimpleDateFormat("MMddyyyy-HHmm", Locale.US).format(now) + ".img"

    /**
     * Exporta a imagem gerada para /sdcard/Download.
     *
     * Android 10+ usa MediaStore (sem precisar de permissão de armazenamento);
     * versões antigas gravam direto em Downloads com WRITE_EXTERNAL_STORAGE.
     */
    fun exportPatchedImageToDownloads(onDone: (String) -> Unit) {
        val out = _patch.value.report?.repack?.outputFile ?: run {
            onDone("Nenhuma imagem para exportar.")
            return
        }
        viewModelScope.launch(Dispatchers.IO) {
            val name = exportFileName()
            val result = try {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    exportViaMediaStore(name, out)
                } else {
                    exportLegacy(name, out)
                }
            } catch (e: Exception) {
                e.message ?: "falha desconhecida"
            }
            operationLog.append(
                if (result == null) LogLevel.INFO else LogLevel.ERROR,
                LogSource.PATCH,
                if (result == null) "imagem exportada: Download/$name" else "falha ao exportar: $result",
            )
            withContext(Dispatchers.Main) {
                onDone(
                    if (result == null) "Imagem exportada em Download/$name" else "Falha ao exportar: $result",
                )
            }
        }
    }

    /** Retorna null em sucesso ou a mensagem de erro. */
    private fun exportViaMediaStore(name: String, source: File): String? {
        val resolver = getApplication<Application>().contentResolver
        val values = ContentValues().apply {
            put(MediaStore.MediaColumns.DISPLAY_NAME, name)
            put(MediaStore.MediaColumns.MIME_TYPE, "application/octet-stream")
            put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS)
            put(MediaStore.MediaColumns.IS_PENDING, 1)
        }
        val uri = resolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, values)
            ?: return "MediaStore recusou criar o arquivo"
        return try {
            resolver.openOutputStream(uri)?.use { os ->
                source.inputStream().use { it.copyTo(os) }
            } ?: return "não foi possível abrir o arquivo de destino"
            val done = ContentValues().apply { put(MediaStore.MediaColumns.IS_PENDING, 0) }
            resolver.update(uri, done, null, null)
            null
        } catch (e: Exception) {
            runCatching { resolver.delete(uri, null, null) }
            e.message ?: "erro ao gravar no MediaStore"
        }
    }

    private fun exportLegacy(name: String, source: File): String? {
        val dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
        if (!dir.exists() && !dir.mkdirs()) return "não foi possível criar ${dir.absolutePath}"
        source.copyTo(File(dir, name), overwrite = true)
        return null
    }

    fun clearPatchReport() {
        _patch.value = _patch.value.copy(report = null, displayReport = null, imageReady = false, message = null)
    }

    // -------------------------------------------------------------- entries
    fun loadEntries() {
        viewModelScope.launch {
            val root = currentRoot()
            _entries.value = _entries.value.copy(message = null, error = null)
            if (!root.exists("entries.ini")) {
                _entries.value = EntriesUiState(
                    entries = EntriesIniParser.builtinEntries(),
                    source = root.displayPath,
                    missing = true,
                    message = "entries.ini ainda não existe; usando as entradas internas do JupiterBoot.",
                )
                return@launch
            }
            val text = root.readText("entries.ini")
            if (text == null) {
                _entries.value = _entries.value.copy(error = "Não foi possível ler entries.ini")
                return@launch
            }
            val doc = EntriesIniParser.parse(text)
            val model = EntriesIniParser.toModel(doc)
            _entries.value = EntriesUiState(
                entries = model,
                source = root.displayPath,
                missing = false,
                dirty = false,
                issues = EntriesValidator.validate(model),
            )
        }
    }

    fun updateEntry(index: Int, transform: (JbmEntry) -> JbmEntry) {
        val list = _entries.value.entries.toMutableList()
        if (index !in list.indices) return
        list[index] = transform(list[index])
        _entries.value = _entries.value.copy(
            entries = list,
            dirty = true,
            issues = EntriesValidator.validate(list),
        )
    }

    fun addEntry() {
        val list = _entries.value.entries.toMutableList()
        val nextOrder = (list.maxOfOrNull { it.order } ?: -1) + 1
        val idBase = "custom"
        var id = "$idBase$nextOrder"
        var n = nextOrder
        while (list.any { it.id == id }) { n++; id = "$idBase$n" }
        list += JbmEntry(id = id, name = "Nova entrada", type = com.spacexjr.jupiter.core.config.EntryType.SCRIPT, order = nextOrder)
        _entries.value = _entries.value.copy(
            entries = list,
            dirty = true,
            issues = EntriesValidator.validate(list),
        )
    }

    fun removeEntry(index: Int) {
        val list = _entries.value.entries.toMutableList()
        if (index !in list.indices) return
        list.removeAt(index)
        _entries.value = _entries.value.copy(
            entries = list,
            dirty = true,
            issues = EntriesValidator.validate(list),
        )
    }

    fun moveEntry(index: Int, delta: Int) {
        val list = _entries.value.entries.toMutableList()
        val target = index + delta
        if (index !in list.indices || target !in list.indices) return
        val tmp = list[index]
        list[index] = list[target]
        list[target] = tmp
        list.forEachIndexed { i, e -> list[i] = e.copy(order = i) }
        _entries.value = _entries.value.copy(
            entries = list,
            dirty = true,
            issues = EntriesValidator.validate(list),
        )
    }

    fun saveEntries() {
        val state = _entries.value
        val fatal = state.issues.filter { it.fatal }
        if (fatal.isNotEmpty()) {
            _entries.value = state.copy(error = "Corrija os erros antes de salvar.")
            return
        }
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val existingText = root.readText("entries.ini")
            val unknown = if (existingText != null) {
                val doc = EntriesIniParser.parse(existingText)
                state.entries.associate { it.id to EntriesIniParser.unknownFieldsFor(doc, it.id) }
            } else emptyMap()
            val header = if (existingText != null) {
                existingText.lineSequence().takeWhile { it.isBlank() || it.trimStart().startsWith("#") }.toList()
            } else {
                EntriesIniParser.builtinTemplate().lineSequence()
                    .takeWhile { it.isBlank() || it.trimStart().startsWith("#") }.toList()
            }
            val text = EntriesIniSerializer.write(state.entries, unknown, header)
            root.backup("entries.ini")
            val ok = root.writeAtomic("entries.ini", text)
            operationLog.append(
                if (ok) LogLevel.INFO else LogLevel.ERROR,
                LogSource.ENTRIES,
                "gravação de entries.ini em ${root.displayPath}: ${if (ok) "ok" else "falha"}",
            )
            withContext(Dispatchers.Main) {
                _entries.value = _entries.value.copy(
                    dirty = !ok,
                    message = if (ok) "entries.ini gravado (backup criado)." else null,
                    error = if (ok) null else "Falha ao gravar entries.ini",
                )
            }
        }
    }

    fun restoreEntriesBackup() {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val backups = root.listBackups("entries.ini")
            if (backups.isEmpty()) {
                withContext(Dispatchers.Main) {
                    _entries.value = _entries.value.copy(error = "Nenhum backup de entries.ini encontrado.")
                }
                return@launch
            }
            val ok = root.restoreBackup("entries.ini", backups.first())
            operationLog.append(
                if (ok) LogLevel.INFO else LogLevel.ERROR,
                LogSource.ENTRIES,
                "restauração do backup ${backups.first()}",
            )
            withContext(Dispatchers.Main) {
                _entries.value = _entries.value.copy(
                    message = if (ok) "Backup restaurado: ${backups.first()}" else null,
                    error = if (ok) null else "Falha ao restaurar o backup",
                )
            }
            loadEntries()
        }
    }

    // --------------------------------------------------------------- config
    fun loadConfig() {
        viewModelScope.launch {
            val root = currentRoot()
            _config.value = ConfigUiState()
            val text = root.readText("config.ini")
            if (text == null) {
                _config.value = _config.value.copy(
                    config = ConfigIni.defaults(),
                    rawText = null,
                    missing = true,
                    message = "config.ini ainda não existe; mostrando os valores padrão do JupiterBoot.",
                    backups = root.listBackups("config.ini"),
                )
                return@launch
            }
            _config.value = _config.value.copy(
                config = ConfigIniParser.parse(text),
                rawText = text,
                missing = false,
                issues = ConfigIniValidator.validate(ConfigIniParser.parse(text)),
                backups = root.listBackups("config.ini"),
            )
        }
    }

    fun updateConfig(transform: (ConfigIni) -> ConfigIni) {
        val next = transform(_config.value.config)
        _config.value = _config.value.copy(
            config = next,
            dirty = true,
            issues = ConfigIniValidator.validate(next),
            message = null,
        )
    }

    fun saveConfig() {
        val next = _config.value.config
        val issues = ConfigIniValidator.validate(next)
        if (issues.isNotEmpty()) {
            _config.value = _config.value.copy(error = "Corrija os valores inválidos antes de salvar.")
            return
        }
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            root.backup("config.ini")
            val text = ConfigIniSerializer.write(next, _config.value.rawText)
            val ok = root.writeAtomic("config.ini", text)
            operationLog.append(
                if (ok) LogLevel.INFO else LogLevel.ERROR,
                LogSource.CONFIG,
                "gravação de config.ini em ${root.displayPath}: ${if (ok) "ok" else "falha"}",
            )
            withContext(Dispatchers.Main) {
                _config.value = _config.value.copy(
                    rawText = if (ok) text else _config.value.rawText,
                    dirty = !ok,
                    message = if (ok) "config.ini gravado (backup criado)." else null,
                    error = if (ok) null else "Falha ao gravar config.ini",
                    config = next.copy(mask = if (ok) next.mask else _config.value.config.mask),
                )
            }
        }
    }

    fun createConfigFromDefaults() {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val ok = root.writeAtomic("config.ini", ConfigIni.defaultTemplate())
            operationLog.append(
                if (ok) LogLevel.INFO else LogLevel.ERROR,
                LogSource.CONFIG,
                "criação de config.ini padrão: ${if (ok) "ok" else "falha"}",
            )
            withContext(Dispatchers.Main) {
                _config.value = _config.value.copy(
                    message = if (ok) "config.ini criado com os padrões." else null,
                    error = if (ok) null else "Falha ao criar config.ini",
                )
            }
            loadConfig()
        }
    }

    fun restoreConfigBackup(name: String) {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val ok = root.restoreBackup("config.ini", name)
            operationLog.append(
                if (ok) LogLevel.INFO else LogLevel.ERROR,
                LogSource.CONFIG,
                "restauração do backup $name",
            )
            withContext(Dispatchers.Main) {
                _config.value = _config.value.copy(
                    message = if (ok) "Backup restaurado: $name" else null,
                    error = if (ok) null else "Falha ao restaurar backup",
                )
            }
            loadConfig()
        }
    }

    // ----------------------------------------------------------- appearance
    fun loadAppearance() {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val names = root.listDirectory("themes")
                .filter { !it.isDirectory && it.name.endsWith(".ini") }
                .map { it.name.removeSuffix(".ini") }
                .sorted()
            val themes = names.associateWith { name ->
                val text = root.readText("themes/$name.ini")
                if (text == null) ThemeIni(name = name) else ThemeIni.parse(name, text)
            }
            val configured = root.readText("config.ini")
                ?.let { ConfigIniParser.parse(it).theme }
                ?.ifEmpty { "default" }
                ?: _config.value.config.theme.ifEmpty { "default" }
            val selected = if (themes.containsKey(configured)) configured
            else themes.keys.firstOrNull() ?: configured
            val preview = if (root.exists("background.bmp")) {
                val tmp = File(appDir, "preview-background.bmp")
                if (root.exportToFile("background.bmp", tmp)) BitmapFactory.decodeFile(tmp.absolutePath) else null
            } else null
            withContext(Dispatchers.Main) {
                _appearance.value = _appearance.value.copy(
                    themes = themes,
                    selectedName = selected,
                    backgroundPreview = preview,
                    error = if (themes.isEmpty()) "Nenhum tema encontrado em themes/." else null,
                    message = null,
                )
            }
        }
    }

    fun selectTheme(name: String) {
        val state = _appearance.value
        if (!state.themes.containsKey(name)) return
        _appearance.value = state.copy(selectedName = name, dirty = false, message = null, error = null)
        updateConfig { it.copy(theme = name) }
    }

    fun updateTheme(transform: (ThemeIni) -> ThemeIni) {
        val state = _appearance.value
        val before = ThemeIni.resolve(state.selectedName, state.themes)
        val next = ThemeIni.markChanged(before, transform(before))
        _appearance.value = state.copy(
            themes = state.themes + (state.selectedName to next),
            dirty = true,
            message = null,
            error = null,
        )
    }

    fun saveTheme() {
        val state = _appearance.value
        val theme = state.selectedTheme
        if (!Regex("^[A-Za-z0-9._-]{1,63}$").matches(state.selectedName)) {
            _appearance.value = state.copy(error = "Nome de tema invalido.")
            return
        }
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val rel = "themes/${state.selectedName}.ini"
            root.backup(rel)
            val ok = root.writeAtomic(rel, ThemeIni.serialize(theme))
            operationLog.append(
                if (ok) LogLevel.INFO else LogLevel.ERROR,
                LogSource.CONFIG,
                "gravação de $rel: ${if (ok) "ok" else "falha"}",
            )
            withContext(Dispatchers.Main) {
                _appearance.value = _appearance.value.copy(
                    dirty = !ok,
                    message = if (ok) "Tema ${state.selectedName} gravado (backup criado)." else null,
                    error = if (ok) null else "Falha ao gravar o tema",
                )
            }
        }
    }

    fun createTheme(name: String) {
        val clean = name.trim()
        if (!Regex("^[A-Za-z0-9._-]{1,63}$").matches(clean)) {
            _appearance.value = _appearance.value.copy(error = "Use letras, numeros, ponto, hifen ou _.")
            return
        }
        val state = _appearance.value
        if (state.themes.containsKey(clean)) {
            _appearance.value = state.copy(error = "Ja existe um tema com esse nome.")
            return
        }
        val base = state.selectedTheme.copy(name = clean, extends = "")
        _appearance.value = state.copy(
            themes = state.themes + (clean to base),
            selectedName = clean,
            dirty = true,
            message = "Tema $clean criado na memoria; salve para gravar.",
            error = null,
        )
    }

    fun deleteTheme(name: String) {
        if (name == _config.value.config.theme) {
            _appearance.value = _appearance.value.copy(
                error = "Nao e possivel apagar o tema em uso.",
            )
            return
        }
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val ok = root.delete("themes/$name.ini")
            withContext(Dispatchers.Main) {
                val state = _appearance.value
                _appearance.value = if (ok) state.copy(
                    themes = state.themes - name,
                    selectedName = if (state.selectedName == name) {
                        state.themes.keys.firstOrNull { it != name } ?: "default"
                    } else state.selectedName,
                    message = "Tema $name removido.",
                    error = null,
                ) else state.copy(error = "Falha ao remover o tema.")
            }
        }
    }

    fun importBackground(source: File) {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val decoded = BitmapFactory.decodeFile(source.absolutePath)
            val ok = root.copyIntoAppFile("background.bmp", source)
            operationLog.append(
                if (ok) LogLevel.INFO else LogLevel.ERROR,
                LogSource.CONFIG,
                "importação de background.bmp: ${if (ok) "ok" else "falha"}",
            )
            withContext(Dispatchers.Main) {
                _appearance.value = _appearance.value.copy(
                    backgroundPreview = if (ok) decoded else _appearance.value.backgroundPreview,
                    backgroundApplied = ok,
                    message = if (ok) "background.bmp gravado em ${root.displayPath}." else null,
                    error = if (ok) null else "Falha ao gravar background.bmp",
                )
                if (ok) {
                    _config.value = _config.value.copy(
                        config = _config.value.config.copy(background = "${root.displayPath}/background.bmp"),
                        dirty = true,
                    )
                    saveConfig()
                }
            }
        }
    }

    // ---------------------------------------------------------------- files
    fun openDirectory(path: String) {
        viewModelScope.launch {
            val root = currentRoot()
            val entries = root.listDirectory(path)
            _files.value = FilesUiState(path = path, entries = entries)
        }
    }

    fun openFile(path: String) {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val stat = root.stat(path)
            val detail = stat?.let { "${it.size} bytes · ${it.mode} · ${it.owner}" }
            // Heurística: tenta ler como texto; se contiver NUL, trata como binário.
            val text = root.readText(path)
            val isBinary = text != null && text.contains('\u0000')
            val shown = when {
                isBinary -> null
                text != null -> text.take(200_000)
                else -> null
            }
            withContext(Dispatchers.Main) {
                _files.value = _files.value.copy(
                    selectedPath = path,
                    selectedContent = shown,
                    selectedDetail = detail,
                    selectedIsBinary = isBinary || (text == null && stat != null),
                    error = if (text == null && stat == null) "Arquivo não encontrado ou sem permissão." else null,
                )
            }
        }
    }

    fun saveFile(path: String, content: String) {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            root.backup(path)
            val ok = root.writeAtomic(path, content)
            operationLog.append(
                if (ok) LogLevel.INFO else LogLevel.ERROR,
                LogSource.FILES,
                "gravação de $path: ${if (ok) "ok" else "falha"}",
            )
            withContext(Dispatchers.Main) {
                _files.value = _files.value.copy(
                    message = if (ok) "Arquivo gravado: $path" else null,
                    error = if (ok) null else "Falha ao gravar $path",
                )
            }
        }
    }

    fun deleteFile(path: String) {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val ok = root.delete(path)
            operationLog.append(
                if (ok) LogLevel.INFO else LogLevel.ERROR,
                LogSource.FILES,
                "exclusão de $path: ${if (ok) "ok" else "falha"}",
            )
            withContext(Dispatchers.Main) {
                _files.value = _files.value.copy(
                    message = if (ok) "Removido: $path" else null,
                    error = if (ok) null else "Falha ao remover $path",
                    selectedPath = if (ok) null else _files.value.selectedPath,
                )
            }
            openDirectory(_files.value.path)
        }
    }

    fun makeDirs(path: String) {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val ok = root.makeDirs(path)
            withContext(Dispatchers.Main) {
                _files.value = _files.value.copy(
                    message = if (ok) "Diretório criado: $path" else null,
                    error = if (ok) null else "Falha ao criar $path",
                )
            }
            openDirectory(_files.value.path)
        }
    }

    fun importAppFile(relPath: String, source: File) {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val ok = root.copyIntoAppFile(relPath, source)
            operationLog.append(
                if (ok) LogLevel.INFO else LogLevel.ERROR,
                LogSource.FILES,
                "cópia de ${source.name} para $relPath",
            )
            withContext(Dispatchers.Main) {
                _files.value = _files.value.copy(
                    message = if (ok) "Importado: $relPath" else null,
                    error = if (ok) null else "Falha ao importar ${source.name}",
                )
            }
        }
    }

    fun exportFile(relPath: String, target: File) {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val ok = root.exportToFile(relPath, target)
            withContext(Dispatchers.Main) {
                _files.value = _files.value.copy(
                    message = if (ok) "Exportado: ${target.name}" else null,
                    error = if (ok) null else "Falha ao exportar $relPath",
                )
            }
        }
    }

    fun chmodExecutable(relPath: String) {
        viewModelScope.launch(Dispatchers.IO) {
            val root = currentRoot()
            val ok = root.chmod(relPath, "0755")
            withContext(Dispatchers.Main) {
                _files.value = _files.value.copy(
                    message = if (ok) "+x aplicado em $relPath" else null,
                    error = if (ok) null else "Falha ao aplicar +x",
                )
            }
            openFile(relPath)
        }
    }

    fun setLogFilter(transform: (LogFilter) -> LogFilter) {
        _logsQuery.value = transform(_logsQuery.value)
    }

    fun clearLogs() = operationLog.clear()

    fun refreshLogs() {
        _logsQuery.value = _logsQuery.value.copy(revision = _logsQuery.value.revision + 1)
    }

    // ------------------------------------------------------------- helpers
    /** Diagnóstico rápido antes de iniciar o patch. */
    private fun binariesProblem(): String? {
        val magiskboot = File(binariesDir, "magiskboot")
        val jbmInit = File(binariesDir, "jbm_init")
        return when {
            !magiskboot.isFile -> "magiskboot não instalado em ${magiskboot.absolutePath}. " +
                "Reinstale o APK (ou limpe os dados do app) e atualize a tela inicial."
            !magiskboot.canExecute() -> "magiskboot está sem permissão de execução. " +
                "Limpe os dados do app para que os binários sejam reinstalados."
            !jbmInit.isFile -> "jbm_init não instalado em ${jbmInit.absolutePath}. " +
                "Reinstale o APK (ou limpe os dados do app) e atualize a tela inicial."
            else -> null
        }
    }

    fun binariesSummary(): String {
        val list = _home.value.binaries
        if (list.isEmpty()) return "não verificado"
        val ok = list.count { it.state == BinaryState.OK || it.state == BinaryState.INSTALLED_NOW }
        return "$ok/${list.size} validados"
    }

    fun formatBytes(bytes: Long): String {
        if (bytes < 1024) return "$bytes B"
        val kb = bytes / 1024.0
        if (kb < 1024) return "${DecimalFormat("0.0").format(kb)} KiB"
        return "${DecimalFormat("0.00").format(kb / 1024.0)} MiB"
    }

    fun logsDirRef(): File = logsDir

    fun exportLogs(target: File, entries: List<LogEntry>, onDone: (String) -> Unit) {
        viewModelScope.launch(Dispatchers.IO) {
            val ok = try {
                target.parentFile?.mkdirs()
                target.bufferedWriter().use { w ->
                    entries.forEach { e ->
                        w.appendLine("[${e.formattedTime()}] ${e.level} ${e.source.label} — ${e.message}")
                        e.stdout?.let { w.appendLine("  stdout: ${it.replace("\n", "\n  ")}") }
                        e.stderr?.let { w.appendLine("  stderr: ${it.replace("\n", "\n  ")}") }
                    }
                }
                true
            } catch (_: Exception) {
                false
            }
            withContext(Dispatchers.Main) {
                onDone(if (ok) "Logs exportados: ${target.name}" else "Falha ao exportar logs")
            }
        }
    }
}

/** Filtro da tela de logs. */
data class LogFilter(
    val query: String = "",
    val levels: Set<LogLevel> = LogLevel.entries.toSet(),
    val sources: Set<LogSource> = LogSource.entries.toSet(),
    val revision: Int = 0,
)

fun List<LogEntry>.applyFilter(filter: LogFilter): List<LogEntry> {
    val q = filter.query.trim().lowercase()
    return filter { entry ->
        entry.level in filter.levels &&
            entry.source in filter.sources &&
            (q.isEmpty() || entry.message.lowercase().contains(q) ||
                entry.stdout?.lowercase()?.contains(q) == true ||
                entry.stderr?.lowercase()?.contains(q) == true)
    }.asReversed()
}
