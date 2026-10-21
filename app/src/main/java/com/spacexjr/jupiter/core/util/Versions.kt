package com.spacexjr.jupiter.core.util

/**
 * Comparação de versões numérica por segmentos (não lexicográfica).
 *
 * Regras:
 * - prefixos não numéricos comuns ("v", "V", "release-") são ignorados;
 * - segmentos são separados por '.', '-', '_' ou fim de string;
 * - segmentos puramente numéricos comparam como inteiros (1.10 > 1.9);
 * - segmentos com sufixo alfanumérico comparan numericamente e depois
 *   alfabeticamente ("1.0-rc1" < "1.0");
 * - strings vazias empilham contra 0.
 */
object Versions {

    private val SEPARATOR = Regex("[._-]")

    fun compare(a: String, b: String): Int {
        val pa = segments(a)
        val pb = segments(b)
        val n = maxOf(pa.size, pb.size)
        for (i in 0 until n) {
            val sa = pa.getOrNull(i) ?: Segment(0, "")
            val sb = pb.getOrNull(i) ?: Segment(0, "")
            val cmp = sa.compareTo(sb)
            if (cmp != 0) return cmp
        }
        return 0
    }

    fun isNewer(remote: String, local: String): Boolean = compare(remote, local) > 0

    private data class Segment(val number: Int, val suffix: String) : Comparable<Segment> {
        override fun compareTo(other: Segment): Int {
            val byNumber = number.compareTo(other.number)
            if (byNumber != 0) return byNumber
            // Sem sufixo ("final") > com sufxo ("rc1"): 1.0 > 1.0-rc1.
            return when {
                suffix.isEmpty() && other.suffix.isEmpty() -> 0
                suffix.isEmpty() -> 1
                other.suffix.isEmpty() -> -1
                else -> suffix.compareTo(other.suffix)
            }
        }
    }

    private fun segments(raw: String): List<Segment> {
        var s = raw.trim()
        while (s.isNotEmpty() && s[0].isLetter()) s = s.substring(1) // ignora "v", "release" inicial
        if (s.isEmpty()) return listOf(Segment(0, ""))
        return s.split(SEPARATOR).map { part ->
            val digits = part.takeWhile { it.isDigit() }
            val suffix = part.drop(digits.length)
            Segment(digits.toIntOrNull() ?: 0, suffix)
        }
    }
}
