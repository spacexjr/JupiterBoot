package com.spacexjr.jupiter.core.patch

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.File

/** Armazena o último relatório de patch no armazenamento privado. */
class PatchStateStore(private val file: File) {

    suspend fun save(report: PatchReport) = withContext(Dispatchers.IO) {
        try {
            file.parentFile?.mkdirs()
            file.writeText(toJson(report))
            true
        } catch (_: Exception) {
            false
        }
    }

    suspend fun load(): PatchReport? = withContext(Dispatchers.IO) {
        try {
            if (!file.isFile) return@withContext null
            fromJson(file.readText())
        } catch (_: Exception) {
            null
        }
    }

    fun toJson(r: PatchReport): String {
        val o = JSONObject()
        o.put("startedAtMs", r.startedAtMs)
        o.put("finishedAtMs", r.finishedAtMs)
        o.put("partition", r.target.partition.name)
        o.put("partitionPath", r.target.partition.linkPath)
        o.put("kind", r.target.kind.name)
        o.put("rationale", r.target.rationale)
        o.put("confidence", r.target.confidence.name)
        r.image?.let {
            o.put("imageSize", it.sizeBytes)
            o.put("imageSha256", it.sha256)
        }
        r.repack?.let {
            o.put("outputPath", it.outputFile.absolutePath)
            o.put("outputSize", it.outputSizeBytes)
            o.put("outputSha256", it.outputSha256)
            o.put("verified", it.verified)
        }
        o.put("failedStep", r.failedStep ?: JSONObject.NULL)
        o.put("failureReason", r.failureReason ?: JSONObject.NULL)
        o.put("success", r.success)
        o.put("warnings", org.json.JSONArray(r.warnings as Collection<*>))
        return o.toString(2)
    }

    fun fromJson(text: String): PatchReport? {
        val o = JSONObject(text)
        val kind = try {
            TargetKind.valueOf(o.optString("kind", "BOOT"))
        } catch (_: Exception) {
            TargetKind.BOOT
        }
        val confidence = try {
            Confidence.valueOf(o.optString("confidence", "LOW"))
        } catch (_: Exception) {
            Confidence.LOW
        }
        val target = PatchTarget(
            partition = PartitionInfo(o.optString("partition"), o.optString("partitionPath")),
            kind = kind,
            rationale = o.optString("rationale"),
            confidence = confidence,
        )
        val warningsArray = o.optJSONArray("warnings")
        val warnings = buildList {
            if (warningsArray != null) for (i in 0 until warningsArray.length()) add(warningsArray.getString(i))
        }
        val repack = if (o.has("outputSha256") && !o.isNull("outputSha256")) {
            RepackResult(
                exitCode = 0,
                output = "",
                outputFile = File(o.optString("outputPath")),
                outputSizeBytes = o.optLong("outputSize"),
                originalSizeBytes = o.optLong("imageSize"),
                outputSha256 = o.optString("outputSha256"),
                ramdiskMatchesStaged = true,
                initMatchesJbm = true,
                sizeUnchanged = true,
                verified = o.optBoolean("verified", false),
            )
        } else null
        return PatchReport(
            startedAtMs = o.optLong("startedAtMs"),
            finishedAtMs = o.optLong("finishedAtMs"),
            target = target,
            image = if (o.has("imageSha256") && !o.isNull("imageSha256")) {
                ExtractedImage(
                    partition = target.partition,
                    file = File(o.optString("outputPath")).parentFile?.let { File(it, "boot-stock.img") }
                        ?: File("boot-stock.img"),
                    sizeBytes = o.optLong("imageSize"),
                    expectedSizeBytes = null,
                    sha256 = o.optString("imageSha256"),
                    ddOutput = "",
                )
            } else null,
            unpack = null,
            ramdisk = null,
            install = null,
            repack = repack,
            warnings = warnings,
            failedStep = if (o.isNull("failedStep")) null else o.optString("failedStep"),
            failureReason = if (o.isNull("failureReason")) null else o.optString("failureReason"),
        )
    }
}
