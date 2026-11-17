// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.sheet

/** CSS-style hexadecimal colors, using RGBA rather than Android's ARGB string order. */
public object SheetColors {
    public fun decode(value: String?): Int? {
        if (value == "transparent") return 0
        if (value == null || !value.startsWith('#')) return null
        val hex = value.substring(1)
        if (hex.length !in listOf(3, 6, 8) || hex.any { it.digitToIntOrNull(16) == null })
            return null
        val number = hex.toLong(16)
        return when (hex.length) {
            3 ->
                (0xff000000L or
                        (((number shr 8) and 15) * 0x110000) or
                        (((number shr 4) and 15) * 0x1100) or
                        ((number and 15) * 0x11))
                    .toInt()
            6 -> (0xff000000L or number).toInt()
            else -> (((number and 255) shl 24) or (number shr 8)).toInt()
        }
    }
}
