package com.spacexjr.jupiter.core.config

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class EntriesIniParserTest {

    private val sample = """
        # JupiterBoot entries
        [system]
        name = System
        type = system
        enabled = 1
        order = 0
        default = 1
        custom_key = keep_me

        [recovery]
        name = Recovery
        type = recovery
        path = /sbin/recovery
        args = --wipe
        order = 1
    """.trimIndent()

    @Test
    fun parsesSectionsInOrder() {
        val doc = EntriesIniParser.parse(sample)
        assertEquals(2, doc.entries.size)
        assertEquals("system", doc.entries[0].id)
        assertEquals("recovery", doc.entries[1].id)
    }

    @Test
    fun preservesHeaderLines() {
        val doc = EntriesIniParser.parse(sample)
        assertTrue(doc.headerLines.any { it.contains("JupiterBoot entries") })
    }

    @Test
    fun toModelMapsKnownFields() {
        val entries = EntriesIniParser.toModel(EntriesIniParser.parse(sample))
        val recovery = entries.first { it.id == "recovery" }
        assertEquals("Recovery", recovery.name)
        assertEquals(EntryType.RECOVERY, recovery.type)
        assertEquals("/sbin/recovery", recovery.path)
        assertEquals("--wipe", recovery.args)
        assertEquals(1, recovery.order)
        val system = entries.first { it.id == "system" }
        assertTrue(system.isDefault)
    }

    @Test
    fun unknownKeysArePreservedSeparately() {
        val doc = EntriesIniParser.parse(sample)
        val unknown = EntriesIniParser.unknownFieldsFor(doc, "system")
        assertTrue(unknown.any { it.first == "custom_key" && it.second == "keep_me" })
    }

    @Test
    fun builtinEntriesMatchFirmwareDefaults() {
        val entries = EntriesIniParser.builtinEntries()
        assertEquals(6, entries.size)
        assertEquals(EntryType.SYSTEM, entries[0].type)
        assertTrue(entries[0].isDefault)
        assertEquals(EntryType.POWEROFF, entries[5].type)
    }
}

class EntriesIniSerializerTest {

    @Test
    fun serializesKnownFieldsAndPreservesUnknown() {
        val entries = listOf(
            JbmEntry("system", "System", EntryType.SYSTEM, order = 0, isDefault = true),
        )
        val out = EntriesIniSerializer.write(
            entries,
            unknownFields = mapOf("system" to listOf("vendor_flag" to "42")),
            headerLines = listOf("# header"),
        )
        assertTrue(out.contains("# header"))
        assertTrue(out.contains("[system]"))
        assertTrue(out.contains("name = System"))
        assertTrue(out.contains("vendor_flag = 42"))
    }

    @Test
    fun ordersEntriesByOrderField() {
        val entries = listOf(
            JbmEntry("b", "B", EntryType.SYSTEM, order = 5),
            JbmEntry("a", "A", EntryType.SYSTEM, order = 1),
        )
        val out = EntriesIniSerializer.write(entries, headerLines = emptyList())
        assertTrue(out.indexOf("[a]") < out.indexOf("[b]"))
    }

    @Test
    fun roundTripPreservesEntryData() {
        val entries = EntriesIniParser.toModel(EntriesIniParser.parse(
            "[x]\nname = X\ntype = script\npath = /data/x.sh\nargs = -v\norder = 2\n",
        ))
        val out = EntriesIniSerializer.write(entries, headerLines = emptyList())
        val reparsed = EntriesIniParser.toModel(EntriesIniParser.parse(out))
        val x = reparsed.first { it.id == "x" }
        assertEquals("X", x.name)
        assertEquals(EntryType.SCRIPT, x.type)
        assertEquals("/data/x.sh", x.path)
        assertEquals("-v", x.args)
    }
}

class EntriesValidatorTest {

    @Test
    fun builtinEntriesAreValid() {
        assertTrue(EntriesValidator.validate(EntriesIniParser.builtinEntries()).isEmpty())
    }

    @Test
    fun scriptWithoutPathIsFlagged() {
        val entries = listOf(JbmEntry("s", "Script", EntryType.SCRIPT, path = ""))
        assertTrue(EntriesValidator.validate(entries).any { it.entryId == "s" })
    }

    @Test
    fun relativePathIsFlagged() {
        val entries = listOf(JbmEntry("s", "Script", EntryType.SCRIPT, path = "bin/x.sh"))
        assertTrue(EntriesValidator.validate(entries).any { it.message.contains("absoluto") })
    }

    @Test
    fun duplicateIdsAreFlagged() {
        val entries = listOf(
            JbmEntry("dup", "A", EntryType.SYSTEM),
            JbmEntry("dup", "B", EntryType.SYSTEM),
        )
        assertTrue(EntriesValidator.validate(entries).any { it.message.contains("duplicado") })
    }

    @Test
    fun invalidIdIsFlagged() {
        val entries = listOf(JbmEntry("bad id", "A", EntryType.SYSTEM))
        assertTrue(EntriesValidator.validate(entries).any { it.message.contains("id invalido") })
    }

    @Test
    fun multipleDefaultsIsNonFatal() {
        val entries = listOf(
            JbmEntry("a", "A", EntryType.SYSTEM, isDefault = true),
            JbmEntry("b", "B", EntryType.SYSTEM, isDefault = true),
        )
        val issue = EntriesValidator.validate(entries).first { it.message.contains("default") }
        assertFalse(issue.fatal)
    }
}
