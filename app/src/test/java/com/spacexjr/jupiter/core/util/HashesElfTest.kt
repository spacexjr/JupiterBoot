package com.spacexjr.jupiter.core.util

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class HashesTest {

    @Test
    fun sha256OfEmptyIsKnown() {
        assertEquals(
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            Hashes.sha256Hex(ByteArray(0)),
        )
    }

    @Test
    fun sha256OfAbcIsKnown() {
        assertEquals(
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            Hashes.sha256Hex("abc".toByteArray()),
        )
    }
}

class ElfTest {

    private fun header(bytes: List<Int>): ByteArray =
        ByteArray(bytes.size) { bytes[it].toByte() }

    @Test
    fun detectsAarch64LittleEndian() {
        val h = header(
            listOf(
                0x7F, 0x45, 0x4C, 0x46, // 0x7F E L F
                0x02, 0x01,             // 64-bit, little-endian
                0x01, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0xB7, 0x00, // e_machine = 0xB7 (AArch64)
            ),
        )
        val info = Elf.inspect(h)
        assertTrue(info.isElf)
        assertTrue(info.is64Bit)
        assertTrue(info.littleEndian)
        assertTrue(info.isAarch64)
    }

    @Test
    fun rejectsNonElf() {
        val info = Elf.inspect("not an elf file at all".toByteArray())
        assertFalse(info.isElf)
    }

    @Test
    fun rejectsShortHeader() {
        assertFalse(Elf.inspect(byteArrayOf(0x7F, 0x45)).isElf)
    }

    @Test
    fun detectsX86_64() {
        val h = header(
            listOf(
                0x7F, 0x45, 0x4C, 0x46,
                0x02, 0x01, 0x01, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x3E, 0x00,
            ),
        )
        val info = Elf.inspect(h)
        assertTrue(info.isElf)
        assertFalse(info.isAarch64)
        assertEquals(Elf.EM_X86_64, info.machine)
    }
}
