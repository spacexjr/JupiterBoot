package com.spacexjr.jupiter.core.shell

import com.spacexjr.jupiter.core.log.LogLevel
import com.spacexjr.jupiter.core.log.LogSource
import com.spacexjr.jupiter.core.log.OperationLog
import java.io.File
import java.io.IOException

/**
 * Serviço de operações privilegiadas.
 *
 * Detecta `su`, executa comandos de forma assíncrona com timeout e
 * cancelamento, captura stdout/stderr/código de saída e registra cada
 * operação no histórico de logs.
 */
class RootService(
    private val runner: CommandRunner,
    private val log: OperationLog,
) {

    @Volatile
    private var cachedRoot: Boolean? = null

    /** Executa argv diretamente (sem root). */
    suspend fun exec(
        argv: List<String>,
        timeoutMs: Long = DEFAULT_TIMEOUT_MS,
        workdir: File? = null,
        stdin: String? = null,
        stdinBytes: ByteArray? = null,
    ): CommandResult {
        val result = runner.runOrError(argv, timeoutMs, workdir, stdin, stdinBytes)
            .toCommandResult(argv)
        log.append(
            LogLevel.DEBUG,
            LogSource.PROCESS,
            result.describe(),
            stdout = result.stdout.takeIf { it.isNotBlank() },
            stderr = result.stderr.takeIf { it.isNotBlank() },
        )
        return result
    }

    /** Executa argv via `su -c` com quoting seguro. */
    suspend fun execRoot(
        argv: List<String>,
        timeoutMs: Long = DEFAULT_TIMEOUT_MS,
        stdin: String? = null,
        stdinBytes: ByteArray? = null,
    ): CommandResult {
        val suArgv = listOf("su", "-c", Sh.join(argv))
        val result = runner.runOrError(suArgv, timeoutMs, null, stdin, stdinBytes)
            .toCommandResult(suArgv)
        log.append(
            if (result.isSuccess) LogLevel.DEBUG else LogLevel.WARN,
            LogSource.ROOT,
            result.describe(),
            stdout = result.stdout.takeIf { it.isNotBlank() },
            stderr = result.stderr.takeIf { it.isNotBlank() },
            exitCode = result.exitCode,
        )
        return result
    }

    /** Retorna true quando `su` responde com uid=0. */
    suspend fun isRootAvailable(refresh: Boolean = false): Boolean {
        if (!refresh) cachedRoot?.let { return it }
        val result = exec(listOf("su", "-c", "id"), timeoutMs = 8_000)
        val ok = !result.timedOut && result.exitCode == 0 && result.stdout.contains("uid=0")
        cachedRoot = ok
        return ok
    }

    /** Lê um arquivo de sistema como root; null quando ausente/inacessível. */
    suspend fun readRootFile(path: String, timeoutMs: Long = 20_000): String? {
        Sh.validateSystemPath(path)
        val result = execRoot(listOf("cat", path), timeoutMs)
        return if (result.isSuccess) result.stdout else null
    }

    /** Lê um arquivo de sistema como root em bytes (seguro para binários). */
    suspend fun readRootFileBase64(path: String, timeoutMs: Long = 120_000): ByteArray? {
        Sh.validateSystemPath(path)
        val result = execRoot(listOf("base64", path), timeoutMs)
        if (!result.isSuccess) return null
        return try {
            java.util.Base64.getDecoder().decode(
                result.stdout.replace("\n", "").replace("\r", ""),
            )
        } catch (_: IllegalArgumentException) {
            null
        }
    }

    /** Escreve texto em caminho de sistema de forma atômica via stdin. */
    suspend fun writeRootFileAtomic(path: String, content: String, timeoutMs: Long = 30_000): Boolean {
        Sh.validateSystemPath(path)
        val tmp = "$path.jupiter.tmp"
        val script = "cat > ${Sh.quote(tmp)} && mv -f ${Sh.quote(tmp)} ${Sh.quote(path)}"
        val result = execRoot(listOf("sh", "-c", script), timeoutMs, stdin = content)
        return result.isSuccess
    }

    /** Escreve bytes em caminho de sistema de forma atômica via stdin. */
    suspend fun writeRootFileBytesAtomic(path: String, bytes: ByteArray, timeoutMs: Long = 120_000): Boolean {
        Sh.validateSystemPath(path)
        val tmp = "$path.jupiter.tmp"
        val script = "cat > ${Sh.quote(tmp)} && mv -f ${Sh.quote(tmp)} ${Sh.quote(path)}"
        val result = execRoot(listOf("sh", "-c", script), timeoutMs, stdinBytes = bytes)
        return result.isSuccess
    }

    /** Info de arquivo via root: null quando não existe. */
    suspend fun rootStat(path: String): RootStat? {
        Sh.validateSystemPath(path)
        val result = execRoot(listOf("stat", "-c", "%A|%s|%U:%G|%Y", path), timeoutMs = 10_000)
        if (!result.isSuccess) return null
        val parts = result.stdout.trim().split("|")
        if (parts.size != 4) return null
        return RootStat(
            mode = parts[0],
            size = parts[1].toLongOrNull() ?: -1L,
            owner = parts[2],
            modifiedEpochSec = parts[3].toLongOrNull() ?: -1L,
        )
    }

    companion object {
        const val DEFAULT_TIMEOUT_MS = 60_000L
    }
}

data class RootStat(
    val mode: String,
    val size: Long,
    val owner: String,
    val modifiedEpochSec: Long,
) {
    val isExecutable: Boolean get() = mode.length == 10 && mode.contains('x')
}

/** Exceção para falhas estruturais do serviço root. */
class RootException(message: String, cause: Throwable? = null) : IOException(message, cause)
