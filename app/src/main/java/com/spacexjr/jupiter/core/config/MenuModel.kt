package com.spacexjr.jupiter.core.config

/** Estilos de botao aceitos pelo firmware (button_style). */
object ButtonStyle {
    const val SOLID = 0
    const val OUTLINE = 1
    const val MINIMAL = 2
    const val GLASS = 3

    val names = listOf("Solido", "Contornado", "Minimal", "Vidro")

    fun label(style: Int): String = names.getOrElse(style) { names[0] }
}

/** Alinhamento do texto (text_align). */
object TextAlign {
    const val LEFT = 0
    const val CENTER = 1
    const val RIGHT = 2

    val names = listOf("Esquerda", "Centro", "Direita")

    fun label(align: Int): String = names.getOrElse(align) { names[0] }
}

/**
 * Resultado de tema + config.ini, equivalente a jbm_build_effects():
 * o tema resolve `extends`, depois apenas as chaves presentes no config
 * sobrescrevem (bits de mascara).
 */
data class EffectiveMenuStyle(
    val buttonStyle: Int,
    val colorBg: Int,
    val colorFg: Int,
    val colorBorder: Int,
    val colorHighlight: Int,
    val colorSelectedBg: Int,
    val colorPressedBg: Int,
    val colorDisabledText: Int,
    val colorText: Int,
    val colorTextSel: Int,
    val colorAccent: Int,
    val buttonWidth: Int,
    val buttonHeight: Int,
    val buttonSpacing: Int,
    val buttonRadius: Int,
    val buttonBorderWidth: Int,
    val buttonAlpha: Int,
    val textSize: Int,
    val textAlign: Int,
    val layoutMargin: Int,
    val showTitle: Boolean,
    val showDescription: Boolean,
    val showIcons: Boolean,
    val iconScale: Int,
) {
    companion object {
        fun resolve(cfg: ConfigIni, theme: ThemeIni): EffectiveMenuStyle {
            var e = EffectiveMenuStyle(
                buttonStyle = theme.buttonStyle,
                colorBg = theme.colorBg,
                colorFg = theme.colorFg,
                colorBorder = theme.colorBorder,
                colorHighlight = theme.colorHighlight,
                colorSelectedBg = theme.colorSelectedBg,
                colorPressedBg = theme.colorPressedBg,
                colorDisabledText = theme.colorDisabledText,
                colorText = theme.colorText,
                colorTextSel = theme.colorTextSel,
                colorAccent = theme.colorAccent,
                buttonWidth = theme.buttonWidth,
                buttonHeight = theme.buttonHeight,
                buttonSpacing = theme.buttonSpacing,
                buttonRadius = theme.buttonRadius,
                buttonBorderWidth = theme.buttonBorderWidth,
                buttonAlpha = theme.buttonAlpha,
                textSize = theme.textSize,
                textAlign = theme.textAlign,
                layoutMargin = theme.layoutMargin,
                showTitle = theme.showTitle,
                showDescription = theme.showDescription,
                showIcons = theme.showIcons,
                iconScale = theme.iconScale,
            )
            val m = cfg.mask
            if (m and ConfigIni.K_COLOR_BG != 0) e = e.copy(colorBg = cfg.colorBg)
            if (m and ConfigIni.K_COLOR_FG != 0) e = e.copy(colorFg = cfg.colorFg)
            if (m and ConfigIni.K_COLOR_BORDER != 0) e = e.copy(colorBorder = cfg.colorBorder)
            if (m and ConfigIni.K_COLOR_HL != 0) e = e.copy(colorHighlight = cfg.colorHighlight)
            if (m and ConfigIni.K_COLOR_SEL_BG != 0) e = e.copy(colorSelectedBg = cfg.colorSelectedBg)
            if (m and ConfigIni.K_COLOR_PRESSED != 0) e = e.copy(colorPressedBg = cfg.colorPressedBg)
            if (m and ConfigIni.K_COLOR_DISABLED != 0) e = e.copy(colorDisabledText = cfg.colorDisabledText)
            if (m and ConfigIni.K_COLOR_TEXT != 0) e = e.copy(colorText = cfg.colorText)
            if (m and ConfigIni.K_COLOR_TEXTSEL != 0) e = e.copy(colorTextSel = cfg.colorTextSel)
            if (m and ConfigIni.K_COLOR_ACCENT != 0) e = e.copy(colorAccent = cfg.colorAccent)
            if (m and ConfigIni.K_BTN_W != 0) e = e.copy(buttonWidth = cfg.buttonWidth)
            if (m and ConfigIni.K_BTN_H != 0) e = e.copy(buttonHeight = cfg.buttonHeight)
            if (m and ConfigIni.K_BTN_GAP != 0) e = e.copy(buttonSpacing = cfg.buttonSpacing)
            if (m and ConfigIni.K_BTN_R != 0) e = e.copy(buttonRadius = cfg.buttonRadius)
            if (m and ConfigIni.K_BTN_BORDER != 0) e = e.copy(buttonBorderWidth = cfg.buttonBorderWidth)
            if (m and ConfigIni.K_BTN_ALPHA != 0) e = e.copy(buttonAlpha = cfg.buttonAlpha)
            if (m and ConfigIni.K_TEXT_SIZE != 0) e = e.copy(textSize = cfg.textSize)
            if (m and ConfigIni.K_LAYOUT_MARGIN != 0) e = e.copy(layoutMargin = cfg.layoutMargin)
            if (m and ConfigIni.K_ICON_SCALE != 0) e = e.copy(iconScale = cfg.iconScale)
            if (m and ConfigIni.K_BTN_STYLE != 0) e = e.copy(buttonStyle = cfg.buttonStyle)
            if (m and ConfigIni.K_SHOW_TITLE != 0) e = e.copy(showTitle = cfg.showTitle)
            if (m and ConfigIni.K_SHOW_DESC != 0) e = e.copy(showDescription = cfg.showDescription)
            if (m and ConfigIni.K_SHOW_ICONS != 0) e = e.copy(showIcons = cfg.showIcons)
            if (m and ConfigIni.K_TEXT_ALIGN != 0) e = e.copy(textAlign = cfg.textAlign)
            if (m and ConfigIni.K_COLOR_HL != 0 && m and ConfigIni.K_COLOR_SEL_BG == 0) {
                e = e.copy(colorSelectedBg = e.colorHighlight)
            }
            return e.validated()
        }
    }

    fun validated(): EffectiveMenuStyle = copy(
        buttonStyle = buttonStyle.coerceIn(0, 3),
        buttonWidth = buttonWidth.coerceIn(80, 4000),
        buttonHeight = buttonHeight.coerceIn(40, 1200),
        buttonSpacing = buttonSpacing.coerceIn(0, 400),
        buttonRadius = buttonRadius.coerceIn(0, 400),
        buttonBorderWidth = buttonBorderWidth.coerceIn(0, 24),
        buttonAlpha = buttonAlpha.coerceIn(0, 255),
        textSize = textSize.coerceIn(8, 96),
        textAlign = textAlign.coerceIn(0, 2),
        layoutMargin = layoutMargin.coerceIn(0, 35),
        iconScale = iconScale.coerceIn(50, 300),
    )
}

