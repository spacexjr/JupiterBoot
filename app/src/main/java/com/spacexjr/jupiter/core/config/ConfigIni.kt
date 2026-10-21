package com.spacexjr.jupiter.core.config

/**
 * Modelo e parser de config.ini do JupiterBoot.
 *
 * Espelha fielmente jbm_set_default_config() e jbm_parse_config_line()
 * de src/jbm_menu.c. Cores sao ARGB (alpha 0xFF adicionado quando o valor
 * tem 6 digitos ou menos); chaves ausentes mantem o padrao e nao ativam a
 * mascara de override.
 */
data class ConfigIni(
    var theme: String = "default",
    var background: String = "",
    var buttonStyle: Int = 0,
    var colorBg: Int = 0xFF020202.toInt(),
    var colorFg: Int = 0xFF20242C.toInt(),
    var colorBorder: Int = 0xFFD88A4E.toInt(),
    var colorHighlight: Int = 0xFFB86837.toInt(),
    var colorText: Int = 0xFF858585.toInt(),
    var colorTextSel: Int = 0xFFFFFFFF.toInt(),
    var colorAccent: Int = 0xFFC87335.toInt(),
    var buttonWidth: Int = 430,
    var buttonHeight: Int = 94,
    var buttonSpacing: Int = 12,
    var buttonRadius: Int = 7,
    var textSize: Int = 19,
    var menuTimeoutSec: Int = 15,
    var defaultEntry: Int = -1,
    var showCountdown: Boolean = true,
    var showBrand: Boolean = true,
    var orientation: Int = 0, // 0/90/180/270; -1 = auto
    var inputDevice: String = "",
    var touchCalX0: Int = -1,
    var touchCalY0: Int = -1,
    var touchCalX1: Int = -1,
    var touchCalY1: Int = -1,
    var preferDrm: Boolean = false,
    var fallbackToFb: Boolean = true,
    var logLevel: Int = 6,
    var logFile: Boolean = true,
    /** Bits JBM_K_* das chaves explicitamente presentes no arquivo. */
    val mask: Int = 0,
) {
    companion object {
        const val K_COLOR_BG = 0x0001
        const val K_COLOR_FG = 0x0002
        const val K_COLOR_BORDER = 0x0004
        const val K_COLOR_HL = 0x0008
        const val K_COLOR_TEXT = 0x0010
        const val K_COLOR_TEXTSEL = 0x0020
        const val K_COLOR_ACCENT = 0x0040
        const val K_BTN_W = 0x0080
        const val K_BTN_H = 0x0100
        const val K_BTN_GAP = 0x0200
        const val K_BTN_R = 0x0400
        const val K_TEXT_SIZE = 0x0800

        fun defaults() = ConfigIni()

        /** Espelha o template gerado por jbm_write_default_config(). */
        fun defaultTemplate(): String = buildString {
            append("# JupiterBoot configuration -- generated on first boot.\n")
            append("# Edit the values below and reboot. Delete this file to get the\n")
            append("# defaults back. Full reference: README.md (Configuration).\n\n")
            append("# Look and feel\n")
            append("theme = default\n")
            append("# background = /cache/Jupiter/background.bmp\n")
            append("# Colours/metrics live in themes/default.ini. Uncomment a line\n")
            append("# here to override the theme just for this device.\n")
            append("# button_style = 0\n")
            append("# color_bg = #020202\n")
            append("# color_fg = #20242c\n")
            append("# color_border = #d88a4e\n")
            append("# color_highlight = #b86837\n")
            append("# color_text = #858585\n")
            append("# color_text_sel = #ffffff\n")
            append("# color_accent = #c87335\n")
            append("# button_width = 430\n")
            append("# button_height = 94\n")
            append("# button_spacing = 12\n")
            append("# button_radius = 7\n")
            append("# text_size = 19\n\n")
            append("# Behaviour\n")
            append("menu_timeout_sec = 15\n")
            append("show_countdown = 1\n")
            append("show_brand = 1\n")
            append("default_entry = -1\n\n")
            append("# Display (orientation: 0/90/180/270 or auto)\n")
            append("orientation = 0\n")
            append("prefer_drm = 0\n")
            append("fallback_to_fb = 1\n")
            append("# input_device = event2\n")
            append("# touch_cal_x0 = 0\n# touch_cal_y0 = 0\n# touch_cal_x1 = 0\n# touch_cal_y1 = 0\n\n")
            append("# Logging\n")
            append("log_level = 6\n")
            append("log_file = 1\n")
        }
    }
}

/** Parsing de cores/bools/inteiros identico ao do firmware. */
object JbmValues {

