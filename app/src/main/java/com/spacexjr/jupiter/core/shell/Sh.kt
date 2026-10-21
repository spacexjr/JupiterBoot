package com.spacexjr.jupiter.core.shell

/**
 * Utilitarios de quoting e validacao para comandos executados via `su -c`.
 *
 * Todo comando privilegiado e construido como argv e convertido para uma
 * unica string com aspas simples POSIX. Isso impede injecao: qualquer
 * caractere especial em um argumento (espacos, $, ;, |, ...) fica literal
 * dentro de aspas simples.
 */
object Sh {

    private const val NUL = '\u0000'

    /** Escapa um valor para uso como palavra literal em sh. */
    fun quote(value: String): String {
        require(!value.contains(NUL)) { "argumento contem NUL" }
        return "'" + value.replace("'", "'\\''") + "'"
    }

    /** Junta argv em uma string segura para `su -c`. */
    fun join(argv: List<String>): String {
        require(argv.isNotEmpty()) { "argv vazio" }
        return argv.joinToString(" ") { quote(it) }
    }

    private val SAFE_PATH = Regex("^/[A-Za-z0-9._+/-]*$")

    /**
     * Valida um caminho de sistema usado em comandos privilegiados.
     * Exige caminho absoluto simples sem `..` e sem caracteres fora do
     * conjunto conservador.
     */
    fun validateSystemPath(path: String): String {
        require(path.startsWith("/")) { "caminho deve ser absoluto: $path" }
        require(!path.contains("..")) { "caminho nao pode conter '..': $path" }
        require(SAFE_PATH.matches(path)) { "caminho contem caracteres nao permitidos: $path" }
        require(path.length <= 4096) { "caminho longo demais" }
        return path
    }
}
