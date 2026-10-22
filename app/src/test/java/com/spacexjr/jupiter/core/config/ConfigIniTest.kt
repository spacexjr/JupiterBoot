package com.spacexjr.jupiter.core.config

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class JbmValuesTest {

    @Test
    fun colorWithHashAndSixDigitsGetsOpaqueAlpha() {
        assertEquals(0xFFFF0000.toInt(), JbmValues.parseColor("#ff0000"))
    }

    @Test
    fun colorWith0xPrefix() {
        assertEquals(0xFF00FF00.toInt(), JbmValues.parseColor("0x00ff00"))
    }

    @Test
    fun colorWithEightDigitsKeepsAlpha() {
        assertEquals(0x80112233.toInt(), JbmValues.parseColor("#80112233"))
    }

    @Test
    fun invalidColorReturnsNull() {
        assertNull(JbmValues.parseColor("not-a-color"))
        assertNull(JbmValues.parseColor(""))
        assertNull(JbmValues.parseColor(null))
    }

    @Test
    fun boolAcceptsLeadingTrueLikeChars() {
        assertTrue(JbmValues.parseBool("1"))
        assertTrue(JbmValues.parseBool("true"))
        assertTrue(JbmValues.parseBool("Yes"))
        assertTrue(JbmValues.parseBool("on"))
        assertFalse(JbmValues.parseBool("0"))
        assertFalse(JbmValues.parseBool("no"))
        assertFalse(JbmValues.parseBool(""))
    }

    @Test
    fun intParsesSignAndStopsAtJunk() {
        assertEquals(42, JbmValues.parseInt("42", -1))
        assertEquals(-7, JbmValues.parseInt("-7", 0))
        assertEquals(12, JbmValues.parseInt("12abc", 0))
        assertEquals(99, JbmValues.parseInt("", 99))
        assertEquals(99, JbmValues.parseInt("abc", 99))
    }
}

class ConfigIniParserTest {

    @Test
    fun parsesKnownKeysAndSkipsCommentsAndSections() {
        val text = """
            # a comment
            ; another
            [section]
            theme = dark
            color_bg = #010203
            button_width = 500
            show_countdown = 0
            orientation = auto
        """.trimIndent()
        val c = ConfigIniParser.parse(text)
        assertEquals("dark", c.theme)
        assertEquals(0xFF010203.toInt(), c.colorBg)
        assertEquals(500, c.buttonWidth)
        assertFalse(c.showCountdown)
        assertEquals(-1, c.orientation)
    }

    @Test
    fun missingKeysKeepDefaultsAndDoNotSetMask() {
        val c = ConfigIniParser.parse("theme = x\n")
        assertEquals(0, c.mask)
    }

    @Test
    fun presentKeysSetMaskBits() {
        val c = ConfigIniParser.parse("color_bg = #000000\nbutton_height = 80\n")
        assertEquals(ConfigIni.K_COLOR_BG or ConfigIni.K_BTN_H, c.mask)
    }

    @Test
    fun orientationNumericOneMeansNinety() {
        assertEquals(90, ConfigIniParser.parse("orientation = 1\n").orientation)
        assertEquals(180, ConfigIniParser.parse("orientation = 2\n").orientation)
        assertEquals(0, ConfigIniParser.parse("orientation = 0\n").orientation)
    }

    @Test
    fun emptyTextYieldsDefaults() {
        assertEquals(ConfigIni.defaults().copy(), ConfigIniParser.parse(""))
    }