    /** Retorna ARGB opaco ou null quando invalido (espelho de jbm_parse_color). */
    fun parseColor(raw: String?): Int? {
        if (raw.isNullOrEmpty()) return null
        var s = raw.trim()
        if (s.startsWith("#")) s = s.substring(1)
        if (s.length >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s = s.substring(2)
        var value = 0L
        var n = 0
        for (i in s.indices.take(8)) {
            val c = s[i]
            val v = when (c) {
                in '0'..'9' -> c - '0'
                in 'a'..'f' -> c - 'a' + 10
                in 'A'..'F' -> c - 'A' + 10
                else -> break
            }
            value = (value shl 4) or v.toLong()
            n++
        }
        if (n == 0) return null
        if (n <= 6) value = value or 0xFF000000L
        return value.toInt()
    }

    /** Espelho de jbm_parse_bool: primeiro caractere em {1,t,T,y,Y,o,O}. */
    fun parseBool(raw: String?): Boolean {
        val s = raw?.trim() ?: return false
        if (s.isEmpty()) return false
        return s[0] in setOf('1', 't', 'T', 'y', 'Y', 'o', 'O')
    }

    /** Espelho de jbm_parse_int/atoi: numeros com sinal, resto ignorado. */
    fun parseInt(raw: String?, def: Int): Int {
        val s = raw?.trim() ?: return def
        if (s.isEmpty()) return def
        var i = 0
        var sign = 1
        if (s[i] == '+' || s[i] == '-') {
            if (s[i] == '-') sign = -1
            i++
        }
        var acc = 0L
        var any = false
        while (i < s.length && s[i].isDigit()) {
            acc = acc * 10 + (s[i] - '0')
            if (acc > Int.MAX_VALUE) acc = Int.MAX_VALUE.toLong()
            any = true
            i++
        }
        if (!any) return def
        return (sign * acc).toInt()
    }

    fun formatColor(argb: Int): String = "#%06x".format(argb and 0xFFFFFF)
}

object ConfigIniParser {

    fun parse(text: String): ConfigIni {
        val c = ConfigIni.defaults().copy()
        var mask = 0
        for (line in text.split('\n')) {
            val trimmed = line.removeSuffix("\r").trim()
            if (trimmed.isEmpty() || trimmed[0] == '#' || trimmed[0] == ';' || trimmed[0] == '[') continue
            val eq = trimmed.indexOf('=')
            if (eq < 0) continue
            val key = trimmed.substring(0, eq).trim()
            val value = trimmed.substring(eq + 1).trim()
            when (key) {
                "background" -> c.background = value
                "theme" -> c.theme = value
                "button_style" -> c.buttonStyle = JbmValues.parseInt(value, c.buttonStyle)
                "color_bg" -> JbmValues.parseColor(value)?.let { c.colorBg = it; mask = mask or ConfigIni.K_COLOR_BG }
                "color_fg" -> JbmValues.parseColor(value)?.let { c.colorFg = it; mask = mask or ConfigIni.K_COLOR_FG }
                "color_border" -> JbmValues.parseColor(value)?.let { c.colorBorder = it; mask = mask or ConfigIni.K_COLOR_BORDER }
                "color_highlight" -> JbmValues.parseColor(value)?.let { c.colorHighlight = it; mask = mask or ConfigIni.K_COLOR_HL }
                "color_text" -> JbmValues.parseColor(value)?.let { c.colorText = it; mask = mask or ConfigIni.K_COLOR_TEXT }
                "color_text_sel" -> JbmValues.parseColor(value)?.let { c.colorTextSel = it; mask = mask or ConfigIni.K_COLOR_TEXTSEL }
                "color_accent" -> JbmValues.parseColor(value)?.let { c.colorAccent = it; mask = mask or ConfigIni.K_COLOR_ACCENT }
                "button_width" -> c.buttonWidth = JbmValues.parseInt(value, c.buttonWidth).also { mask = mask or ConfigIni.K_BTN_W }
                "button_height" -> c.buttonHeight = JbmValues.parseInt(value, c.buttonHeight).also { mask = mask or ConfigIni.K_BTN_H }
                "button_spacing" -> c.buttonSpacing = JbmValues.parseInt(value, c.buttonSpacing).also { mask = mask or ConfigIni.K_BTN_GAP }
                "button_radius" -> c.buttonRadius = JbmValues.parseInt(value, c.buttonRadius).also { mask = mask or ConfigIni.K_BTN_R }
                "text_size" -> c.textSize = JbmValues.parseInt(value, c.textSize).also { mask = mask or ConfigIni.K_TEXT_SIZE }
                "menu_timeout_sec" -> c.menuTimeoutSec = JbmValues.parseInt(value, c.menuTimeoutSec)
                "default_entry" -> c.defaultEntry = JbmValues.parseInt(value, c.defaultEntry)
                "show_countdown" -> c.showCountdown = JbmValues.parseBool(value)
                "show_brand" -> c.showBrand = JbmValues.parseBool(value)
                "orientation" -> c.orientation = parseOrientation(value, c.orientation)
                "input_device" -> c.inputDevice = value
                "touch_cal_x0" -> c.touchCalX0 = JbmValues.parseInt(value, c.touchCalX0)
                "touch_cal_y0" -> c.touchCalY0 = JbmValues.parseInt(value, c.touchCalY0)
                "touch_cal_x1" -> c.touchCalX1 = JbmValues.parseInt(value, c.touchCalX1)
                "touch_cal_y1" -> c.touchCalY1 = JbmValues.parseInt(value, c.touchCalY1)
                "prefer_drm" -> c.preferDrm = JbmValues.parseBool(value)
                "fallback_to_fb" -> c.fallbackToFb = JbmValues.parseBool(value)
                "log_level" -> c.logLevel = JbmValues.parseInt(value, c.logLevel)
                "log_file" -> c.logFile = JbmValues.parseBool(value)
            }
        }
        return c.copy(mask = mask)
    }

