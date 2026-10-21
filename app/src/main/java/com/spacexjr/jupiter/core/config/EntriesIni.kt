package com.spacexjr.jupiter.core.config

/**
 * Modelo de entries.ini do JupiterBoot.
 *
 * Espelha jbm_entry_t / jbm_parse_entry_line() de src/jbm_menu.c:
 * uma secao [id] por entrada; chaves conhecidas name, type, path, args,
 * enabled, order, default, behavior_after. Chaves desconhecidas sao
 * preservadas para round-trip.
 */
enum class EntryType(val id: String) {
    SYSTEM("system"),
    RECOVERY("recovery"),
    FASTBOOT("fastboot"),
    DOWNLOAD("download"),
    REBOOT("reboot"),
    POWEROFF("poweroff"),
    SCRIPT("script"),
    BINARY("binary");

    companion object {
        fun fromId(id: String): EntryType? = entries.firstOrNull { it.id == id }

        /** Como jbm_parse_entry_type: desconhecido vira SYSTEM. */
        fun fromIdOrSystem(id: String): EntryType = fromId(id) ?: SYSTEM
    }
}

data class JbmEntry(
    var id: String = "",
    var name: String = "",
    var type: EntryType = EntryType.SYSTEM,
    var path: String = "",
    var args: String = "",
    var enabled: Boolean = true,
    var order: Int = 0,
    var isDefault: Boolean = false,
    var behaviorAfter: Int = 0,
) {
    /** Chaves que o firmware conhece (usado para preservar as demais). */
    val knownKeys: Set<String> = setOf(
        "name", "type", "path", "args", "enabled", "order", "default", "behavior_after",
    )
}

/** Documento de entries com preservacao de chaves/comentarios externos. */
class EntriesDocument {
    val headerLines = mutableListOf<String>()
    val entries = mutableListOf<EntriesSection>()

    fun toText(): String = buildString {
        for (line in headerLines) appendLine(line)
        if (headerLines.isNotEmpty()) appendLine()
        entries.forEachIndexed { index, section ->
            appendLine("[${section.id}]")
            for ((key, value) in section.fields) appendLine("$key = $value")
            for ((key, value) in section.unknownFields) appendLine("$key = $value")
            if (index != entries.lastIndex) appendLine()
        }
    }
}

/** Secao [id] com campos tipados + campos desconhecidos preservados. */
class EntriesSection(
    val id: String,
    val fields: MutableList<Pair<String, String>> = mutableListOf(),
    val unknownFields: MutableList<Pair<String, String>> = mutableListOf(),
) {
    fun get(key: String): String? =
        fields.firstOrNull { it.first == key }?.second
            ?: unknownFields.firstOrNull { it.first == key }?.second
}

object EntriesIniParser {

    fun parse(text: String): EntriesDocument {
        val doc = EntriesDocument()
        var current: EntriesSection? = null
        var inHeader = true

        for (rawLine in text.split('\n')) {
            val line = rawLine.removeSuffix("\r").trim()
            when {
                line.isEmpty() -> if (inHeader) doc.headerLines.add("")
                line[0] == '#' || line[0] == ';' -> if (inHeader) doc.headerLines.add(line)
                line[0] == '[' -> {
                    inHeader = false
                    val end = line.indexOf(']')
                    val id = if (end > 0) line.substring(1, end) else line.substring(1)
                    current = EntriesSection(id.trim())
                    doc.entries += current
                }
                else -> {
                    inHeader = false
                    val eq = line.indexOf('=')
                    if (eq < 0 || current == null) continue
                    val key = line.substring(0, eq).trim()
                    val value = line.substring(eq + 1).trim()
                    val target = if (key in KNOWN_KEYS) current.fields else current.unknownFields
                    target.removeAll { it.first == key }
                    target += key to value
                }
            }
        }
        return doc
    }

    /** Converte para o modelo de edicao (ordem preservada do arquivo). */
    fun toModel(doc: EntriesDocument): MutableList<JbmEntry> =
        doc.entries.map { section -> sectionToEntry(section) }.toMutableList()

    private fun sectionToEntry(section: EntriesSection): JbmEntry {
        val e = JbmEntry(id = section.id)
        section.fields.forEach { (key, value) ->
            when (key) {
                "name" -> e.name = value
                "type" -> e.type = EntryType.fromIdOrSystem(value)
                "path" -> e.path = value
                "args" -> e.args = value
                "enabled" -> e.enabled = JbmValues.parseBool(value)
                "order" -> e.order = JbmValues.parseInt(value, e.order)
                "default" -> e.isDefault = JbmValues.parseBool(value)
                "behavior_after" -> e.behaviorAfter = JbmValues.parseInt(value, e.behaviorAfter)
            }
        }
        return e
    }

