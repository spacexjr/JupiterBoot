package com.spacexjr.jupiter.core.config

internal val THEME_KEYS: Set<String> = setOf(
    "extends", "button_style", "color_bg", "color_fg", "color_border",
    "color_highlight", "color_selected_bg", "color_pressed_bg",
    "color_disabled_text", "color_text", "color_text_sel", "color_accent",
    "button_width", "button_height", "button_spacing", "button_radius",
    "button_border_width", "button_alpha", "text_size", "text_align",
    "layout_margin", "show_title", "show_description", "show_icons", "icon_scale",
)

/**
 * Modelo de um arquivo themes/<nome>.ini.
 *
 * Os valores padrao espelham jbm_set_default_theme() em src/jbm_menu.c.
 * `extends` permite herdar outro tema (o firmware resolve ate dois niveis).
 */
data class ThemeIni(
    var name: String = "default",
    var extends: String = "",
    var buttonStyle: Int = 0,
    var colorBg: Int = 0xFF020202.toInt(),
    var colorFg: Int = 0xFF20242C.toInt(),
    var colorBorder: Int = 0xFFD88A4E.toInt(),
    var colorHighlight: Int = 0xFFB86837.toInt(),
    var colorSelectedBg: Int = 0xFFB86837.toInt(),
    var colorPressedBg: Int = 0xFFD88A4E.toInt(),
    var colorDisabledText: Int = 0xFF525862.toInt(),
    var colorText: Int = 0xFF858585.toInt(),
    var colorTextSel: Int = 0xFFFFFFFF.toInt(),
    var colorAccent: Int = 0xFFC87335.toInt(),
    var buttonWidth: Int = 430,
    var buttonHeight: Int = 94,
    var buttonSpacing: Int = 12,
    var buttonRadius: Int = 7,
    var buttonBorderWidth: Int = 1,
    var buttonAlpha: Int = 255,
    var textSize: Int = 19,
    var textAlign: Int = 0,
    var layoutMargin: Int = 7,
    var showTitle: Boolean = true,
    var showDescription: Boolean = false,
    var showIcons: Boolean = false,
    var iconScale: Int = 100,
    /** Chaves realmente presentes no arquivo (usado pelo `extends`). */
    val explicitKeys: Set<String> = THEME_KEYS,
) {
    companion object {
        fun defaults() = ThemeIni()

        fun markChanged(before: ThemeIni, after: ThemeIni): ThemeIni {
            if (before == after) return after
            val changed = buildSet {
                if (before.buttonStyle != after.buttonStyle) add("button_style")
                if (before.colorBg != after.colorBg) add("color_bg")
                if (before.colorFg != after.colorFg) add("color_fg")
                if (before.colorBorder != after.colorBorder) add("color_border")
                if (before.colorHighlight != after.colorHighlight) add("color_highlight")
                if (before.colorSelectedBg != after.colorSelectedBg) add("color_selected_bg")
                if (before.colorPressedBg != after.colorPressedBg) add("color_pressed_bg")
                if (before.colorDisabledText != after.colorDisabledText) add("color_disabled_text")
                if (before.colorText != after.colorText) add("color_text")
                if (before.colorTextSel != after.colorTextSel) add("color_text_sel")
                if (before.colorAccent != after.colorAccent) add("color_accent")
                if (before.buttonWidth != after.buttonWidth) add("button_width")
                if (before.buttonHeight != after.buttonHeight) add("button_height")
                if (before.buttonSpacing != after.buttonSpacing) add("button_spacing")
                if (before.buttonRadius != after.buttonRadius) add("button_radius")
                if (before.buttonBorderWidth != after.buttonBorderWidth) add("button_border_width")
                if (before.buttonAlpha != after.buttonAlpha) add("button_alpha")
                if (before.textSize != after.textSize) add("text_size")
                if (before.textAlign != after.textAlign) add("text_align")
                if (before.layoutMargin != after.layoutMargin) add("layout_margin")
                if (before.showTitle != after.showTitle) add("show_title")
                if (before.showDescription != after.showDescription) add("show_description")
                if (before.showIcons != after.showIcons) add("show_icons")
                if (before.iconScale != after.iconScale) add("icon_scale")
            }
            return after.copy(explicitKeys = after.explicitKeys + changed)
        }

        fun parse(name: String, text: String): ThemeIni {
            var theme = ThemeIni(name = name, explicitKeys = emptySet())
            val seen = mutableSetOf<String>()
            IniDocument.parse(text).topLevelKv().forEach { line ->
                val key = line.key
                val value = line.value
                if (key in THEME_KEYS) seen += key
                when (key) {
                    "extends" -> theme.extends = value
                    "button_style" -> theme.buttonStyle = JbmValues.parseInt(value, theme.buttonStyle)
                    "color_bg" -> JbmValues.parseColor(value)?.let { theme.colorBg = it }
                    "color_fg" -> JbmValues.parseColor(value)?.let { theme.colorFg = it }
                    "color_border" -> JbmValues.parseColor(value)?.let { theme.colorBorder = it }
                    "color_highlight" -> JbmValues.parseColor(value)?.let {
                        theme.colorHighlight = it
                        theme.colorSelectedBg = it
                    }
                    "color_selected_bg" -> JbmValues.parseColor(value)?.let { theme.colorSelectedBg = it }
                    "color_pressed_bg" -> JbmValues.parseColor(value)?.let { theme.colorPressedBg = it }
                    "color_disabled_text" -> JbmValues.parseColor(value)?.let { theme.colorDisabledText = it }
                    "color_text" -> JbmValues.parseColor(value)?.let { theme.colorText = it }
                    "color_text_sel" -> JbmValues.parseColor(value)?.let { theme.colorTextSel = it }
                    "color_accent" -> JbmValues.parseColor(value)?.let { theme.colorAccent = it }
                    "button_width" -> theme.buttonWidth = JbmValues.parseInt(value, theme.buttonWidth)
                    "button_height" -> theme.buttonHeight = JbmValues.parseInt(value, theme.buttonHeight)
                    "button_spacing" -> theme.buttonSpacing = JbmValues.parseInt(value, theme.buttonSpacing)
                    "button_radius" -> theme.buttonRadius = JbmValues.parseInt(value, theme.buttonRadius)
                    "button_border_width" ->
                        theme.buttonBorderWidth = JbmValues.parseInt(value, theme.buttonBorderWidth)
                    "button_alpha" -> theme.buttonAlpha = JbmValues.parseInt(value, theme.buttonAlpha)
                    "text_size" -> theme.textSize = JbmValues.parseInt(value, theme.textSize)
                    "text_align" -> theme.textAlign = JbmValues.parseInt(value, theme.textAlign)
                    "layout_margin" -> theme.layoutMargin = JbmValues.parseInt(value, theme.layoutMargin)
                    "show_title" -> theme.showTitle = JbmValues.parseBool(value)
                    "show_description" -> theme.showDescription = JbmValues.parseBool(value)
                    "show_icons" -> theme.showIcons = JbmValues.parseBool(value)
                    "icon_scale" -> theme.iconScale = JbmValues.parseInt(value, theme.iconScale)
                }
            }
            theme = theme.copy(explicitKeys = seen)
            return theme
        }

        /**
         * Resolve `extends` com a mesma profundidade do firmware (2 saltos).
         * Devolve uma copia ja mesclada, pronta para receber o config.ini.
         */
        fun resolve(name: String, themes: Map<String, ThemeIni>, depth: Int = 0): ThemeIni {
            val current = themes[name] ?: ThemeIni(name = name)
            val merged = if (depth < 2 && current.extends.isNotEmpty() &&
                current.extends != name && themes.containsKey(current.extends)
            ) {
                resolve(current.extends, themes, depth + 1)
            } else {
                defaults()
            }
            return applyExplicit(merged, current)
        }

        private fun applyExplicit(base: ThemeIni, override: ThemeIni): ThemeIni {
            val keys = override.explicitKeys
            var out = base.copy(name = override.name, extends = override.extends)
            fun has(k: String) = k in keys
            if (has("button_style")) out = out.copy(buttonStyle = override.buttonStyle)
            if (has("color_bg")) out = out.copy(colorBg = override.colorBg)
            if (has("color_fg")) out = out.copy(colorFg = override.colorFg)
            if (has("color_border")) out = out.copy(colorBorder = override.colorBorder)
            if (has("color_highlight")) out = out.copy(colorHighlight = override.colorHighlight)
            if (has("color_selected_bg")) out = out.copy(colorSelectedBg = override.colorSelectedBg)
            if (has("color_pressed_bg")) out = out.copy(colorPressedBg = override.colorPressedBg)
            if (has("color_disabled_text")) out = out.copy(colorDisabledText = override.colorDisabledText)
            if (has("color_text")) out = out.copy(colorText = override.colorText)
            if (has("color_text_sel")) out = out.copy(colorTextSel = override.colorTextSel)
            if (has("color_accent")) out = out.copy(colorAccent = override.colorAccent)
            if (has("button_width")) out = out.copy(buttonWidth = override.buttonWidth)
            if (has("button_height")) out = out.copy(buttonHeight = override.buttonHeight)
            if (has("button_spacing")) out = out.copy(buttonSpacing = override.buttonSpacing)
            if (has("button_radius")) out = out.copy(buttonRadius = override.buttonRadius)
            if (has("button_border_width")) out = out.copy(buttonBorderWidth = override.buttonBorderWidth)
            if (has("button_alpha")) out = out.copy(buttonAlpha = override.buttonAlpha)
            if (has("text_size")) out = out.copy(textSize = override.textSize)
            if (has("text_align")) out = out.copy(textAlign = override.textAlign)
            if (has("layout_margin")) out = out.copy(layoutMargin = override.layoutMargin)
            if (has("show_title")) out = out.copy(showTitle = override.showTitle)
            if (has("show_description")) out = out.copy(showDescription = override.showDescription)
            if (has("show_icons")) out = out.copy(showIcons = override.showIcons)
            if (has("icon_scale")) out = out.copy(iconScale = override.iconScale)
            return out
        }

        fun serialize(theme: ThemeIni): String = buildString {
            val d = defaults()
            fun key(name: String) = name in theme.explicitKeys
            if (theme.extends.isNotEmpty()) append("extends = ${theme.extends}\n")
            if (key("button_style") || theme.buttonStyle != d.buttonStyle)
                append("button_style = ${theme.buttonStyle}\n")
            if (key("color_bg") || theme.colorBg != d.colorBg)
                append("color_bg = ${JbmValues.formatColor(theme.colorBg)}\n")
            if (key("color_fg") || theme.colorFg != d.colorFg)
                append("color_fg = ${JbmValues.formatColor(theme.colorFg)}\n")
            if (key("color_border") || theme.colorBorder != d.colorBorder)
                append("color_border = ${JbmValues.formatColor(theme.colorBorder)}\n")
            if (key("color_highlight") || theme.colorHighlight != d.colorHighlight)
                append("color_highlight = ${JbmValues.formatColor(theme.colorHighlight)}\n")
            if (key("color_selected_bg") || theme.colorSelectedBg != d.colorSelectedBg)
                append("color_selected_bg = ${JbmValues.formatColor(theme.colorSelectedBg)}\n")
            if (key("color_pressed_bg") || theme.colorPressedBg != d.colorPressedBg)
                append("color_pressed_bg = ${JbmValues.formatColor(theme.colorPressedBg)}\n")
            if (key("color_disabled_text") || theme.colorDisabledText != d.colorDisabledText)
                append("color_disabled_text = ${JbmValues.formatColor(theme.colorDisabledText)}\n")
            if (key("color_text") || theme.colorText != d.colorText)
                append("color_text = ${JbmValues.formatColor(theme.colorText)}\n")
            if (key("color_text_sel") || theme.colorTextSel != d.colorTextSel)
                append("color_text_sel = ${JbmValues.formatColor(theme.colorTextSel)}\n")
            if (key("color_accent") || theme.colorAccent != d.colorAccent)
                append("color_accent = ${JbmValues.formatColor(theme.colorAccent)}\n")
            if (key("button_width") || theme.buttonWidth != d.buttonWidth)
                append("button_width = ${theme.buttonWidth}\n")
            if (key("button_height") || theme.buttonHeight != d.buttonHeight)
                append("button_height = ${theme.buttonHeight}\n")
            if (key("button_spacing") || theme.buttonSpacing != d.buttonSpacing)
                append("button_spacing = ${theme.buttonSpacing}\n")
            if (key("button_radius") || theme.buttonRadius != d.buttonRadius)
                append("button_radius = ${theme.buttonRadius}\n")
            if (key("button_border_width") || theme.buttonBorderWidth != d.buttonBorderWidth)
                append("button_border_width = ${theme.buttonBorderWidth}\n")
            if (key("button_alpha") || theme.buttonAlpha != d.buttonAlpha)
                append("button_alpha = ${theme.buttonAlpha}\n")
            if (key("text_size") || theme.textSize != d.textSize)
                append("text_size = ${theme.textSize}\n")
            if (key("text_align") || theme.textAlign != d.textAlign)
                append("text_align = ${theme.textAlign}\n")
            if (key("layout_margin") || theme.layoutMargin != d.layoutMargin)
                append("layout_margin = ${theme.layoutMargin}\n")
            if (key("show_title") || theme.showTitle != d.showTitle)
                append("show_title = ${if (theme.showTitle) 1 else 0}\n")
            if (key("show_description") || theme.showDescription != d.showDescription)
                append("show_description = ${if (theme.showDescription) 1 else 0}\n")
            if (key("show_icons") || theme.showIcons != d.showIcons)
                append("show_icons = ${if (theme.showIcons) 1 else 0}\n")
            if (key("icon_scale") || theme.iconScale != d.iconScale)
                append("icon_scale = ${theme.iconScale}\n")
        }
    }

    /** Copia os campos nao identidade do [other] sobre este tema. */
    fun merge(other: ThemeIni): ThemeIni = copy(
        name = other.name,
        extends = other.extends,
        buttonStyle = other.buttonStyle,
        colorBg = other.colorBg,
        colorFg = other.colorFg,
        colorBorder = other.colorBorder,
        colorHighlight = other.colorHighlight,
        colorSelectedBg = other.colorSelectedBg,
        colorPressedBg = other.colorPressedBg,
        colorDisabledText = other.colorDisabledText,
        colorText = other.colorText,
        colorTextSel = other.colorTextSel,
        colorAccent = other.colorAccent,
        buttonWidth = other.buttonWidth,
        buttonHeight = other.buttonHeight,
        buttonSpacing = other.buttonSpacing,
        buttonRadius = other.buttonRadius,
        buttonBorderWidth = other.buttonBorderWidth,
        buttonAlpha = other.buttonAlpha,
        textSize = other.textSize,
        textAlign = other.textAlign,
        layoutMargin = other.layoutMargin,
        showTitle = other.showTitle,
        showDescription = other.showDescription,
        showIcons = other.showIcons,
        iconScale = other.iconScale,
    )
}
