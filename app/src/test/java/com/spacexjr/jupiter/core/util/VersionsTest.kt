package com.spacexjr.jupiter.core.util

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class VersionsTest {

    @Test
    fun numericSegmentsAreNotComparedLexicographically() {
        assertTrue(Versions.isNewer("1.10", "1.9"))
        assertTrue(Versions.isNewer("2.0", "1.99"))
        assertTrue(Versions.isNewer("1.0.1", "1.0"))
    }

    @Test
    fun ignoresLeadingV() {
        assertEquals(0, Versions.compare("v1.2.3", "1.2.3"))
        assertEquals(0, Versions.compare("V1.2.3", "1.2.3"))
        assertTrue(Versions.isNewer("v1.2.4", "1.2.3"))
    }

    @Test
    fun releaseIsNewerThanPreRelease() {
        assertTrue(Versions.isNewer("1.0", "1.0-rc1"))
        assertFalse(Versions.isNewer("1.0-rc1", "1.0"))
    }

    @Test
    fun equalVersionsCompareZero() {
        assertEquals(0, Versions.compare("1.0", "1.0"))
        assertEquals(0, Versions.compare("1.0.0", "1.0"))
    }

    @Test
    fun emptyIsZero() {
        assertTrue(Versions.isNewer("1.0", ""))
    }
}
