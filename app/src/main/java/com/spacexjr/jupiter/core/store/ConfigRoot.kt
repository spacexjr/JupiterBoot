package com.spacexjr.jupiter.core.store

import com.spacexjr.jupiter.core.shell.RootService
import com.spacexjr.jupiter.core.shell.Sh
import com.spacexjr.jupiter.core.util.Elf
import com.spacexjr.jupiter.core.util.Hashes
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File

/** Entrada de diretorio listada pelo store. */
data class StoreEntry(
    val name: String,
    val isDirectory: Boolean,
)

/** Metadados de arquivo. */
data class StoreStat(
    val size: Long,
    val mode: String,
    val owner: String,
    val modifiedEpochSec: Long,
) {
    val isExecutable: Boolean get() = mode.length >= 7 && mode.substring(7).contains('x')
}

/**
 * Abstracao de raiz de configuracao do JupiterBoot.
 *
 * Duas implementacoes:
 * - [DeviceConfigRoot]: /cache/Jupiter no aparelho (via su);
 * - [LocalConfigRoot]: copia de teste no diretorio privado do app.
 */
interface ConfigRoot {
    val id: String
    val displayPath: String

    suspend fun exists(relPath: String): Boolean
    suspend fun readText(relPath: String): String?
    suspend fun writeAtomic(relPath: String, content: String): Boolean
    suspend fun listDirectory(relPath: String): List<StoreEntry>
    suspend fun stat(relPath: String): StoreStat?
    suspend fun delete(relPath: String): Boolean
    suspend fun makeDirs(relPath: String): Boolean
    suspend fun copyIntoAppFile(relPath: String, sourceFile: File): Boolean
    suspend fun exportToFile(relPath: String, target: File): Boolean
    suspend fun backup(relPath: String): String?
    suspend fun listBackups(relPath: String): List<String>
    suspend fun restoreBackup(relPath: String, backupName: String): Boolean
    suspend fun chmod(relPath: String, mode: String): Boolean
    suspend fun sha256(relPath: String): String?

    companion object {
        const val DEVICE_ID = "device"
        const val LOCAL_ID = "local"
    }
}

