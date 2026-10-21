package com.spacexjr.jupiter.core.update

import com.spacexjr.jupiter.core.log.OperationLog
import org.junit.Assert.assertEquals
import org.junit.Test

class GitHubReleaseClientTest {

    private val client = GitHubReleaseClient(OperationLog(null))

    @Test
    fun parsesTagNameAndAssets() {
        val json = """
            {
              "tag_name": "v1.2.0",
              "name": "JupiterBoot 1.2.0",
              "published_at": "2026-01-01T00:00:00Z",
              "body": "notes",
              "html_url": "https://github.com/spacexjr/JupiterBoot/releases/tag/v1.2.0",
              "assets": [
                {"name": "jbm_init", "size": 892968, "browser_download_url": "https://example/jbm_init"},
                {"name": "magiskboot", "size": 788840, "browser_download_url": "https://example/magiskboot"}
              ]
            }
        """.trimIndent()
        val release = client.parseRelease(json)
        assertEquals("v1.2.0", release.tagName)
        assertEquals("JupiterBoot 1.2.0", release.name)
        assertEquals(2, release.assets.size)
        assertEquals("jbm_init", release.assets[0].name)
        assertEquals(892968L, release.assets[0].size)
    }

    @Test
    fun handlesMissingAssetsArray() {
        val release = client.parseRelease("""{"tag_name":"v1.0.0"}""")
        assertEquals("v1.0.0", release.tagName)
        assertEquals(0, release.assets.size)
    }

    @Test
    fun fallsBackToRepositoryUrlWhenHtmlUrlMissing() {
        val release = client.parseRelease("""{"tag_name":"v1.0.0"}""")
        assertEquals(client.repositoryUrl, release.htmlUrl)
    }
}
