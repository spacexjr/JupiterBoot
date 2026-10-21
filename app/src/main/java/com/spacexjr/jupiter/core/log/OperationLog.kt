package com.spacexjr.jupiter.core.log

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.ConcurrentLinkedDeque

enum class LogLevel { DEBUG, INFO, WARN, ERROR }

enum class LogSource(val label: String) {
    PROCESS("processo"),
    ROOT("root"),
    BINARIES("binários"),
    PATCH("patch"),
    CONFIG("config"),
    ENTRIES("entries"),
    FILES("arquivos"),
    UPDATE("github"),
    APP("app"),
}

data class LogEntry(
    val timestampMs: Long,
    val level: LogLevel,
    val source: LogSource,
    val message: String,
    val stdout: String? = null,
    val stderr: String? = null,
    val exitCode: Int? = null,
) {
    fun formattedTime(): String =
        SimpleDateFormat("dd/MM HH:mm:ss", Locale.getDefault()).format(Date(timestampMs))
}

/**
 * Histórico de operações do app. Em memória (StateFlow para a UI) e
 * persistido em arquivo de texto no diretório privado.
 */
class OperationLog(private val persistFile: File? = null) {

    private val deque = ConcurrentLinkedDeque<LogEntry>()
    private val _entries = MutableStateFlow<List<LogEntry>>(emptyList())
    val entries: StateFlow<List<LogEntry>> = _entries.asStateFlow()

    private val lock = Any()

    fun append(
        level: LogLevel,
        source: LogSource,
        message: String,
        stdout: String? = null,
        stderr: String? = null,
        exitCode: Int? = null,
    ) {
        val entry = LogEntry(
            timestampMs = System.currentTimeMillis(),
            level = level,
            source = source,
            message = message,
            stdout = stdout,
            stderr = stderr,
            exitCode = exitCode,
        )
        synchronized(lock) {
            deque.addLast(entry)
            while (deque.size > MAX_ENTRIES) deque.removeFirst()
            _entries.value = deque.toList()
            persistFile?.let { file ->
                try {
                    file.parentFile?.mkdirs()
                    file.appendText(entry.toLine() + "\n")
                } catch (_: Exception) {
                    // Persistência é best-effort; a UI continua com o histórico em memória.
                }
            }
        }
    }

    fun clear() {
        synchronized(lock) {
            deque.clear()
            _entries.value = emptyList()
        }
    }

    private fun LogEntry.toLine(): String {
        // Formato de uma linha: nível|origem|epochMs|exit|mensagem
        // stdout/stderr não são persistidos por linha (podem ser multilinha).
        return listOf(
            level.name,
            source.name,
            timestampMs.toString(),
            exitCode?.toString() ?: "-",
            message.replace('\n', ' ').replace('\r', ' '),
        ).joinToString("|")
    }

    companion object {
        const val MAX_ENTRIES = 2000
    }
}
