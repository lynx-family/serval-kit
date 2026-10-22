// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Portions Copyright (c) 2026 Software Mansion. See sheet/LICENSE.screens.
package com.lynx.serval.sheet

public class SheetDetents private constructor(private val values: List<Double>) {
    val count: Int
        get() = values.size

    val isFitToContents: Boolean
        get() = values == listOf(FIT_TO_CONTENTS)

    fun heightAt(index: Int, availableHeight: Int): Int = (values[index] * availableHeight).toInt()

    fun largestHeight(availableHeight: Int): Int = heightAt(values.lastIndex, availableHeight)

    companion object {
        private const val FIT_TO_CONTENTS = -1.0
        private const val MAX_DETENTS = 3

        fun parse(rawValues: List<Double>): SheetDetents {
            val values = if (rawValues.isEmpty()) listOf(1.0) else rawValues
            return try {
                require(values.size <= MAX_DETENTS) {
                    "Maximum of $MAX_DETENTS detents supported, got ${values.size}."
                }
                val isFitToContents = values == listOf(FIT_TO_CONTENTS)
                if (!isFitToContents) {
                    require(values.all { it in 0.0..1.0 }) {
                        "Detent values must be within 0.0 and 1.0."
                    }
                    require(values == values.distinct().sorted()) {
                        "Detents must be sorted in strictly ascending order."
                    }
                }
                SheetDetents(values)
            } catch (error: IllegalArgumentException) {
                android.util.Log.e(
                    "ServalSheet",
                    "Invalid Sheet detents: ${error.message} Falling back to [1.0].",
                )
                SheetDetents(listOf(1.0))
            }
        }
    }
}

public data class SheetDetentGeometry(val availableHeight: Int, val sheetHeights: List<Int>)

public fun SheetDetents.resolveGeometry(windowHeight: Int): SheetDetentGeometry {
    val availableHeight = windowHeight.coerceAtLeast(1)
    val sheetHeights =
        if (isFitToContents) {
            emptyList()
        } else {
            List(count) { index -> heightAt(index, availableHeight) }
        }
    return SheetDetentGeometry(availableHeight, sheetHeights)
}

internal fun visibleSheetSurfaceHeight(
    parentHeight: Int,
    sheetTop: Int,
    maximumHeight: Int,
    unmeasuredHeight: Int = 0,
): Int =
    if (parentHeight <= 0 || maximumHeight <= 0) {
        // Insets can arrive before the first layout. Keep the configured detent until bounds exist.
        unmeasuredHeight
    } else {
        (parentHeight - sheetTop).coerceIn(0, maximumHeight)
    }
