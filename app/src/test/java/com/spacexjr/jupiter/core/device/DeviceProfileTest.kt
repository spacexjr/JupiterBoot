package com.spacexjr.jupiter.core.device

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class DeviceProfileTest {

    private val json = """
        {
          "profiles": [
            {"id": "generic", "name": "Generic", "codename": ""},
            {"id": "r0q", "name": "Galaxy S22", "codename": "r0q", "orientation": 90,
             "touchCalX0": 10, "touchCalY0": 20, "touchCalX1": 30, "touchCalY1": 40,
             "notes": "test"}
          ]
        }
    """.trimIndent()

    @Test
    fun parsesProfiles() {
        val profiles = DeviceProfile.parse(json)
        assertEquals(2, profiles.size)
        val r0q = profiles[1]
        assertEquals("Galaxy S22", r0q.name)
        assertEquals(90, r0q.orientation)
        assertEquals(10, r0q.touchCalX0)
        assertEquals(40, r0q.touchCalY1)
    }

    @Test
    fun matchesCodenameCaseInsensitively() {
        val profiles = DeviceProfile.parse(json)
        val r0q = DeviceProfile.detect(profiles, "R0Q")
        assertEquals("r0q", r0q?.id)
    }

    @Test
    fun genericProfileNeverMatches() {
        val profiles = DeviceProfile.parse(json)
        assertNull(DeviceProfile.detect(profiles, ""))
        assertTrue(profiles[0].codename.isEmpty())
    }

    @Test
    fun missingFieldsUseAutomaticDefaults() {
        val profiles = DeviceProfile.parse("""{"profiles":[{"id":"x"}]}""")
        val x = profiles.single()
        assertEquals(-1, x.touchCalX0)
        assertEquals(0, x.orientation)
    }
}
