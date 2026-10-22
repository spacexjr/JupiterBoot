package com.spacexjr.jupiter.core.update

import com.spacexjr.jupiter.core.log.LogLevel
import com.spacexjr.jupiter.core.log.LogSource
import com.spacexjr.jupiter.core.log.OperationLog
import com.spacexjr.jupiter.core.util.Elf
import com.spacexjr.jupiter.core.util.Hashes
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL

sealed class UpdateInstallResult {
    data class Installed(
        val version: String,
        val path: File,
        val sha256: String,
        val hashVerified: Boolean,
        val backup: File?,
    ) : UpdateInstallResult()

    data class Failed(val message: String) : UpdateInstallResult()
}

/**
 * Baixa e instala o asset `jbm_init` de um release do GitHub.
 *
 * Garantias:
 * - somente HTTPS em dominios do GitHub;
 * - nome do asset exatamente `jbm_init`;
 * - limite de tamanho e validacao ELF AArch64;
 * - hash conferido quando o release publica `jbm_init.sha256`;
 * - arquivo anterior preservado como backup; nada e executado.
 */
class UpdateInstaller(
    private val log: OperationLog,
    private val maxBytes: Long = 32L * 1024 * 1024,
) {

    suspend fun install(release: GitHubRelease, targetDir: File): UpdateInstallResult =
        withContext(Dispatchers.IO) {
            val asset = release.assets.firstOrNull { it.name == "jbm_init" }
                ?: return@withContext UpdateInstallResult.Failed(
                    "release ${release.tagName} nao publica o asset jbm_init",
                )
            if (!trustedUrl(asset.downloadUrl)) {
                return@withContext UpdateInstallResult.Failed("URL do asset nao e do GitHub")
            }
            if (asset.size <= 0 || asset.size > maxBytes) {
                return@withContext UpdateInstallResult.Failed("tamanho do asset invalido: ${asset.size}")
            }
            val shaAsset = release.assets.firstOrNull { it.name == "jbm_init.sha256" }
            val expectedHash = if (shaAsset != null && trustedUrl(shaAsset.downloadUrl)) {
                downloadText(shaAsset.downloadUrl)?.trim()?.split(Regex("\\s+"))?.firstOrNull()
            } else {
                null
            }
            val bytes = try {
                download(asset.downloadUrl)
            } catch (e: IOException) {
                log.append(LogLevel.ERROR, LogSource.UPDATE, "download do jbm_init falhou: ${e.message}")
                return@withContext UpdateInstallResult.Failed(e.message ?: "falha de download")
            }
            val hash = Hashes.sha256Hex(bytes)
            val hashVerified = expectedHash != null
            if (expectedHash != null && !hash.equals(expectedHash, ignoreCase = true)) {
                log.append(LogLevel.ERROR, LogSource.UPDATE, "sha256 do jbm_init nao confere")
                return@withContext UpdateInstallResult.Failed("sha256 do jbm_init nao confere")
            }
            if (expectedHash == null) {
                log.append(
                    LogLevel.WARN, LogSource.UPDATE,
                    "release sem jbm_init.sha256; apenas ELF/size foram verificados",
                )
            }
            val elf = Elf.inspect(bytes)
            if (!elf.isAarch64) {
                return@withContext UpdateInstallResult.Failed("jbm_init nao e um ELF AArch64: ${elf.describe}")
            }
            val target = File(targetDir, "jbm_init")
            targetDir.mkdirs()
            val backup = if (target.isFile) {
                File(targetDir, "jbm_init.bak-${System.currentTimeMillis()}").also {
                    target.copyTo(it, overwrite = true)
                }
            } else null
            val tmp = File(targetDir, "jbm_init.download")
            try {
                tmp.writeBytes(bytes)
                tmp.setReadable(true, false)
                tmp.setExecutable(true, false)
                if (!tmp.renameTo(target)) {
                    target.delete()
                    if (!tmp.renameTo(target)) throw IOException("rename falhou")
                }
            } catch (e: Exception) {
                tmp.delete()
                log.append(LogLevel.ERROR, LogSource.UPDATE, "instalacao do jbm_init falhou: ${e.message}")
                return@withContext UpdateInstallResult.Failed("falha ao gravar jbm_init: ${e.message}")
            }
            log.append(
                LogLevel.INFO, LogSource.UPDATE,
                "jbm_init ${release.tagName} instalado (${bytes.size} bytes, sha=$hash)",
            )
            UpdateInstallResult.Installed(
                version = release.tagName,
                path = target,
                sha256 = hash,
                hashVerified = hashVerified,
                backup = backup,
            )
        }

    private fun trustedUrl(url: String): Boolean {
        return try {
            val u = URL(url)
            u.protocol == "https" &&
                (u.host == "github.com" || u.host.endsWith(".github.com") ||
                    u.host == "objects.githubusercontent.com" ||
                    u.host.endsWith(".githubusercontent.com"))
        } catch (_: Exception) {
            false
        }
    }

    private fun download(url: String): ByteArray {
        val conn = URL(url).openConnection() as HttpURLConnection
        conn.connectTimeout = 20_000
        conn.readTimeout = 30_000
        conn.setRequestProperty("User-Agent", "JupiterApp/1.0")
        conn.instanceFollowRedirects = true
        try {
            if (conn.responseCode != 200) throw IOException("HTTP ${conn.responseCode}")
            val length = conn.contentLengthLong
            if (length > maxBytes) throw IOException("asset grande demais: $length")
            conn.inputStream.use { input ->
                val out = java.io.ByteArrayOutputStream()
                val buf = ByteArray(64 * 1024)
                var total = 0L
                while (true) {
                    val n = input.read(buf)
                    if (n < 0) break
                    total += n
                    if (total > maxBytes) throw IOException("asset grande demais")
                    out.write(buf, 0, n)
                }
                return out.toByteArray()
            }
        } finally {
            conn.disconnect()
        }
    }

    private fun downloadText(url: String): String? = try {
        download(url).decodeToString()
    } catch (_: Exception) {
        null
    }
}