/** Estilo efetivo de uma linha, equivalente a btn_effective(). */
data class EffectiveButtonStyle(
    val style: Int,
    val width: Int,
    val height: Int,
    val radius: Int,
    val border: Int,
    val textAlign: Int,
    val iconScale: Int,
    val showIcon: Boolean,
    val colorBg: Int,
    val colorBorder: Int,
    val colorText: Int,
    val colorTextSel: Int,
) {
    companion object {
        fun resolve(entry: JbmEntry, menu: EffectiveMenuStyle): EffectiveButtonStyle {
            val dims = entry.buttonMetric(menu)
            return EffectiveButtonStyle(
                style = (if (entry.buttonStyle >= 0) entry.buttonStyle else menu.buttonStyle).coerceIn(0, 3),
                width = dims.first,
                height = dims.second,
                radius = (if (entry.buttonRadius >= 0) entry.buttonRadius else menu.buttonRadius).coerceIn(0, 400),
                border = (if (entry.buttonBorderWidth >= 0) entry.buttonBorderWidth
                else menu.buttonBorderWidth).coerceIn(0, 24),
                textAlign = (if (entry.textAlign >= 0) entry.textAlign else menu.textAlign).coerceIn(0, 2),
                iconScale = (if (entry.iconScale > 0) entry.iconScale else menu.iconScale).coerceIn(50, 300),
                showIcon = if (entry.showIcon >= 0) entry.showIcon == 1 else menu.showIcons,
                colorBg = if (entry.colorBg != 0) entry.colorBg else menu.colorSelectedBg,
                colorBorder = if (entry.colorBorder != 0) entry.colorBorder else menu.colorBorder,
                colorText = if (entry.colorText != 0) entry.colorText else menu.colorText,
                colorTextSel = if (entry.colorTextSel != 0) entry.colorTextSel else menu.colorTextSel,
            )
        }
    }
}

