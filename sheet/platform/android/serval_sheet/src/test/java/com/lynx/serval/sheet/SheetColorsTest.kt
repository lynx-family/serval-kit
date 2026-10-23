// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.sheet

import org.junit.Assert.*
import org.junit.Test

class SheetColorsTest {
    @Test
    fun colorsUseCssChannelOrder() {
        assertEquals(0x80123456.toInt(), SheetColors.decode("#12345680"))
        assertEquals(0xffaabbcc.toInt(), SheetColors.decode("#abc"))
        assertEquals(0xff123456.toInt(), SheetColors.decode("#123456"))
        assertEquals(0, SheetColors.decode("transparent"))
    }

    @Test
    fun malformedColorsAreRejected() {
        listOf(null, "red", "#12", "#abcdefzz", "#123456junk").forEach {
            assertNull(SheetColors.decode(it))
        }
    }
}
