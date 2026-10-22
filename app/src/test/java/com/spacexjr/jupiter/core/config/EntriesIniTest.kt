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
        assertEquals("Start Android", entries[0].description)
        assertEquals("system", entries[0].icon)
    }

    @Test
    fun parsesPerButtonAppearance() {
        val doc = EntriesIniParser.parse(
            "[x]\nname = X\ntype = script\ndescription = Runs\nicon = script\n" +
                "button_style = 3\nbutton_width = 640\nbutton_height = 120\n" +
                "button_radius = 22\nbutton_border_width = 2\ntext_align = 2\n" +
                "icon_scale = 130\nshow_icon = 0\ncolor_bg = #101010\n" +
                "color_border = #202020\ncolor_text = #303030\ncolor_text_sel = #404040\n",
        )
        val x = EntriesIniParser.toModel(doc).first()
        assertEquals("Runs", x.description)
        assertEquals("script", x.icon)
        assertEquals(3, x.buttonStyle)
        assertEquals(640, x.buttonWidth)
        assertEquals(120, x.buttonHeight)
        assertEquals(22, x.buttonRadius)
        assertEquals(2, x.buttonBorderWidth)
        assertEquals(2, x.textAlign)
        assertEquals(130, x.iconScale)
        assertEquals(0, x.showIcon)
        assertEquals(0xFF101010.toInt(), x.colorBg)
        assertEquals(0xFF404040.toInt(), x.colorTextSel)
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
    fun perButtonAppearanceRoundTrips() {
        val e = JbmEntry("x", "X", EntryType.SCRIPT, description = "D", icon = "script",
            buttonStyle = 3, buttonWidth = 600, buttonRadius = 20, textAlign = 1,
            showIcon = 0, colorTextSel = 0xFF00FF00.toInt())
        val out = EntriesIniSerializer.write(listOf(e), headerLines = emptyList())
        assertTrue(out.contains("description = D"))
        assertTrue(out.contains("button_style = 3"))
        assertTrue(out.contains("show_icon = 0"))
        val x = EntriesIniParser.toModel(EntriesIniParser.parse(out)).first()
        assertEquals(3, x.buttonStyle)
        assertEquals(600, x.buttonWidth)
        assertEquals(20, x.buttonRadius)
        assertEquals(1, x.textAlign)
        assertEquals(0, x.showIcon)
        assertEquals(0xFF00FF00.toInt(), x.colorTextSel)
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
    fun invalidPerButtonValuesAreFlagged() {
        val entries = listOf(
            JbmEntry("x", "X", EntryType.SYSTEM, buttonStyle = 9, textAlign = 5,
                buttonWidth = 10, buttonRadius = -5),
        )
        val issues = EntriesValidator.validate(entries)
        assertTrue(issues.any { it.message.contains("button_style") })
        assertTrue(issues.any { it.message.contains("text_align") })
        assertTrue(issues.any { it.message.contains("button_width") })
        assertTrue(issues.any { it.message.contains("button_radius") })
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
