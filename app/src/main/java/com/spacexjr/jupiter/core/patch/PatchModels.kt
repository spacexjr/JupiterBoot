package com.spacexjr.jupiter.core.patch

import java.io.File

/** Partição de boot identificada no aparelho. */
data class PartitionInfo(
    val name: String,
    val linkPath: String,
)

/** Alvo do patch com a justificativa da escolha. */
data class PatchTarget(
    val partition: PartitionInfo,
    val kind: TargetKind,
    val rationale: String,
    val confidence: Confidence,
)

enum class TargetKind { BOOT, INIT_BOOT, VENDOR_BOOT }

enum class Confidence { HIGH, MEDIUM, LOW }

/** Resultado do scan de partições. */
data class ScanResult(
    val partitions: List<PartitionInfo>,
    val slotSuffix: String,
    val device: String,
    val model: String,
    val platform: String,
    val chosen: PatchTarget?,
    val notes: List<String>,
)

/** Imagem extraída da partição. */
data class ExtractedImage(
    val partition: PartitionInfo,
    val file: File,
    val sizeBytes: Long,
    val expectedSizeBytes: Long?,
    val sha256: String,
    val ddOutput: String,
)

/** Resultado do magiskboot unpack. */
data class UnpackResult(
    val exitCode: Int,
    val output: String,
    val headerInfo: String?,
    val hasRamdisk: Boolean,
    val hasKernel: Boolean,
)

/** Presença de arquivos no ramdisk relevantes para o patch. */
data class RamdiskInfo(
    val hasInit: Boolean,
    val hasInitSystem: Boolean,
    val cpioTestOk: Boolean,
    val listing: String,
)

/** Resultado da troca /init -> /init.system + instalação do jbm_init. */
data class InstallResult(
    val renamedInit: Boolean,
    val addedJbmInit: Boolean,
    val verified: Boolean,
    val detail: String,
)

/** Imagem final reconstruída e verificada. */
data class RepackResult(
    val exitCode: Int,
    val output: String,
    val outputFile: File,
    val outputSizeBytes: Long,
    val originalSizeBytes: Long,
    val outputSha256: String,
    val ramdiskMatchesStaged: Boolean,
    val initMatchesJbm: Boolean,
    val sizeUnchanged: Boolean,
    val verified: Boolean,
)

/** Relatório completo de uma sessão de patch. */
data class PatchReport(
    val startedAtMs: Long,
    val finishedAtMs: Long,
    val target: PatchTarget,
    val image: ExtractedImage?,
    val unpack: UnpackResult?,
    val ramdisk: RamdiskInfo?,
    val install: InstallResult?,
    val repack: RepackResult?,
    val warnings: List<String>,
    val failedStep: String?,
    val failureReason: String?,
) {
    val success: Boolean get() = failedStep == null && repack?.verified == true
}