    /** Preserva chaves desconhecidas da secao correspondente ao reescrever. */
    fun unknownFieldsFor(doc: EntriesDocument, id: String): List<Pair<String, String>> =
        doc.entries.firstOrNull { it.id == id }?.unknownFields?.toList() ?: emptyList()

    private val KNOWN_KEYS = setOf(
        "name", "type", "path", "args", "enabled", "order", "default", "behavior_after",
    )

    /** Entradas embutidas do firmware (usadas quando o arquivo nao existe). */
    fun builtinEntries(): List<JbmEntry> = listOf(
        JbmEntry("system", "System", EntryType.SYSTEM, order = 0, isDefault = true),
        JbmEntry("recovery", "Recovery", EntryType.RECOVERY, order = 1),
        JbmEntry("fastboot", "Fastboot", EntryType.FASTBOOT, order = 2),
        JbmEntry("download", "Download", EntryType.DOWNLOAD, order = 3),
        JbmEntry("reboot", "Reboot", EntryType.REBOOT, order = 4),
        JbmEntry("poweroff", "Power off", EntryType.POWEROFF, order = 5),
    )

    fun builtinTemplate(): String = buildString {
        append("# JupiterBoot boot menu entries -- generated on first boot.\n")
        append("# Each [section] is one row. Delete this file to get the\n")
        append("# defaults back. type: system|recovery|fastboot|download|\n")
        append("# reboot|poweroff|script|binary\n\n")
        builtinEntries().forEachIndexed { i, e ->
            append("[${e.id}]\n")
            append("name = ${e.name}\n")
            append("type = ${e.type.id}\n")
            append("enabled = 1\n")
            append("order = $i\n")
            append("default = ${if (i == 0) 1 else 0}\n")
            if (i != 5) append("\n")
        }
    }
}

/** Serializa modelo de edicao de volta para texto, reordenando por `order`. */
object EntriesIniSerializer {

    fun write(
        entries: List<JbmEntry>,
        unknownFields: Map<String, List<Pair<String, String>>> = emptyMap(),
        headerLines: List<String> = EntriesIniParser.builtinTemplate().lineSequence()
            .takeWhile { it.isBlank() || it.trimStart().startsWith("#") }
            .toList(),
    ): String {
        val doc = EntriesDocument()
        doc.headerLines += headerLines
        val sorted = entries.sortedBy { it.order }
        for (e in sorted) {
            val section = EntriesSection(e.id)
            section.fields += "name" to e.name
            section.fields += "type" to e.type.id
            section.fields += "enabled" to if (e.enabled) "1" else "0"
            section.fields += "order" to e.order.toString()
            section.fields += "default" to if (e.isDefault) "1" else "0"
            if (e.path.isNotEmpty()) section.fields += "path" to e.path
            if (e.args.isNotEmpty()) section.fields += "args" to e.args
            section.fields += "behavior_after" to e.behaviorAfter.toString()
            unknownFields[e.id]?.forEach { pair ->
                if (pair.first !in section.fields.map { it.first }) section.unknownFields += pair
            }
            doc.entries += section
        }
        return doc.toText()
    }
}

/** Validacao de entradas antes da gravacao. */
object EntriesValidator {

    data class Issue(val entryId: String, val message: String, val fatal: Boolean = true)

    private val ID_REGEX = Regex("^[A-Za-z0-9_-]{1,31}$")

    fun validate(entries: List<JbmEntry>): List<Issue> {
        val issues = mutableListOf<Issue>()
        val seen = mutableSetOf<String>()
        for (e in entries) {
            if (!ID_REGEX.matches(e.id)) {
                issues += Issue(e.id, "id invalido: use 1-31 caracteres [A-Za-z0-9_-]")
            } else if (!seen.add(e.id)) {
                issues += Issue(e.id, "id duplicado")
            }
            if (e.name.isBlank()) {
                issues += Issue(e.id, "nome vazio")
            }
            if (e.name.length > 63) {
                issues += Issue(e.id, "nome longo demais (max 63)")
            }
            if (e.behaviorAfter !in 0..2) {
                issues += Issue(e.id, "behavior_after deve ser 0, 1 ou 2")
            }
            if (e.type == EntryType.SCRIPT || e.type == EntryType.BINARY) {
                if (e.path.isBlank()) {
                    issues += Issue(e.id, "tipo ${e.type.id} exige path")
                } else if (!e.path.startsWith("/")) {
                    issues += Issue(e.id, "path deve ser absoluto")
                } else if (e.path.contains("..")) {
                    issues += Issue(e.id, "path nao pode conter '..'")
                }
                if (e.args.length > 255) {
                    issues += Issue(e.id, "args longo demais (max 255)")
                }
            }
        }
        val defaults = entries.count { it.isDefault }
        if (defaults > 1) {
            issues += Issue(
                entries.first { it.isDefault }.id,
                "multiplas entradas marcadas como default; o firmware usa a primeira",
                fatal = false,
            )
        }
        return issues
    }
}