/** Raiz no aparelho: /cache/Jupiter via comandos root. */
class DeviceConfigRoot(
    private val root: RootService,
    val basePath: String = "/cache/Jupiter",
) : ConfigRoot {

    override val id: String = ConfigRoot.DEVICE_ID
    override val displayPath: String = basePath

    private fun abs(relPath: String): String {
        val clean = relPath.trimStart('/')
        return if (clean.isEmpty()) basePath
        else Sh.validateSystemPath("$basePath/$clean")
    }

    override suspend fun exists(relPath: String): Boolean =
        root.rootStat(abs(relPath)) != null

    override suspend fun readText(relPath: String): String? =
        root.readRootFile(abs(relPath))

    override suspend fun writeAtomic(relPath: String, content: String): Boolean =
        root.writeRootFileAtomic(abs(relPath), content)

    override suspend fun listDirectory(relPath: String): List<StoreEntry> {
        val result = root.execRoot(listOf("ls", "-1A", abs(relPath)), timeoutMs = 10_000)
        if (!result.isSuccess) return emptyList()
        val dirs = root.execRoot(listOf("find", abs(relPath), "-maxdepth", "1", "-mindepth", "1", "-type", "d"), 10_000)
        val dirNames = dirs.stdout.lines().filter { it.isNotBlank() }
            .map { it.substringAfterLast('/') }.toSet()
        return result.stdout.lines().filter { it.isNotBlank() }
            .map { StoreEntry(it, it in dirNames) }
            .sortedWith(compareByDescending<StoreEntry> { it.isDirectory }.thenBy { it.name })
    }

    override suspend fun stat(relPath: String): StoreStat? {
        val s = root.rootStat(abs(relPath)) ?: return null
        return StoreStat(s.size, s.mode, s.owner, s.modifiedEpochSec)
    }

    override suspend fun delete(relPath: String): Boolean =
        root.execRoot(listOf("rm", "-f", abs(relPath))).isSuccess

    override suspend fun makeDirs(relPath: String): Boolean =
        root.execRoot(listOf("mkdir", "-p", abs(relPath))).isSuccess

    override suspend fun copyIntoAppFile(relPath: String, sourceFile: File): Boolean {
        val bytes = withContext(Dispatchers.IO) { sourceFile.readBytes() }
        return root.writeRootFileBytesAtomic(abs(relPath), bytes)
    }

    override suspend fun exportToFile(relPath: String, target: File): Boolean {
        val bytes = root.readRootFileBase64(abs(relPath)) ?: return false
        return withContext(Dispatchers.IO) {
            try {
                target.parentFile?.mkdirs()
                target.writeBytes(bytes)
                true
            } catch (_: Exception) {
                false
            }
        }
    }

    override suspend fun backup(relPath: String): String? {
        val stamp = System.currentTimeMillis()
        val backupName = "$relPath.bak-$stamp"
        val ok = root.execRoot(
            listOf("cp", "-p", abs(relPath), abs(backupName)),
        ).isSuccess
        return if (ok) backupName else null
    }

    override suspend fun listBackups(relPath: String): List<String> {
        val dir = abs(relPath).substringBeforeLast('/', basePath)
        val base = relPath.substringAfterLast('/')
        val result = root.execRoot(listOf("ls", "-1A", dir), timeoutMs = 10_000)
        if (!result.isSuccess) return emptyList()
        return result.stdout.lines()
            .filter { it.startsWith("$base.bak-") }
            .sortedDescending()
    }

    override suspend fun restoreBackup(relPath: String, backupName: String): Boolean =
        root.execRoot(listOf("cp", "-p", abs(backupName), abs(relPath))).isSuccess

    override suspend fun chmod(relPath: String, mode: String): Boolean {
        require(Regex("^[0-7]{3,4}$").matches(mode)) { "modo invalido: $mode" }
        return root.execRoot(listOf("chmod", mode, abs(relPath))).isSuccess
    }

    override suspend fun sha256(relPath: String): String? {
        val result = root.execRoot(listOf("sha256sum", abs(relPath)), timeoutMs = 120_000)
        if (!result.isSuccess) return null
        return result.stdout.trim().split(Regex("\\s+")).firstOrNull()?.lowercase()
    }
}

/** Raiz de teste local (copia de arquivos no diretorio privado do app). */
class LocalConfigRoot(baseDir: File) : ConfigRoot {

    private val base: File = baseDir

    override val id: String = ConfigRoot.LOCAL_ID
    override val displayPath: String = base.absolutePath

    private fun file(relPath: String): File {
        val clean = relPath.trimStart('/')
        val f = if (clean.isEmpty()) base else File(base, clean)
        val canonicalBase = base.canonicalFile
        val canonical = f.canonicalFile
        require(canonical.path == canonicalBase.path || canonical.path.startsWith(canonicalBase.path + File.separator)) {
            "caminho fora da raiz local"
        }
        return f
    }

    override suspend fun exists(relPath: String): Boolean = withContext(Dispatchers.IO) {
        file(relPath).exists()
    }

    override suspend fun readText(relPath: String): String? = withContext(Dispatchers.IO) {
        val f = file(relPath)
        if (f.isFile) f.readText() else null
    }

    override suspend fun writeAtomic(relPath: String, content: String): Boolean =
        withContext(Dispatchers.IO) {
            try {
                val f = file(relPath)
                f.parentFile?.mkdirs()
                val tmp = File(f.parentFile, f.name + ".tmp")
                tmp.writeText(content)
                if (!tmp.renameTo(f)) {
                    f.writeText(content)
                    tmp.delete()
                }
                true
            } catch (_: Exception) {
                false
            }
        }

