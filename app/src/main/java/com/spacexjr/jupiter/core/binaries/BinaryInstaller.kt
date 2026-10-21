package com.spacexjr.jupiter.core.binaries

import com.spacexjr.jupiter.core.log.LogLevel
import com.spacexjr.jupiter.core.log.LogSource
import com.spacexjr.jupiter.core.log.OperationLog
import com.spacexjr.jupiter.core.shell.CommandRunner
import com.spacexjr.jupiter.core.shell.runOrError
import com.spacexjr.jupiter.core.util.Elf
import com.spacexjr.jupiter.core.util.Hashes
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.File

data class BinarySpec(
    val name: String,
    val asset: String,
    val file: String,
    val version: String,
    val size: Long,
    val sha256: String,
    val executable: Boolean,
    val license: String,
)

enum class BinaryState {
    /** Extraído, hash conferem e ELF válido. */
    OK,

    /** Arquivo novo extraído agora. */
    INSTALLED_NOW,

    /** Arquivo já existia com hash diferente (nunca substituído). */
    MODIFIED_LOCALLY,

    /** Falha na extração ou validação. */
    ERROR,

    /** Ainda não instalado. */
    MISSING,
}

data class BinaryStatus(
    val spec: BinarySpec,
    val state: BinaryState,
    val path: File?,
    val detail: String,
    val elfDescription: String? = null,
    val executableNow: Boolean = false,
)

/**
 * Instala os binários ARM64 dos assets para o diretório privado do app.
 *
 * Regras:
 * - na primeira instalação, copia, valida ELF AArch64, valida SHA-256 e
 *   define permissão executável;
 * - arquivos existentes nunca são substituídos silenciosamente: se o hash
 *   divergir, o estado MODIFIED é reportado e o arquivo é mantido;
 * - magiskboot tem sua execução verificada (invocação sem argumentos deve
 *   produzir saída de uso); jbm_init não é executado no Android (é um
 *   /init freestanding que assumiria o sistema) — apenas ELF/hash.
 */