    private fun parseOrientation(value: String, def: Int): Int {
        val v = value.trim()
        if (v == "auto") return -1
        var o = JbmValues.parseInt(v, def)
        if (o == 1 || o == 2 || o == 3) o *= 90
        return o
    }
}

/**
 * Gravacao do config.ini preservando a estrutura do arquivo original.
 *
 * - Chaves existentes sao atualizadas no lugar (comentario inline removido
 *   na linha alterada, pois o parser do firmware inclui o comentario no
 *   valor).
 * - Chaves novas sao adicionadas ao final sob um comentario de secao.
 * - Linhas de comentario e secoes nao tocadas permanecem intactas.
 */
object ConfigIniSerializer {

    fun write(cfg: ConfigIni, originalText: String?): String {
        if (originalText == null) return renderFull(cfg)

        val doc = IniDocument.parse(originalText)
        val updates = linkedMapOf<String, String>()

        // Sempre define o conjunto completo de chaves gerenciadas; o loop
        // abaixo so grava no documento as que existem ou as explicitamente
        // novas (definidas pelo chamador).
        updates["theme"] = cfg.theme
        if (cfg.background.isNotEmpty()) updates["background"] = cfg.background
        updates["menu_timeout_sec"] = cfg.menuTimeoutSec.toString()
        updates["show_countdown"] = if (cfg.showCountdown) "1" else "0"
        updates["show_brand"] = if (cfg.showBrand) "1" else "0"
        updates["default_entry"] = cfg.defaultEntry.toString()
        updates["orientation"] = if (cfg.orientation == -1) "auto" else cfg.orientation.toString()
        updates["prefer_drm"] = if (cfg.preferDrm) "1" else "0"
        updates["fallback_to_fb"] = if (cfg.fallbackToFb) "1" else "0"
        updates["log_level"] = cfg.logLevel.toString()
        updates["log_file"] = if (cfg.logFile) "1" else "0"
        updates["button_style"] = cfg.buttonStyle.toString()
        updates["color_bg"] = JbmValues.formatColor(cfg.colorBg)
        updates["color_fg"] = JbmValues.formatColor(cfg.colorFg)
        updates["color_border"] = JbmValues.formatColor(cfg.colorBorder)
        updates["color_highlight"] = JbmValues.formatColor(cfg.colorHighlight)
        updates["color_text"] = JbmValues.formatColor(cfg.colorText)
        updates["color_text_sel"] = JbmValues.formatColor(cfg.colorTextSel)
        updates["color_accent"] = JbmValues.formatColor(cfg.colorAccent)
        updates["button_width"] = cfg.buttonWidth.toString()
        updates["button_height"] = cfg.buttonHeight.toString()
        updates["button_spacing"] = cfg.buttonSpacing.toString()
        updates["button_radius"] = cfg.buttonRadius.toString()
        updates["text_size"] = cfg.textSize.toString()
        updates["touch_cal_x0"] = cfg.touchCalX0.toString()
        updates["touch_cal_y0"] = cfg.touchCalY0.toString()
        updates["touch_cal_x1"] = cfg.touchCalX1.toString()
        updates["touch_cal_y1"] = cfg.touchCalY1.toString()
        if (cfg.inputDevice.isNotEmpty()) updates["input_device"] = cfg.inputDevice

        val existingKeys = doc.topLevelKv().map { it.key }.toSet()
        val missingKeys = updates.keys.filter { it !in existingKeys }

        // Atualiza chaves existentes.
        for (i in doc.lines.indices) {
            val line = doc.lines[i]
            if (line is IniLine.KeyValue && updates.containsKey(line.key)) {
                doc.lines[i] = IniLine.KeyValue(line.key, updates.getValue(line.key))
            }
        }

        if (missingKeys.isNotEmpty()) {
            doc.lines += IniLine.Blank
            doc.lines += IniLine.Raw("# Gerenciado pelo app Jupiter (chaves novas)")
            for (key in missingKeys) {
                doc.lines += IniLine.KeyValue(key, updates.getValue(key))
            }
        }

        return doc.serialize()
    }

