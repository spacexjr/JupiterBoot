package com.spacexjr.jupiter.core.config

/**
 * Documento INI generico que preserva as linhas originais para round-trip.
 *
 * Reproduz o comportamento do parser do JupiterBoot (jbm_foreach_kv):
 * linhas vazias ou iniciadas por '#', ';' ou '[' sao ignoradas na leitura
 * de chaves, mas permanecem no documento para reescrita.
 */
sealed class IniLine {
    data class Raw(val text: String) : IniLine()
    data class Section(val id: String) : IniLine()
    data class KeyValue(val key: String, val value: String) : IniLine()
    object Blank : IniLine()
}

class IniDocument private constructor(val lines: MutableList<IniLine>) {

    fun sections(): List<SectionBlock> {
        val result = mutableListOf<SectionBlock>()
        var current: SectionBlock? = null
        for (line in lines) {
            when (line) {
                is IniLine.Section -> {
                    current = SectionBlock(line.id, mutableListOf(), mutableListOf())
                    result += current
                }
                is IniLine.KeyValue -> current?.kvLines?.add(line)
                is IniLine.Raw -> current?.preludeLines?.add(line.text)
                IniLine.Blank -> current?.preludeLines?.add("")
            }
        }
        return result
    }

    /** Valores chave=valor fora de secoes (usado pelo config.ini). */
    fun topLevelKv(): List<IniLine.KeyValue> = lines.filterIsInstance<IniLine.KeyValue>()

    fun setTopLevel(key: String, value: String) {
        val idx = lines.indexOfFirst { it is IniLine.KeyValue && it.key == key }
        if (idx >= 0) {
            lines[idx] = IniLine.KeyValue(key, value)
        } else {
            lines += IniLine.KeyValue(key, value)
        }
    }

    fun serialize(): String = lines.joinToString("\n") { line ->
        when (line) {
            is IniLine.Raw -> line.text
            is IniLine.Section -> "[${line.id}]"
            is IniLine.KeyValue -> "${line.key} = ${line.value}"
            IniLine.Blank -> ""
        }
    } + "\n"

    companion object {
        fun parse(text: String): IniDocument {
            val parsed = mutableListOf<IniLine>()
            for (rawLine in text.split('\n')) {
                val line = rawLine.removeSuffix("\r").trim()
                when {
                    line.isEmpty() -> parsed += IniLine.Blank
                    line[0] == '[' -> {
                        val end = line.indexOf(']')
                        val id = if (end > 0) line.substring(1, end) else line.substring(1)
                        parsed += IniLine.Section(id.trim())
                    }
                    line[0] == '#' || line[0] == ';' -> parsed += IniLine.Raw(line)
                    else -> {
                        val eq = line.indexOf('=')
                        if (eq < 0) {
                            parsed += IniLine.Raw(line)
                        } else {
                            val key = line.substring(0, eq).trim()
                            val value = line.substring(eq + 1).trim()
                            parsed += IniLine.KeyValue(key, value)
                        }
                    }
                }
            }
            return IniDocument(parsed)
        }

        fun empty(): IniDocument = IniDocument(mutableListOf())
    }
}

/** Bloco de secao [id] com suas linhas. */
class SectionBlock(
    val id: String,
    val preludeLines: MutableList<String>,
    val kvLines: MutableList<IniLine.KeyValue>,
) {
    fun get(key: String): String? = kvLines.firstOrNull { it.key == key }?.value

    fun set(key: String, value: String) {
        val idx = kvLines.indexOfFirst { it.key == key }
        if (idx >= 0) kvLines[idx] = IniLine.KeyValue(key, value)
        else kvLines += IniLine.KeyValue(key, value)
    }

    fun remove(key: String) {
        kvLines.removeAll { it.key == key }
    }
}