class BinaryInstaller(
    private val assets: AssetReader,
    private val targetDir: File,
    private val runner: CommandRunner,
    private val log: OperationLog,
) {

    interface AssetReader {
        fun open(assetPath: String): java.io.InputStream?
    }

    suspend fun manifest(): List<BinarySpec> {
        val json = withContext(Dispatchers.IO) {
            assets.open("binaries.json")?.use { it.readBytes().decodeToString() }
        } ?: return emptyList()
        return parseManifest(json)
    }

    suspend fun ensureInstalled(): List<BinaryStatus> {
        val specs = manifest()
        if (specs.isEmpty()) {
            log.append(LogLevel.ERROR, LogSource.BINARIES, "binaries.json ausente ou ilegível")
            return emptyList()
        }
        withContext(Dispatchers.IO) { targetDir.mkdirs() }
        return specs.map { ensureOne(it) }
    }

    private suspend fun ensureOne(spec: BinarySpec): BinaryStatus {
        val target = File(targetDir, spec.file)

        val usable = withContext(Dispatchers.IO) { target.isFile && target.length() > 0L }
        if (!usable) {
            withContext(Dispatchers.IO) { target.delete() }
            val installed = install(spec, target)
            return installed ?: BinaryStatus(
                spec = spec,
                state = BinaryState.ERROR,
                path = target,
                detail = "falha ao extrair ${spec.asset}",
            )
        }

        // Arquivo existente: validar sem substituir.
        val hash = withContext(Dispatchers.IO) {
            if (target.isFile) Hashes.sha256Hex(target) else null
        }
        val elf = withContext(Dispatchers.IO) { if (target.isFile) Elf.inspect(target) else null }

        val hashOk = hash != null && hash.equals(spec.sha256, ignoreCase = true)
        val elfOk = elf?.isAarch64 == true

        return when {
            hashOk && elfOk -> {
                if (spec.executable && !withContext(Dispatchers.IO) { target.canExecute() }) {
                    withContext(Dispatchers.IO) { target.setExecutable(true, false) }
                }
                val execNow = validateExecution(spec, target)
                log.append(
                    LogLevel.INFO, LogSource.BINARIES,
                    "${spec.name}: já instalado e verificado (${spec.version})",
                )
                BinaryStatus(
                    spec = spec,
                    state = BinaryState.OK,
                    path = target,
                    detail = if (execNow || spec.name != "magiskboot") "hash e ELF conferem"
                    else "hash e ELF conferem, mas a execução falhou",
                    elfDescription = elf?.describe,
                    executableNow = execNow,
                )
            }
            !elfOk -> {
                log.append(
                    LogLevel.WARN, LogSource.BINARIES,
                    "${spec.name}: ELF inválido (${elf?.describe}); removendo e reinstalando do asset",
                )
                withContext(Dispatchers.IO) { target.delete() }
                install(spec, target) ?: BinaryStatus(
                    spec = spec,
                    state = BinaryState.ERROR,
                    path = target,
                    detail = "ELF inválido (${elf?.describe}) e falha ao reinstalar do asset",
                    elfDescription = elf?.describe,
                )
            }
            !hashOk -> {
                log.append(
                    LogLevel.WARN, LogSource.BINARIES,
                    "${spec.name}: arquivo local difere do manifesto (mantido, sem overwrite)",
                )
                BinaryStatus(
                    spec = spec,
                    state = BinaryState.MODIFIED_LOCALLY,
                    path = target,
                    detail = "hash local ${hash ?: "?"} != esperado ${spec.sha256.take(16)}… — arquivo mantido",
                    elfDescription = elf?.describe,
                )
            }
            else -> BinaryStatus(
                spec = spec,
                state = BinaryState.ERROR,
                path = target,
                detail = "falha desconhecida ao validar o binário",
                elfDescription = elf?.describe,
            )
        }
    }

    private suspend fun install(spec: BinarySpec, target: File): BinaryStatus? {
        val bytes = withContext(Dispatchers.IO) {
            try {
                assets.open(spec.asset)?.use { it.readBytes() }
            } catch (_: Exception) {
                null
            }
        }
        if (bytes == null) {
            log.append(LogLevel.ERROR, LogSource.BINARIES, "asset ${spec.asset} não encontrado")
            return null
        }
        val hash = Hashes.sha256Hex(bytes)
        if (!hash.equals(spec.sha256, ignoreCase = true)) {
            log.append(
                LogLevel.ERROR, LogSource.BINARIES,
                "${spec.name}: hash do asset não confere (${hash.take(16)}…)",
            )
            return BinaryStatus(
                spec = spec, state = BinaryState.ERROR, path = target,
                detail = "hash do asset não confere",
            )
        }
        val elf = Elf.inspect(bytes)
        if (!elf.isAarch64) {
            log.append(LogLevel.ERROR, LogSource.BINARIES, "${spec.name}: ELF não é AArch64")
            return BinaryStatus(
                spec = spec, state = BinaryState.ERROR, path = target,
                detail = "ELF não é AArch64: ${elf.describe}",
            )
        }
        val ok = withContext(Dispatchers.IO) {
            val tmp = File(target.parentFile, target.name + ".tmp")
            try {
                target.parentFile?.mkdirs()
                tmp.writeBytes(bytes)
                if (spec.executable) tmp.setExecutable(true, false)
                if (!tmp.renameTo(target)) {
                    target.delete()
                    tmp.renameTo(target)
                }
                target.setReadable(true, false)
                if (spec.executable) target.setExecutable(true, false)
                target.isFile && target.length() == bytes.size.toLong()
            } catch (_: Exception) {
                tmp.delete()
                false
            }
        }
        if (!ok) {
            log.append(LogLevel.ERROR, LogSource.BINARIES, "${spec.name}: falha ao gravar em $target")
            return null
        }
        log.append(
            LogLevel.INFO, LogSource.BINARIES,
            "${spec.name}: instalado (${bytes.size} bytes, ${spec.version})",
        )
        val execNow = validateExecution(spec, target)
        return BinaryStatus(
            spec = spec,
            state = BinaryState.INSTALLED_NOW,
            path = target,
            detail = "extraído e validado",
            elfDescription = elf.describe,
            executableNow = execNow,
        )
    }

    /**
     * Verifica que o binário pode ser executado neste aparelho.
     * magiskboot: invocação sem argumentos deve retornar o texto de uso.
     * jbm_init: NÃO executado (assumiria /init); apenas ELF validado.
     */
    private suspend fun validateExecution(spec: BinarySpec, target: File): Boolean {
        if (spec.name != "magiskboot") return false
        if (!target.isFile || !target.canExecute()) {
            log.append(
                LogLevel.ERROR, LogSource.BINARIES,
                "magiskboot exec test: arquivo ausente ou sem +x em $target",
            )
            return false
        }
        val result = runner.runOrError(listOf(target.absolutePath), timeoutMs = 10_000)
        val ran = result.stdout.contains("MagiskBoot", ignoreCase = true) ||
            result.stderr.contains("MagiskBoot", ignoreCase = true) ||
            (!result.timedOut && result.exitCode != 126 && result.exitCode != 127 && result.exitCode != -1)
        log.append(
            if (ran) LogLevel.INFO else LogLevel.ERROR,
            LogSource.BINARIES,
            "magiskboot exec test: exit=${result.exitCode} timedOut=${result.timedOut}" +
                if (ran) "" else " stderr=${result.stderr.take(300)}",
        )
        return ran
    }

    companion object {
        fun parseManifest(json: String): List<BinarySpec> {
            val root = JSONObject(json)
            val array = root.getJSONArray("binaries")
            return (0 until array.length()).map { i ->
                val o = array.getJSONObject(i)
                BinarySpec(
                    name = o.getString("name"),
                    asset = o.getString("asset"),
                    file = o.getString("file"),
                    version = o.getString("version"),
                    size = o.getLong("size"),
                    sha256 = o.getString("sha256"),
                    executable = o.optBoolean("executable", true),
                    license = o.optString("license", ""),
                )
            }
        }

        fun jupiterBootVersion(manifestJson: String): String? =
            try {
                JSONObject(manifestJson).optString("jupiterBootVersion").ifEmpty { null }
            } catch (_: Exception) {
                null
            }
    }
}