    private fun renderFull(cfg: ConfigIni): String = buildString {
        append("# JupiterBoot configuration (gerado pelo app Jupiter).\n\n")
        append("theme = ${cfg.theme}\n")
        if (cfg.background.isNotEmpty()) append("background = ${cfg.background}\n")
        append("button_style = ${cfg.buttonStyle}\n")
        append("color_bg = ${JbmValues.formatColor(cfg.colorBg)}\n")
        append("color_fg = ${JbmValues.formatColor(cfg.colorFg)}\n")
        append("color_border = ${JbmValues.formatColor(cfg.colorBorder)}\n")
        append("color_highlight = ${JbmValues.formatColor(cfg.colorHighlight)}\n")
        append("color_text = ${JbmValues.formatColor(cfg.colorText)}\n")
        append("color_text_sel = ${JbmValues.formatColor(cfg.colorTextSel)}\n")
        append("color_accent = ${JbmValues.formatColor(cfg.colorAccent)}\n")
        append("button_width = ${cfg.buttonWidth}\n")
        append("button_height = ${cfg.buttonHeight}\n")
        append("button_spacing = ${cfg.buttonSpacing}\n")
        append("button_radius = ${cfg.buttonRadius}\n")
        append("text_size = ${cfg.textSize}\n")
        append("menu_timeout_sec = ${cfg.menuTimeoutSec}\n")
        append("default_entry = ${cfg.defaultEntry}\n")
        append("show_countdown = ${if (cfg.showCountdown) 1 else 0}\n")
        append("show_brand = ${if (cfg.showBrand) 1 else 0}\n")
        append("orientation = ${if (cfg.orientation == -1) "auto" else cfg.orientation}\n")
        append("prefer_drm = ${if (cfg.preferDrm) 1 else 0}\n")
        append("fallback_to_fb = ${if (cfg.fallbackToFb) 1 else 0}\n")
        if (cfg.inputDevice.isNotEmpty()) append("input_device = ${cfg.inputDevice}\n")
        append("touch_cal_x0 = ${cfg.touchCalX0}\n")
        append("touch_cal_y0 = ${cfg.touchCalY0}\n")
        append("touch_cal_x1 = ${cfg.touchCalX1}\n")
        append("touch_cal_y1 = ${cfg.touchCalY1}\n")
        append("log_level = ${cfg.logLevel}\n")
        append("log_file = ${if (cfg.logFile) 1 else 0}\n")
    }
}

/** Validacao de valores antes da gravacao. */
object ConfigIniValidator {

    data class Issue(val key: String, val message: String)

    fun validate(cfg: ConfigIni): List<Issue> {
        val issues = mutableListOf<Issue>()
        if (cfg.menuTimeoutSec < 0 || cfg.menuTimeoutSec > 3600) {
            issues += Issue("menu_timeout_sec", "deve estar entre 0 e 3600")
        }
        if (cfg.buttonWidth < 50 || cfg.buttonWidth > 4000) {
            issues += Issue("button_width", "deve estar entre 50 e 4000")
        }
        if (cfg.buttonHeight < 20 || cfg.buttonHeight > 2000) {
            issues += Issue("button_height", "deve estar entre 20 e 2000")
        }
        if (cfg.buttonSpacing < 0 || cfg.buttonSpacing > 500) {
            issues += Issue("button_spacing", "deve estar entre 0 e 500")
        }
        if (cfg.buttonRadius < 0 || cfg.buttonRadius > 200) {
            issues += Issue("button_radius", "deve estar entre 0 e 200")
        }
        if (cfg.textSize < 8 || cfg.textSize > 64) {
            issues += Issue("text_size", "deve estar entre 8 e 64")
        }
        if (cfg.orientation != -1 && cfg.orientation !in setOf(0, 90, 180, 270)) {
            issues += Issue("orientation", "use auto, 0, 90, 180 ou 270")
        }
        if (cfg.defaultEntry < -1) {
            issues += Issue("default_entry", "-1 ou um indice >= 0")
        }
        if (cfg.logLevel < 0 || cfg.logLevel > 7) {
            issues += Issue("log_level", "deve estar entre 0 e 7")
        }
        if (cfg.theme.isNotEmpty() && !Regex("^[A-Za-z0-9_-]+$").matches(cfg.theme)) {
            issues += Issue("theme", "nome de tema com caracteres invalidos")
        }
        if (cfg.background.isNotEmpty() && !cfg.background.startsWith("/")) {
            issues += Issue("background", "use caminho absoluto ou deixe vazio")
        }
        return issues
    }
}