    @Test
    fun parsesAppearanceKeysAndMasks() {
        val c = ConfigIniParser.parse(
            "config_version = 2\ntitle = Nebula\nsubtitle = OS\n" +
                "show_title = 0\nshow_description = 0\nshow_icons = 1\n" +
                "icon_scale = 140\ntext_align = 1\nlayout_margin = 11\n" +
                "button_alpha = 200\nbutton_border_width = 3\nbutton_style = 2\n" +
                "color_selected_bg = #112233\ncolor_pressed_bg = #445566\n" +
                "color_disabled_text = #778899\n",
        )
        assertEquals(2, c.configVersion)
        assertEquals("Nebula", c.title)
        assertEquals("OS", c.subtitle)
        assertFalse(c.showTitle)
        assertFalse(c.showDescription)
        assertEquals(140, c.iconScale)
        assertEquals(1, c.textAlign)
        assertEquals(11, c.layoutMargin)
        assertEquals(200, c.buttonAlpha)
        assertEquals(3, c.buttonBorderWidth)
        assertEquals(2, c.buttonStyle)
        assertEquals(0xFF112233.toInt(), c.colorSelectedBg)
        assertEquals(0xFF445566.toInt(), c.colorPressedBg)
        assertEquals(0xFF778899.toInt(), c.colorDisabledText)
        val expected = ConfigIni.K_SHOW_TITLE or ConfigIni.K_SHOW_DESC or
            ConfigIni.K_SHOW_ICONS or ConfigIni.K_ICON_SCALE or ConfigIni.K_TEXT_ALIGN or
            ConfigIni.K_LAYOUT_MARGIN or ConfigIni.K_BTN_ALPHA or ConfigIni.K_BTN_BORDER or
            ConfigIni.K_BTN_STYLE or ConfigIni.K_COLOR_SEL_BG or ConfigIni.K_COLOR_PRESSED or
            ConfigIni.K_COLOR_DISABLED
        assertEquals(expected, c.mask)
    }
}

class ConfigIniSerializerTest {

    @Test
    fun updatesExistingKeyInPlace() {
        val original = "# header\nbutton_width = 100\n"
        val cfg = ConfigIniParser.parse(original).copy(buttonWidth = 250)
        val out = ConfigIniSerializer.write(cfg, original)
        assertTrue(out.contains("button_width = 250"))
        assertTrue(out.contains("# header"))
        assertFalse(out.contains("button_width = 100"))
    }

    @Test
    fun appendsMissingKeys() {
        val original = "theme = default\n"
        val cfg = ConfigIniParser.parse(original).copy(buttonWidth = 333)
        val out = ConfigIniSerializer.write(cfg, original)
        assertTrue(out.contains("button_width = 333"))
        assertTrue(out.contains("theme = default"))
    }

    @Test
    fun missingAppearanceKeysDoNotOverrideTheme() {
        val original = "theme = jupiter\n"
        val out = ConfigIniSerializer.write(ConfigIniParser.parse(original), original)
        assertFalse(out.contains("button_style ="))
        assertFalse(out.contains("color_bg ="))
        assertTrue(out.contains("config_version ="))
    }

    @Test
    fun touchedAppearanceKeyIsAppended() {
        val original = "theme = jupiter\n"
        val cfg = ConfigIniParser.parse(original)
            .copy(buttonStyle = 2, mask = ConfigIni.K_BTN_STYLE)
        val out = ConfigIniSerializer.write(cfg, original)
        assertTrue(out.contains("button_style = 2"))
    }

    @Test
    fun renderFullWhenNoOriginal() {
        val out = ConfigIniSerializer.write(ConfigIni.defaults(), null)
        assertTrue(out.contains("theme = default"))
        assertTrue(out.contains("menu_timeout_sec = 15"))
    }

    @Test
    fun roundTripPreservesParsedValues() {
        val original = "color_bg = #010203\nbutton_width = 512\norientation = 90\n"
        val cfg = ConfigIniParser.parse(original)
        val out = ConfigIniSerializer.write(cfg, original)
        val reparsed = ConfigIniParser.parse(out)
        assertEquals(cfg.colorBg, reparsed.colorBg)
        assertEquals(cfg.buttonWidth, reparsed.buttonWidth)
        assertEquals(cfg.orientation, reparsed.orientation)
    }
}

class ConfigIniValidatorTest {

    @Test
    fun defaultsAreValid() {
        assertTrue(ConfigIniValidator.validate(ConfigIni.defaults()).isEmpty())
    }

    @Test
    fun negativeButtonWidthIsFlagged() {
        assertTrue(ConfigIniValidator.validate(ConfigIni.defaults().copy(buttonWidth = -1)).isNotEmpty())
    }

    @Test
    fun appearanceOutOfRangeIsFlagged() {
        val bad = ConfigIni.defaults().copy(
            buttonAlpha = 999,
            layoutMargin = 90,
            textAlign = 7,
            iconScale = 10,
            configVersion = 3,
        )
        val issues = ConfigIniValidator.validate(bad)
        assertTrue(issues.any { it.key == "button_alpha" })
        assertTrue(issues.any { it.key == "layout_margin" })
        assertTrue(issues.any { it.key == "text_align" })
        assertTrue(issues.any { it.key == "icon_scale" })
        assertTrue(issues.any { it.key == "config_version" })
    }
}
