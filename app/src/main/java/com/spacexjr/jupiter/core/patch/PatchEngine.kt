package com.spacexjr.jupiter.core.patch

import com.spacexjr.jupiter.core.log.LogLevel
import com.spacexjr.jupiter.core.log.LogSource
import com.spacexjr.jupiter.core.log.OperationLog
import com.spacexjr.jupiter.core.shell.CommandRunner
import com.spacexjr.jupiter.core.shell.Sh
import com.spacexjr.jupiter.core.util.Hashes
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File

/** Executors usados pelo motor: direto (app) e via root. */
interface PatchExecutors {
    suspend fun direct(argv: List<String>, workdir: File?, timeoutMs: Long): CommandRunner.Result
    suspend fun root(argv: List<String>, timeoutMs: Long): CommandRunner.Result
}

/**
 * Motor de patch do boot image.
 *
 * Fluxo (cada etapa pode ser chamada isoladamente pela UI, com confirmação
 * do usuário entre etapas):
 * 1. scanPartitions — identifica boot/init_boot, slot e decide o alvo;
 * 2. extractImage — dd da partição para o armazenamento privado, com
 *    verificação de tamanho e SHA-256;
 * 3. unpack — magiskboot unpack -h;
 * 4. inspectRamdisk — presença de /init e /init.system;
 * 5. installJupiter — mv init init.system + add 0750 init jbm_init;
 * 6. repack + verify — reconstrói e revalida a imagem.
 *
 * O motor NUNCA grava partições físicas. A imagem final permanece no
 * armazenamento privado até exportação explícita do usuário.
 */
