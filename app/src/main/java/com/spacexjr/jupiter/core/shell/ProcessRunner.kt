package com.spacexjr.jupiter.core.shell

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeoutOrNull
import java.io.File
import java.io.IOException

/** Abstração de execução de processos — permite testes com implementação fake. */
interface CommandRunner {
    suspend fun run(
        argv: List<String>,
        timeoutMs: Long = 60_000,
        workdir: File? = null,
        stdin: String? = null,
        stdinBytes: ByteArray? = null,
    ): CommandRunner.Result

    data class Result(
        val exitCode: Int,
        val stdout: String,
        val stderr: String,
        val timedOut: Boolean,
        val durationMs: Long,
    ) {
        val isSuccess: Boolean get() = !timedOut && exitCode == 0

        fun toCommandResult(argv: List<String>) = CommandResult(
            command = argv,
            exitCode = exitCode,
            stdout = stdout,
            stderr = stderr,
            timedOut = timedOut,
            durationMs = durationMs,
        )
    }
}

/** Execução real de processos via ProcessBuilder, sem shell. */
class ProcessCommandRunner : CommandRunner {

    override suspend fun run(
        argv: List<String>,
        timeoutMs: Long,
        workdir: File?,
        stdin: String?,
        stdinBytes: ByteArray?,
    ): CommandRunner.Result = coroutineScope {
        require(argv.isNotEmpty()) { "argv vazio" }
        withContext(Dispatchers.IO) {
            val start = System.currentTimeMillis()
            val process = ProcessBuilder(argv)
                .apply { if (workdir != null) directory(workdir) }
                .redirectErrorStream(false)
                .start()

            val stdoutJob = async { process.inputStream.bufferedReader().readText() }
            val stderrJob = async { process.errorStream.bufferedReader().readText() }

            when {
                stdinBytes != null -> process.outputStream.use { it.write(stdinBytes) }
                stdin != null -> process.outputStream.bufferedWriter().use { it.write(stdin) }
                else -> process.outputStream.close()
            }

            val finished = withTimeoutOrNull(timeoutMs) { process.waitFor() }
            if (finished == null) {
                process.destroy()
                withTimeoutOrNull(2_000) { process.waitFor() }
                process.destroyForcibly()
            }
            val stdout = stdoutJob.await()
            val stderr = stderrJob.await()
            CommandRunner.Result(
                exitCode = if (finished == null) -1 else process.exitValue(),
                stdout = stdout,
                stderr = stderr,
                timedOut = finished == null,
                durationMs = System.currentTimeMillis() - start,
            )
        }
    }
}

/** Converte falhas de I/O do processo em resultado com erro claro. */
suspend fun CommandRunner.runOrError(
    argv: List<String>,
    timeoutMs: Long = 60_000,
    workdir: File? = null,
    stdin: String? = null,
    stdinBytes: ByteArray? = null,
): CommandRunner.Result {
    try {
        return run(argv, timeoutMs, workdir, stdin, stdinBytes)
    } catch (e: IOException) {
        return CommandRunner.Result(
            exitCode = -1,
            stdout = "",
            stderr = e.message ?: e.javaClass.simpleName,
            timedOut = false,
            durationMs = 0,
        )
    }
}

/** Adapta um CommandResult (API de alto nível) para CommandRunner.Result. */
fun CommandResult.toRunnerResult() = CommandRunner.Result(
    exitCode = exitCode,
    stdout = stdout,
    stderr = stderr,
    timedOut = timedOut,
    durationMs = durationMs,
)
