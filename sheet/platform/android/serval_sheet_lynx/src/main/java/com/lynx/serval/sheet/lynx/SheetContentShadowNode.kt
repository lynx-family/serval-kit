// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Portions Copyright (c) 2026 Software Mansion. See sheet/LICENSE.screens.

package com.lynx.serval.sheet.lynx

import com.lynx.tasm.behavior.shadow.AlignContext
import com.lynx.tasm.behavior.shadow.AlignParam
import com.lynx.tasm.behavior.shadow.CustomMeasureFunc
import com.lynx.tasm.behavior.shadow.MeasureContext
import com.lynx.tasm.behavior.shadow.MeasureMode
import com.lynx.tasm.behavior.shadow.MeasureParam
import com.lynx.tasm.behavior.shadow.MeasureResult
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef
import com.lynx.tasm.behavior.shadow.ShadowNode

public open class SheetContentShadowNode : ShadowNode(), CustomMeasureFunc, SheetContentState {
    public var fitContent: Boolean = false
    private var frameWidth = 0f
    private var frameHeight = 0f

    init {
        setCustomMeasureFunc(this)
    }

    // Adaptation: native dialog dimensions replace Fabric state and drive Lynx child layout.
    override fun updateState(
        contentOffsetX: Float,
        contentOffsetY: Float,
        frameWidth: Float,
        frameHeight: Float,
    ) {
        this.frameWidth = frameWidth
        this.frameHeight = frameHeight
        for (index in 0 until childCount) layoutNodeManager.markDirty(getChildAt(index).signature)
        resetIsDirty()
        markDirty()
        setNeedsLayoutForce()
    }

    override fun measure(param: MeasureParam?, context: MeasureContext?): MeasureResult {
        val width = if (frameWidth > 0f) frameWidth else param?.mWidth ?: 0f
        val height = if (frameHeight > 0f) frameHeight else param?.mHeight ?: 0f
        for (index in 0 until childCount) {
            val child = getChildAt(index)
            // Intrinsic content must not inherit the current sheet height: an empty first
            // layout can report 1px, and later content must still be able to grow or shrink.
            val fitsContent = fitContent || child.tagName == "ls-form-sheet-content-wrapper"
            val childHeight = if (fitsContent) 0f else height
            val childHeightMode = if (fitsContent) MeasureMode.UNDEFINED else MeasureMode.EXACTLY
            val childParam =
                MeasureParam().apply {
                    updateConstraints(width, MeasureMode.EXACTLY, childHeight, childHeightMode)
                }
            // Fixed-detent content needs a definite height for percentage and absolute children.
            when (child) {
                is NativeLayoutNodeRef -> child.measureNativeNode(context, childParam)
                else ->
                    layoutNodeManager.measureNativeNode(
                        child.signature,
                        width,
                        MeasureMode.EXACTLY.intValue(),
                        childHeight,
                        childHeightMode.intValue(),
                        true,
                    )
            }
        }
        return MeasureResult(width, height)
    }

    override fun align(param: AlignParam?, context: AlignContext?) {
        for (index in 0 until childCount) {
            when (val child = getChildAt(index)) {
                is NativeLayoutNodeRef -> child.alignNativeNode(context, AlignParam())
                else -> layoutNodeManager.alignNativeNode(child.signature, 0f, 0f)
            }
        }
    }
}
