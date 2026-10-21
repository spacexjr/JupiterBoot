package com.spacexjr.jupiter.core.shell

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ShTest {

    @Test
    fun quotesSimpleWord() {
        assertEquals("'ls'", Sh.quote("ls"))
    }

    @Test
    fun quotesWithSpaces() {
        assertEquals("'a b'", Sh.quote("a b"))
    }

    @Test
    fun escapesSingleQuotes() {
        assertEquals("'it'\\''s'", Sh.quote("it's"))
    }

    @Test
    fun joinProducesPosixList() {
        assertEquals("'ls' '-l' '/data'", Sh.join(listOf("ls", "-l", "/data")))
    }

    @Test
    fun shellMetacharactersStayLiteral() {
        val joined = Sh.join(listOf("echo", "\$(rm -rf /)"))
        assertEquals("'echo' '\$(rm -rf /)'", joined)
    }

    @Test
    fun validateAcceptsNormalAbsolutePath() {
        assertEquals("/dev/block/by-name/init_boot_a", Sh.validateSystemPath("/dev/block/by-name/init_boot_a"))
    }

    @Test(expected = IllegalArgumentException::class)
    fun validateRejectsRelative() {
        Sh.validateSystemPath("relative/path")
    }

    @Test(expected = IllegalArgumentException::class)
    fun validateRejectsDotDot() {
        Sh.validateSystemPath("/data/../etc/passwd")
    }

    @Test(expected = IllegalArgumentException::class)
    fun validateRejectsMetacharacters() {
        Sh.validateSystemPath("/data/a;b")
    }

    @Test
    fun quoteRejectsNul() {
        var threw = false
        try {
            Sh.quote("a\u0000b")
        } catch (_: IllegalArgumentException) {
            threw = true
        }
        assertTrue(threw)
    }

    @Test
    fun joinRejectsEmpty() {
        var threw = false
        try {
            Sh.join(emptyList())
        } catch (_: IllegalArgumentException) {
            threw = true
        }
        assertTrue(threw)
    }

    @Test
    fun isSuccessReflectsTimeoutAndExitCode() {
        val ok = CommandRunner.Result(0, "", "", timedOut = false, durationMs = 1)
        val fail = CommandRunner.Result(1, "", "", timedOut = false, durationMs = 1)
        val timeout = CommandRunner.Result(0, "", "", timedOut = true, durationMs = 1)
        assertTrue(ok.isSuccess)
        assertFalse(fail.isSuccess)
        assertFalse(timeout.isSuccess)
    }
}
