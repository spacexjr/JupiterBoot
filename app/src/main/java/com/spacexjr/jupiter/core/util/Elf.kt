package com.spacexjr.jupiter.core.util

/**
 * Verificação leve de ELF sem depender de bibliotecas nativas.
 * Cabeçalho ELF64 little-endian: ident[0..3] = 0x7F 'E' 'L' 'F',
 * EI_CLASS = 2 (64-bit), e_machine (offset 18, u16) = 0xB7 (AArch64).
 */
object Elf {

    const val EM_AARCH64 = 0xB7
    const val EM_X86_64 = 0x3E

    data class Info(
        val isElf: Boolean,
        val is64Bit: Boolean = false,
        val littleEndian: Boolean = false,
        val machine: Int = -1,
        val isAarch64: Boolean = false,
    ) {
        val describe: String
            get() = when {
                !isElf -> "não é ELF"
                isAarch64 -> "ELF64 AArch64"
                is64Bit && machine == EM_X86_64 -> "ELF64 x86_64"
                is64Bit -> "ELF64 (machine=0x${machine.toString(16)})"
                else -> "ELF32 (machine=0x${machine.toString(16)})"
            }
    }

    fun inspect(header: ByteArray): Info {
        if (header.size < 20) return Info(isElf = false)
        if (header[0] != 0x7F.toByte() || header[1] != 'E'.code.toByte() ||
            header[2] != 'L'.code.toByte() || header[3] != 'F'.code.toByte()
        ) {
            return Info(isElf = false)
        }
        val is64 = header[4].toInt() == 2
        val little = header[5].toInt() == 1
        val machine = (header[18].toInt() and 0xFF) or ((header[19].toInt() and 0xFF) shl 8)
        return Info(
            isElf = true,
            is64Bit = is64,
            littleEndian = little,
            machine = machine,
            isAarch64 = is64 && little && machine == EM_AARCH64,
        )
    }

    fun inspect(file: java.io.File): Info {
        val bytes = file.inputStream().use { input ->
            val buf = ByteArray(64)
            val read = input.read(buf)
            if (read <= 0) ByteArray(0) else buf.copyOf(read)
        }
        return inspect(bytes)
    }
}
