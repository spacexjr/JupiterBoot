package com.spacexjr.jupiter.core.patch

import com.spacexjr.jupiter.core.log.OperationLog
import com.spacexjr.jupiter.core.shell.CommandRunner
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

/** Fake executor que responde a comandos por um mapa argv->resultado. */
private class FakeExecutors(
    private val rules: List<Rule>,
) : PatchExecutors {

    data class Rule(
        val match: (List<String>) -> Boolean,
        val result: CommandRunner.Result,
    )

    private fun respond(argv: List<String>): CommandRunner.Result {
        val rule = rules.firstOrNull { it.match(argv) }
        return rule?.result ?: ok("")
    }

    override suspend fun direct(argv: List<String>, workdir: File?, timeoutMs: Long) = respond(argv)
    override suspend fun root(argv: List<String>, workdir: File?, timeoutMs: Long) = respond(argv)

    companion object {
        fun ok(stdout: String = ""): CommandRunner.Result =
            CommandRunner.Result(0, stdout, "", timedOut = false, durationMs = 1)

        fun fail(stderr: String = "erro", code: Int = 1): CommandRunner.Result =
            CommandRunner.Result(code, "", stderr, timedOut = false, durationMs = 1)
    }
}

private fun log() = OperationLog(File.createTempFile("log", ".txt").apply { deleteOnExit() })

private fun engine(executors: PatchExecutors) = PatchEngine(
    executors = executors,
    log = log(),
    magiskboot = { File("/nonexistent/magiskboot") },
    jbmInit = { File("/nonexistent/jbm_init") },
)

private fun getpropRules(vararg props: Pair<String, String>) = props.map { (k, v) ->
    FakeExecutors.Rule({ it.firstOrNull() == "getprop" && it.getOrNull(1) == k }, FakeExecutors.ok(v))
}

class PatchEngineScanTest {

    @Test
    fun onlyInitBootGivesHighConfidenceInitBootTarget() = runTest {
        val executors = FakeExecutors(
            listOf(FakeExecutors.Rule(
                { it.firstOrNull() == "ls" && it.getOrNull(2) == "/dev/block/by-name" },
                FakeExecutors.ok("init_boot_a\n"),
            )) + getpropRules(
                "ro.boot.slot_suffix" to "_a",
                "ro.product.device" to "device",
                "ro.product.model" to "Model",
                "ro.board.platform" to "platform",
            ),
        )
        val scan = engine(executors).scanPartitions()
        assertEquals(TargetKind.INIT_BOOT, scan.chosen?.kind)
        assertEquals(Confidence.HIGH, scan.chosen?.confidence)
    }

    @Test
    fun bothBootAndInitBootPrefersInitBootWithMediumConfidence() = runTest {
        val executors = FakeExecutors(
            listOf(FakeExecutors.Rule(
                { it.firstOrNull() == "ls" && it.getOrNull(2) == "/dev/block/by-name" },
                FakeExecutors.ok("boot\ninit_boot\n"),
            )) + getpropRules("ro.product.device" to "device"),
        )
        val scan = engine(executors).scanPartitions()
        assertEquals(TargetKind.INIT_BOOT, scan.chosen?.kind)
        assertEquals(Confidence.MEDIUM, scan.chosen?.confidence)
        assertEquals("init_boot", scan.chosen?.partition?.name)
    }

    @Test
    fun onlyBootGivesMediumConfidenceBootTarget() = runTest {
        val executors = FakeExecutors(
            listOf(FakeExecutors.Rule(
                { it.firstOrNull() == "ls" && it.getOrNull(2) == "/dev/block/by-name" },
                FakeExecutors.ok("boot\n"),
            )) + getpropRules("ro.product.device" to "device"),
        )
        val scan = engine(executors).scanPartitions()
        assertEquals(TargetKind.BOOT, scan.chosen?.kind)
        assertEquals(Confidence.MEDIUM, scan.chosen?.confidence)
    }

    @Test
    fun onlyVendorBootYieldsNoTarget() = runTest {
        val executors = FakeExecutors(
            listOf(FakeExecutors.Rule(
                { it.firstOrNull() == "ls" && it.getOrNull(2) == "/dev/block/by-name" },
                FakeExecutors.ok("vendor_boot\n"),
            )) + getpropRules("ro.product.device" to "device"),
        )
        val scan = engine(executors).scanPartitions()
        assertNull(scan.chosen)
        assertTrue(scan.notes.any { it.contains("não suportada") })
    }

    @Test
    fun noPartitionsYieldsNoTarget() = runTest {
        val executors = FakeExecutors(
            listOf(FakeExecutors.Rule(
                { it.firstOrNull() == "ls" },
                FakeExecutors.fail("not found"),
            )),
        )
        val scan = engine(executors).scanPartitions()
        assertNull(scan.chosen)
        assertTrue(scan.partitions.isEmpty())
    }

    @Test
    fun slotSuffixSelectsMatchingPartition() = runTest {
        val executors = FakeExecutors(
            listOf(FakeExecutors.Rule(
                { it.firstOrNull() == "ls" && it.getOrNull(2) == "/dev/block/by-name" },
                FakeExecutors.ok("init_boot_a\ninit_boot_b\n"),
            )) + getpropRules(
                "ro.boot.slot_suffix" to "_b",
                "ro.product.device" to "device",
            ),
        )
        val scan = engine(executors).scanPartitions()
        assertEquals("init_boot_b", scan.chosen?.partition?.name)
        assertEquals("_b", scan.slotSuffix)
    }
}
