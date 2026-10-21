package com.spacexjr.jupiter.core.update

import com.spacexjr.jupiter.core.log.LogLevel
import com.spacexjr.jupiter.core.log.LogSource
import com.spacexjr.jupiter.core.log.OperationLog
import com.spacexjr.jupiter.core.util.Versions
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL

data class GitHubRelease(
    val tagName: String,
    val name: String,
    val publishedAt: String,
    val body: String,
    val htmlUrl: String,
    val assets: List<ReleaseAsset>,
)

data class ReleaseAsset(
    val name: String,
    val size: Long,
    val downloadUrl: String,
)

sealed class UpdateCheckResult {
    data class UpdateAvailable(val local: String, val remote: String, val release: GitHubRelease) : UpdateCheckResult()
    data class UpToDate(val local: String, val remote: String, val release: GitHubRelease) : UpdateCheckResult()
    data class NoReleases(val repositoryUrl: String) : UpdateCheckResult()
    data class NetworkError(val message: String) : UpdateCheckResult()
    object UnknownLocalVersion : UpdateCheckResult()
}

/**
 * Consulta a API oficial de releases do GitHub para o JupiterBoot.
 * Nunca executa arquivos baixados; apenas informa o estado da atualização.
 */
class GitHubReleaseClient(
    private val log: OperationLog,
    private val endpoint: String = "https://api.github.com/repos/spacexjr/JupiterBoot/releases/latest",
    val repositoryUrl: String = "https://github.com/spacexjr/JupiterBoot",
) {

    suspend fun checkForUpdate(localVersion: String?): UpdateCheckResult =
        withContext(Dispatchers.IO) {
            val release = try {
                fetchLatestRelease()
            } catch (e: IOException) {
                log.append(LogLevel.WARN, LogSource.UPDATE, "falha de rede: ${e.message}")
                return@withContext UpdateCheckResult.NetworkError(e.message ?: "erro de rede")
            } catch (e: Exception) {
                log.append(LogLevel.ERROR, LogSource.UPDATE, "resposta inválida: ${e.message}")
                return@withContext UpdateCheckResult.NetworkError("resposta inválida da API: ${e.message}")
            }

            if (localVersion.isNullOrBlank()) {
                return@withContext UpdateCheckResult.UnknownLocalVersion
            }
            if (Versions.isNewer(release.tagName, localVersion)) {
                log.append(
                    LogLevel.INFO, LogSource.UPDATE,
                    "atualização disponível: $localVersion -> ${release.tagName}",
                )
                UpdateCheckResult.UpdateAvailable(localVersion, release.tagName, release)
            } else {
                log.append(LogLevel.INFO, LogSource.UPDATE, "sem atualização: local=$localVersion remote=${release.tagName}")
                UpdateCheckResult.UpToDate(localVersion, release.tagName, release)
            }
        }

    suspend fun fetchLatestRelease(): GitHubRelease = withContext(Dispatchers.IO) {
        val conn = URL(endpoint).openConnection() as HttpURLConnection
        conn.connectTimeout = 15_000
        conn.readTimeout = 15_000
        conn.setRequestProperty("User-Agent", "JupiterApp/1.0")
        conn.setRequestProperty("Accept", "application/vnd.github+json")
        try {
            when (val code = conn.responseCode) {
                200 -> {
                    val text = conn.inputStream.bufferedReader().readText()
                    parseRelease(text)
                }
                404 -> throw NoReleasesException()
                else -> throw IOException("HTTP $code")
            }
        } finally {
            conn.disconnect()
        }
    }

    fun parseRelease(text: String): GitHubRelease {
        val o = JSONObject(text)
        val assetsArray = o.optJSONArray("assets")
        val assets = buildList {
            if (assetsArray != null) {
                for (i in 0 until assetsArray.length()) {
                    val a = assetsArray.getJSONObject(i)
                    add(
                        ReleaseAsset(
                            name = a.optString("name"),
                            size = a.optLong("size"),
                            downloadUrl = a.optString("browser_download_url"),
                        ),
                    )
                }
            }
        }
        return GitHubRelease(
            tagName = o.optString("tag_name"),
            name = o.optString("name"),
            publishedAt = o.optString("published_at"),
            body = o.optString("body"),
            htmlUrl = o.optString("html_url", repositoryUrl),
            assets = assets,
        )
    }

    class NoReleasesException : IOException("nenhum release publicado")
}