    override suspend fun listDirectory(relPath: String): List<StoreEntry> = withContext(Dispatchers.IO) {
        val f = file(relPath)
        if (!f.isDirectory) return@withContext emptyList()
        f.listFiles()?.map { StoreEntry(it.name, it.isDirectory) }
            ?.sortedWith(compareByDescending<StoreEntry> { it.isDirectory }.thenBy { it.name })
            ?: emptyList()
    }

    override suspend fun stat(relPath: String): StoreStat? = withContext(Dispatchers.IO) {
        val f = file(relPath)
        if (!f.exists()) return@withContext null
        StoreStat(
            size = f.length(),
            mode = if (f.canExecute()) "-rwxr-xr-x" else "-rw-r--r--",
            owner = "app",
            modifiedEpochSec = f.lastModified() / 1000,
        )
    }

    override suspend fun delete(relPath: String): Boolean = withContext(Dispatchers.IO) {
        file(relPath).delete()
    }

    override suspend fun makeDirs(relPath: String): Boolean = withContext(Dispatchers.IO) {
        file(relPath).mkdirs()
    }

    override suspend fun copyIntoAppFile(relPath: String, sourceFile: File): Boolean =
        withContext(Dispatchers.IO) {
            try {
                val f = file(relPath)
                f.parentFile?.mkdirs()
                sourceFile.copyTo(f, overwrite = true)
                true
            } catch (_: Exception) {
                false
            }
        }

    override suspend fun exportToFile(relPath: String, target: File): Boolean =
        withContext(Dispatchers.IO) {
            try {
                val f = file(relPath)
                if (!f.isFile) return@withContext false
                target.parentFile?.mkdirs()
                f.copyTo(target, overwrite = true)
                true
            } catch (_: Exception) {
                false
            }
        }

    override suspend fun backup(relPath: String): String? = withContext(Dispatchers.IO) {
        val f = file(relPath)
        if (!f.isFile) return@withContext null
        val stamp = System.currentTimeMillis()
        val name = "$relPath.bak-$stamp"
        val target = file(name)
        try {
            f.copyTo(target, overwrite = true)
            name
        } catch (_: Exception) {
            null
        }
    }

    override suspend fun listBackups(relPath: String): List<String> = withContext(Dispatchers.IO) {
        val dir = file(relPath).parentFile ?: return@withContext emptyList()
        val base = relPath.substringAfterLast('/')
        dir.listFiles()
            ?.map { it.name }
            ?.filter { it.startsWith("$base.bak-") }
            ?.sortedDescending()
            ?: emptyList()
    }

    override suspend fun restoreBackup(relPath: String, backupName: String): Boolean =
        withContext(Dispatchers.IO) {
            try {
                val src = file(backupName)
                if (!src.isFile) return@withContext false
                val dst = file(relPath)
                src.copyTo(dst, overwrite = true)
                true
            } catch (_: Exception) {
                false
            }
        }

    override suspend fun chmod(relPath: String, mode: String): Boolean =
        withContext(Dispatchers.IO) {
            file(relPath).setExecutable(true, false)
        }

    override suspend fun sha256(relPath: String): String? = withContext(Dispatchers.IO) {
        val f = file(relPath)
        if (!f.isFile) return@withContext null
        Hashes.sha256Hex(f)
    }
}

/** Detalhes extras para exibicao de arquivos (ELF etc.). */
suspend fun ConfigRoot.describeBinary(relPath: String): String? {
    val stat = stat(relPath) ?: return null
    val tmp = File.createTempFile("jup-elf", ".bin")
    return try {
        val exported = exportToFile(relPath, tmp)
        if (exported && tmp.length() > 0) {
            val elf = Elf.inspect(tmp)
            "${stat.size} bytes · ${elf.describe} · ${stat.mode}"
        } else {
            "${stat.size} bytes · ${stat.mode}"
        }
    } finally {
        tmp.delete()
    }
}