class PatchEngine(
    private val executors: PatchExecutors,
    private val log: OperationLog,
    private val magiskboot: () -> File,
    private val jbmInit: () -> File,
) {

    /** 1. Identificação de partições e escolha do alvo. */
    suspend fun scanPartitions(): ScanResult {
        val notes = mutableListOf<String>()

        val byNameDirs = listOf(
            "/dev/block/by-name",
            "/dev/block/bootdevice/by-name",
            "/dev/block/mapper",
        )
        val found = linkedMapOf<String, PartitionInfo>()
        for (dir in byNameDirs) {
            val ls = executors.root(listOf("ls", "-1", dir), timeoutMs = 10_000)
            if (!ls.isSuccess) continue
            notes += "lista de partições em $dir: ok"
            for (name in ls.stdout.lines().map { it.trim() }.filter { it.isNotBlank() }) {
                val base = name.removeSuffix("_a").removeSuffix("_b")
                if (base in INTERESTING || name in INTERESTING) {
                    found.putIfAbsent(name, PartitionInfo(name, "$dir/$name"))
                }
            }
        }

        val slotA = executors.root(listOf("getprop", "ro.boot.slot_suffix"), 8_000).stdout.trim()
        val slotB = executors.root(listOf("getprop", "ro.boot.slot"), 8_000).stdout.trim()
        val slot = slotA.ifEmpty { slotB }
        val device = executors.root(listOf("getprop", "ro.product.device"), 8_000).stdout.trim()
        val model = executors.root(listOf("getprop", "ro.product.model"), 8_000).stdout.trim()
        val platform = executors.root(listOf("getprop", "ro.board.platform"), 8_000).stdout.trim()

        val partitions = found.values.sortedBy { it.name }

        if (partitions.isEmpty()) {
            notes += "nenhuma partição boot/init_boot/vendor_boot reconhecida"
            log.append(LogLevel.ERROR, LogSource.PATCH, "scan: nenhuma partição de boot encontrada")
            return ScanResult(partitions, slot, device, model, platform, null, notes)
        }

        val chosen = chooseTarget(partitions, slot, notes)
        log.append(
            if (chosen != null) LogLevel.INFO else LogLevel.ERROR,
            LogSource.PATCH,
            chosen?.let { "alvo escolhido: ${it.partition.name} (${it.kind}) — ${it.rationale}" }
                ?: "scan: alvo não reconhecido — patch interrompido",
        )
        return ScanResult(partitions, slot, device, model, platform, chosen, notes)
    }

    /**
     * Política de escolha do alvo:
     * - só existe init_boot → alta confiança (GKI Android 13+);
     * - só existe boot → média confiança (ramdisk tradicional);
     * - ambos → init_boot com confirmação explícita do usuário;
     * - só vendor_boot → não suportado (o /init raramente vive lá);
     * - arquitetura não reconhecida → patch interrompido.
     */
    private fun chooseTarget(
        partitions: List<PartitionInfo>,
        slot: String,
        notes: MutableList<String>,
    ): PatchTarget? {
        fun pick(base: String): PartitionInfo? {
            if (slot.isNotEmpty()) {
                partitions.firstOrNull { it.name == base + slot }?.let { return it }
            }
            return partitions.firstOrNull { it.name == base }
        }

        val initBoot = pick("init_boot")
        val boot = pick("boot")
        val vendorBoot = pick("vendor_boot")

        vendorBoot?.let {
            notes += "vendor_boot presente (${it.name}); geralmente não contém /init em GKI"
        }

        return when {
            initBoot != null && boot != null -> {
                notes += "boot e init_boot presentes — priorizando init_boot (Android 13+ GKI " +
                    "mantém o first-stage init em init_boot); confirme no aparelho antes de prosseguir"
                PatchTarget(
                    partition = initBoot,
                    kind = TargetKind.INIT_BOOT,
                    rationale = "init_boot é o ponto de integração do /init em dispositivos GKI " +
                        "Android 13+; boot pode nem sequer ter ramdisk",
                    confidence = Confidence.MEDIUM,
                )
            }
            initBoot != null -> PatchTarget(
                partition = initBoot,
                kind = TargetKind.INIT_BOOT,
                rationale = "única partição com ramdisk de init detectada (init_boot)",
                confidence = Confidence.HIGH,
            )
            boot != null -> PatchTarget(
                partition = boot,
                kind = TargetKind.BOOT,
                rationale = "única partição de boot detectada (boot clássico com ramdisk)",
                confidence = Confidence.MEDIUM,
            )
            vendorBoot != null -> {
                notes += "apenas vendor_boot detectada — arquitetura não suportada para este patch"
                null
            }
            else -> {
                notes += "arquitetura de boot não reconhecida"
                null
            }
        }
    }

    /** 2. Extração com dd + verificação de tamanho e hash. */
    suspend fun extractImage(target: PatchTarget, workDir: File): ExtractedImage {
        val imageFile = File(workDir, "boot-stock.img")
        val part = Sh.validateSystemPath(target.partition.linkPath)

        executors.root(listOf("mkdir", "-p", workDir.absolutePath), 10_000)

        val dd = executors.root(
            listOf("dd", "if=$part", "of=${imageFile.absolutePath}", "bs=4096"),
            timeoutMs = DD_TIMEOUT_MS,
        )
        if (!dd.isSuccess) {
            throw PatchException("dd falhou: ${dd.stderr.ifBlank { dd.stdout }}")
        }
        executors.root(listOf("chmod", "0644", imageFile.absolutePath), 10_000)
        executors.root(listOf("sync"), 15_000)

        val size = withContext(Dispatchers.IO) { imageFile.length() }
        if (size <= 0L) {
            throw PatchException("extração produziu arquivo vazio")
        }

        val expected = executors.root(listOf("blockdev", "--getsize64", part), 10_000)
            .stdout.trim().toLongOrNull()

        if (expected != null && expected > 0 && size != expected) {
            throw PatchException("tamanho lido ($size) difere do tamanho da partição ($expected)")
        }

        val statOut = executors.root(listOf("stat", "-c", "%s", imageFile.absolutePath), 10_000)
        val statSize = statOut.stdout.trim().toLongOrNull()
        if (statSize != null && statSize != size) {
            throw PatchException("stat divergente: $statSize vs $size")
        }

        val sha = withContext(Dispatchers.IO) { Hashes.sha256Hex(imageFile) }
        log.append(
            LogLevel.INFO, LogSource.PATCH,
            "imagem extraída: ${imageFile.name} · $size bytes · sha256=${sha.take(16)}…",
        )
        return ExtractedImage(
            partition = target.partition,
            file = imageFile,
            sizeBytes = size,
            expectedSizeBytes = expected,
            sha256 = sha,
            ddOutput = dd.stdout.takeLast(2000),
        )
    }

    /** 3. Unpack com magiskboot. */
    suspend fun unpack(workDir: File, image: ExtractedImage): UnpackResult {
        val mb = magiskboot()
        val result = executors.direct(
            listOf(mb.absolutePath, "unpack", "-h", image.file.name),
            workdir = workDir,
            timeoutMs = STEP_TIMEOUT_MS,
        )
        when (result.exitCode) {
            0, 3 -> Unit // 3 = vendor_boot (aceitável apenas para inspeção)
            2 -> throw PatchException("imagem ChromeOS não suportada")
            else -> throw PatchException(
                "magiskboot unpack falhou (exit=${result.exitCode}): ${result.stderr.ifBlank { result.stdout }}",
            )
        }
        val ramdisk = File(workDir, "ramdisk.cpio")
        val kernel = File(workDir, "kernel")
        val header = File(workDir, "header")
        val headerInfo = if (header.isFile) {
            withContext(Dispatchers.IO) { header.readText() }.take(4000)
        } else null

        if (!ramdisk.isFile) {
            throw PatchException(
                "a imagem não tem ramdisk.cpio — o alvo provavelmente não é onde vive o /init " +
                    "(em GKI o init está em init_boot; tente outra partição)",
            )
        }
        log.append(LogLevel.INFO, LogSource.PATCH, "unpack ok · ramdisk=${ramdisk.length()} bytes")
        return UnpackResult(
            exitCode = result.exitCode,
            output = result.stdout.takeLast(4000),
            headerInfo = headerInfo,
            hasRamdisk = true,
            hasKernel = kernel.isFile,
        )
    }

    /** 4. Inspeciona o ramdisk sem modificá-lo. */
    suspend fun inspectRamdisk(workDir: File): RamdiskInfo {
        val mb = magiskboot()
        val hasInit = executors.direct(
            listOf(mb.absolutePath, "cpio", "ramdisk.cpio", "exists", "init"),
            workDir, STEP_TIMEOUT_MS,
        ).exitCode == 0
        val hasInitSystem = executors.direct(
            listOf(mb.absolutePath, "cpio", "ramdisk.cpio", "exists", "init.system"),
            workDir, STEP_TIMEOUT_MS,
        ).exitCode == 0
        val test = executors.direct(
            listOf(mb.absolutePath, "cpio", "ramdisk.cpio", "test"),
            workDir, STEP_TIMEOUT_MS,
        )
        val listing = executors.direct(
            listOf(mb.absolutePath, "cpio", "ramdisk.cpio", "ls"),
            workDir, STEP_TIMEOUT_MS,
        ).stdout.take(8000)

        if (!hasInit) {
            throw PatchException(
                "ramdisk não contém /init — nada a substituir; " +
                    (if (hasInitSystem) "já contém init.system (patch anterior?)"
                    else "arquitetura não compatível com o modelo do JupiterBoot"),
            )
        }
        log.append(
            LogLevel.INFO, LogSource.PATCH,
            "ramdisk: init=$hasInit init.system=$hasInitSystem test=${test.exitCode == 0}",
        )
        return RamdiskInfo(hasInit, hasInitSystem, test.exitCode == 0, listing)
    }

    /** 5. Integração: /init original vira /init.system; jbm_init vira /init. */
    suspend fun installJupiter(workDir: File): InstallResult {
        val mb = magiskboot()
        val jbm = jbmInit()
        if (!withContext(Dispatchers.IO) { jbm.isFile }) {
            throw PatchException("binário jbm_init não instalado no app")
        }

        var renamed = false
        val preInitSystem = executors.direct(
            listOf(mb.absolutePath, "cpio", "ramdisk.cpio", "exists", "init.system"),
            workDir, STEP_TIMEOUT_MS,
        ).exitCode == 0

        if (!preInitSystem) {
            val mv = executors.direct(
                listOf(mb.absolutePath, "cpio", "ramdisk.cpio", "mv", "init", "init.system"),
                workDir, STEP_TIMEOUT_MS,
            )
            if (mv.exitCode != 0) {
                throw PatchException("falha ao renomear init -> init.system: ${mv.stderr.ifBlank { mv.outputText() }}")
            }
            renamed = true
        }

        val add = executors.direct(
            listOf(mb.absolutePath, "cpio", "ramdisk.cpio", "add", "0750", "init", jbm.absolutePath),
            workDir, STEP_TIMEOUT_MS,
        )
        if (add.exitCode != 0) {
            throw PatchException("falha ao instalar jbm_init como /init: ${add.stderr.ifBlank { add.outputText() }}")
        }

        val okInit = executors.direct(
            listOf(mb.absolutePath, "cpio", "ramdisk.cpio", "exists", "init"),
            workDir, STEP_TIMEOUT_MS,
        ).exitCode == 0
        val okInitSystem = executors.direct(
            listOf(mb.absolutePath, "cpio", "ramdisk.cpio", "exists", "init.system"),
            workDir, STEP_TIMEOUT_MS,
        ).exitCode == 0
        val test = executors.direct(
            listOf(mb.absolutePath, "cpio", "ramdisk.cpio", "test"),
            workDir, STEP_TIMEOUT_MS,
        )
        val verified = okInit && okInitSystem && test.exitCode == 0
        if (!verified) {
            throw PatchException(
                "verificação pós-instalação falhou (init=$okInit init.system=$okInitSystem test=${test.exitCode})",
            )
        }
        log.append(
            LogLevel.INFO, LogSource.PATCH,
            "jbm_init instalado como /init; original preservado em /init.system " +
                "(o menu encaminha o controle via execve(\"/init.system\"))",
        )
        return InstallResult(
            renamedInit = renamed,
            addedJbmInit = true,
            verified = true,
            detail = if (renamed) "init -> init.system; jbm_init adicionado como init"
            else "init.system já existia; init substituído pelo jbm_init",
        )
    }

    /** 6. Repack e verificação completa da imagem gerada. */
    suspend fun repack(workDir: File, image: ExtractedImage): RepackResult {
        val mb = magiskboot()
        val out = File(workDir, "boot-jbm.img")

        val repack = executors.direct(
            listOf(mb.absolutePath, "repack", image.file.name, out.name),
            workDir, STEP_TIMEOUT_MS,
        )
        if (repack.exitCode != 0 || !withContext(Dispatchers.IO) { out.isFile }) {
            throw PatchException(
                "magiskboot repack falhou (exit=${repack.exitCode}): ${repack.stderr.ifBlank { repack.outputText() }}",
            )
        }

        // Verificação: reabre a saída em um diretório próprio.
        val verifyDir = File(workDir, "verify")
        withContext(Dispatchers.IO) { verifyDir.mkdirs() }
        val unpackOut = executors.direct(
            listOf(mb.absolutePath, "unpack", "-h", out.name),
            verifyDir, STEP_TIMEOUT_MS,
        )
        val stagedRamdisk = File(workDir, "ramdisk.cpio")
        val verifyRamdisk = File(verifyDir, "ramdisk.cpio")
        val ramdiskMatches = withContext(Dispatchers.IO) {
            verifyRamdisk.isFile && stagedRamdisk.isFile &&
                Hashes.sha256Hex(verifyRamdisk) == Hashes.sha256Hex(stagedRamdisk)
        }
        if (unpackOut.exitCode != 0) {
            throw PatchException("reabertura da imagem gerada falhou (exit=${unpackOut.exitCode})")
        }

        // Extrai o /init gerado e compara com o jbm_init instalado.
        withContext(Dispatchers.IO) { File(verifyDir, "init").delete() }
        executors.direct(
            listOf(mb.absolutePath, "cpio", "ramdisk.cpio", "extract", "init"),
            verifyDir, STEP_TIMEOUT_MS,
        ) // magiskboot pode retornar 1 mesmo em sucesso; o arquivo é a prova
        val extractedInit = File(verifyDir, "init")
        val initMatches = withContext(Dispatchers.IO) {
            extractedInit.isFile && Hashes.sha256Hex(extractedInit) == Hashes.sha256Hex(jbmInit())
        }

        val outSize = withContext(Dispatchers.IO) { out.length() }
        val sizeUnchanged = outSize == image.sizeBytes
        val outSha = withContext(Dispatchers.IO) { Hashes.sha256Hex(out) }

        val verified = ramdiskMatches && initMatches && sizeUnchanged
        if (!verified) {
            throw PatchException(
                "imagem gerada não passou na verificação " +
                    "(ramdisk=$ramdiskMatches init=$initMatches tamanho=$sizeUnchanged)",
            )
        }
        log.append(
            LogLevel.INFO, LogSource.PATCH,
            "repack verificado · ${outSize} bytes · sha256=${outSha.take(16)}…",
        )
        return RepackResult(
            exitCode = repack.exitCode,
            output = repack.stdout.takeLast(4000),
            outputFile = out,
            outputSizeBytes = outSize,
            originalSizeBytes = image.sizeBytes,
            outputSha256 = outSha,
            ramdiskMatchesStaged = ramdiskMatches,
            initMatchesJbm = initMatches,
            sizeUnchanged = sizeUnchanged,
            verified = true,
        )
    }

    private fun CommandRunner.Result.outputText(): String =
        stdout + stderr

    companion object {
        const val DD_TIMEOUT_MS = 10 * 60 * 1000L
        const val STEP_TIMEOUT_MS = 5 * 60 * 1000L
        private val INTERESTING = setOf(
            "boot", "init_boot", "vendor_boot", "recovery_boot", "ramdisk",
        )
    }
}

class PatchException(message: String) : Exception(message)
