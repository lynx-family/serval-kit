// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Portions Copyright (c) 2026 Software Mansion. See sheet/LICENSE.screens.
package com.lynx.serval.sheet

internal fun resolveSheetAnchorTops(
    parentHeight: Int,
    sheetHeight: Int,
    detentHeights: List<Int>,
    fitToContents: Boolean,
    availableTop: Int = 0,
    availableBottom: Int = parentHeight,
): List<Int> {
    val boundedBottom = availableBottom.coerceIn(1, parentHeight.coerceAtLeast(1))
    val boundedTop = availableTop.coerceIn(0, boundedBottom - 1)
    val availableHeight = boundedBottom - boundedTop
    val heights =
        if (fitToContents) {
            listOf(sheetHeight)
        } else {
            detentHeights
        }
    val rawTops =
        heights.map { height -> boundedBottom - height.coerceIn(0, availableHeight) }.sorted()
    if (availableHeight < rawTops.lastIndex) {
        return rawTops
    }
    val resolvedTops = ArrayList<Int>(rawTops.size)
    rawTops.forEachIndexed { index, top ->
        val minimumTop = if (index == 0) boundedTop else resolvedTops.last() + 1
        val maximumTop = boundedBottom - (rawTops.lastIndex - index)
        resolvedTops += top.coerceIn(minimumTop, maximumTop)
    }
    return resolvedTops
}

internal fun selectSheetAnchorTop(
    anchorTops: List<Int>,
    currentTop: Int,
    verticalVelocity: Float,
    minimumFlingVelocity: Float,
): Int {
    require(anchorTops.isNotEmpty())
    return when {
        verticalVelocity < 0f && -verticalVelocity >= minimumFlingVelocity ->
            anchorTops.lastOrNull { it < currentTop } ?: anchorTops.first()
        verticalVelocity > 0f && verticalVelocity >= minimumFlingVelocity ->
            anchorTops.firstOrNull { it > currentTop } ?: anchorTops.last()
        else ->
            anchorTops.minByOrNull { top -> kotlin.math.abs(top - currentTop) }
                ?: anchorTops.first()
    }
}
