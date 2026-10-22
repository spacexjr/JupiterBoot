package com.spacexjr.jupiter.core.config

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ThemeIniTest {

    @Test
    fun parsesThemeFields() {
        val t = ThemeIni.parse(
            "aurora",
            "button_style = 3\ncolor_accent = #112233\nbutton_radius = 18\n" +
                "text_align = 1\nshow_icons = 0\nicon_scale = 130\n",
        )
        assertEquals("aurora", t.name)
        assertEquals(3, t.buttonStyle)
        assertEquals(0xFF112233.toInt(), t.colorAccent)
        assertEquals(18, t.buttonRadius)
        assertEquals(1, t.textAlign)
        assertFalse(t.showIcons)
        assertEquals(130, t.iconScale)
    }

    @Test
    fun resolvesExtendsChain() {
        val base = ThemeIni.parse("base", "color_accent = #101010\nbutton_radius = 4\n")
        val mid = ThemeIni.parse("mid", "extends = base\nbutton_radius = 8\n")
        val top = ThemeIni.parse("top", "extends = mid\ncolor_accent = #202020\n")
        val resolved = ThemeIni.resolve("top", mapOf("base" to base, "mid" to mid, "top" to top))
        assertEquals(8, resolved.buttonRadius)
        assertEquals(0xFF202020.toInt(), resolved.colorAccent)
    }

    @Test
    fun unknownThemeFallsBackToDefaults() {
        assertEquals(ThemeIni.defaults().copy(name = "missing"), ThemeIni.resolve("missing", emptyMap()))
    }

    @Test
    fun changedDefaultValueStaysExplicit() {
        val before = ThemeIni.defaults()
        val after = ThemeIni.markChanged(before, before.copy(buttonRadius = 7))
        assertTrue("button_radius" in after.explicitKeys)
        assertTrue(ThemeIni.serialize(after).contains("button_radius = 7"))
    }

    @Test
    fun serializeKeepsInheritanceMinimal() {
        val theme = ThemeIni.parse("child", "extends = base\nbutton_radius = 8\n")
        val out = ThemeIni.serialize(theme)
        assertTrue(out.contains("extends = base"))
        assertTrue(out.contains("button_radius = 8"))
        assertFalse(out.contains("color_bg ="))
        assertEquals(8, ThemeIni.resolve("child", mapOf("child" to ThemeIni.parse("child", out)))
            .buttonRadius)
    }

    @Test
    fun serializeRoundTrips() {
        val theme = ThemeIni.defaults().copy(name = "x", buttonStyle = 2, colorAccent = 0xFF010203.toInt())
        val out = ThemeIni.serialize(theme)
        val reparsed = ThemeIni.parse("x", out)
        assertEquals(theme.buttonStyle, reparsed.buttonStyle)
        assertEquals(theme.colorAccent, reparsed.colorAccent)
        assertEquals(theme.buttonAlpha, reparsed.buttonAlpha)
    }
}

class EffectiveMenuStyleTest {

    @Test
    fun themeValuesApplyWhenConfigHasNoMask() {
        val cfg = ConfigIniParser.parse("theme = x\n")
        val theme = ThemeIni.defaults().copy(buttonStyle = 3, colorAccent = 0xFF00FF00.toInt())
        val eff = EffectiveMenuStyle.resolve(cfg, theme)
        assertEquals(3, eff.buttonStyle)
        assertEquals(0xFF00FF00.toInt(), eff.colorAccent)
    }

    @Test
    fun onlyPresentConfigKeysOverrideTheme() {
        val cfg = ConfigIniParser.parse("button_style = 2\nlayout_margin = 12\n")
        val theme = ThemeIni.defaults().copy(buttonStyle = 3, layoutMargin = 4, colorAccent = 0xFF00FF00.toInt())
        val eff = EffectiveMenuStyle.resolve(cfg, theme)
        assertEquals(2, eff.buttonStyle)
        assertEquals(12, eff.layoutMargin)
        assertEquals(0xFF00FF00.toInt(), eff.colorAccent)
    }

    @Test
    fun effectiveValuesAreClamped() {
        val cfg = ConfigIniParser.parse("layout_margin = 90\nicon_scale = 10\nbutton_alpha = 999\n")
        val eff = EffectiveMenuStyle.resolve(cfg, ThemeIni.defaults())
        assertEquals(35, eff.layoutMargin)
        assertEquals(50, eff.iconScale)
        assertEquals(255, eff.buttonAlpha)
    }

    @Test
    fun entryOverridesAndInheritsPerField() {
        val menu = EffectiveMenuStyle.resolve(ConfigIni.defaults(), ThemeIni.defaults())
        val entry = JbmEntry("x", "X", EntryType.SCRIPT, buttonStyle = 3, buttonWidth = 600,
            buttonRadius = 24, showIcon = 0, colorTextSel = 0xFF00FF00.toInt())
        val eff = EffectiveButtonStyle.resolve(entry, menu)
        assertEquals(3, eff.style)
        assertEquals(600, eff.width)
        assertEquals(24, eff.radius)
        assertFalse(eff.showIcon)
        assertEquals(0xFF00FF00.toInt(), eff.colorTextSel)
        assertEquals(menu.colorText, eff.colorText)
        assertEquals(menu.buttonHeight, eff.height)
    }

    @Test
    fun geometryStacksRowsUsingEffectiveHeights() {
        val menu = EffectiveMenuStyle.resolve(ConfigIni.defaults(), ThemeIni.defaults())
        val rows = MenuGeometry.rows(
            720, 1600, menu,
            listOf(
                JbmEntry("a", "A", EntryType.SYSTEM, buttonHeight = 100),
                JbmEntry("b", "B", EntryType.SYSTEM, buttonHeight = 150),
                JbmEntry("c", "C", EntryType.SYSTEM),
            ),
        )
        assertEquals(3, rows.size)
        assertEquals(100, rows[0].height)
        assertEquals(rows[0].y + rows[0].height + menu.buttonSpacing, rows[1].y)
        assertTrue(rows[2].y > rows[1].y)
    }
}