private fun JbmEntry.buttonMetric(menu: EffectiveMenuStyle): Pair<Int, Int> {
    val width = if (buttonWidth > 0) buttonWidth.coerceIn(80, 4000) else menu.buttonWidth
    val height = if (buttonHeight > 0) buttonHeight.coerceIn(40, 1200) else menu.buttonHeight
    return width to height
}

/**
 * Geometria da lista, em unidades de projeto 720x1600. Reproduz btn_x(),
 * menu_top() e btn_geom() do firmware; a UI converte para pixels.
 */
object MenuGeometry {
    const val DESIGN_WIDTH = 720f
    const val DESIGN_HEIGHT = 1600f

    fun scale(width: Int, height: Int): Float =
        minOf(width / DESIGN_WIDTH, height / DESIGN_HEIGHT)

    fun menuTop(k: Float, style: EffectiveMenuStyle, showBrand: Boolean = true): Int {
        var top = (DESIGN_HEIGHT * 0.09f * k).toInt()
        if (style.showTitle && showBrand) top += (38f * k).toInt() + (5f * k).toInt()
        top += (45f * k).toInt()
        return top
    }

    data class Row(val x: Int, val y: Int, val width: Int, val height: Int)

    fun rows(
        screenWidth: Int,
        screenHeight: Int,
        style: EffectiveMenuStyle,
        entries: List<JbmEntry>,
        showBrand: Boolean = true,
    ): List<Row> {
        val k = scale(screenWidth, screenHeight)
        val margin = (screenWidth * style.layoutMargin / 100f).toInt()
        val avail = screenWidth - margin * 2
        var yy = menuTop(k, style, showBrand)
        return entries.map { entry ->
            val eff = EffectiveButtonStyle.resolve(entry, style)
            var w = (eff.width * k).toInt().coerceAtMost(avail).coerceAtLeast(80)
            val h = (eff.height * k).toInt().coerceAtLeast(30)
            val row = Row(margin, yy, w, h)
            yy += h + (style.buttonSpacing * k).toInt()
            row
        }
    }

    fun visibleRows(
        screenHeight: Int,
        style: EffectiveMenuStyle,
        entryHeight: Int,
        k: Float,
        showBrand: Boolean = true,
    ): Int {
        val top = menuTop(k, style, showBrand)
        val foot = (150f * k).toInt()
        val avail = screenHeight - top - foot
        val need = entryHeight + (style.buttonSpacing * k).toInt()
        if (need <= 0) return 1
        return (avail / need).coerceAtLeast(1)
    }
}
