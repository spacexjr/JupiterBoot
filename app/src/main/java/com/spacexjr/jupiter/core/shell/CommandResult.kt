package com.spacexjr.jupiter.core.shell

/** Resultado de uma execucao de processo (direta ou via su). */
data class CommandResult(
    val command: List<String>,
    val exitCode: Int,
    val stdout: String,
    val stderr: String,
    val timedOut: Boolean,
    val durationMs: Long,
) {
    val isSuccess: Boolean get() = !timedOut && exitCode == 0

    fun describe(): String = buildString {
        append(command.joinToString(" "))
        append(" -> exit=")
        append(exitCode)
        if (timedOut) append(" (timeout)")
    }
}
