// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Portions Copyright (c) 2026 Software Mansion. See sheet/LICENSE.screens.

package com.lynx.serval.sheet.lynx

import android.annotation.SuppressLint
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import com.lynx.tasm.behavior.LynxContext

@SuppressLint("ViewConstructor")
public open class SheetContentView(
    context: LynxContext,
    private val onSizeChangedCallback: (width: Int, height: Int) -> Unit,
    private val dispatchLynxTouchEvent: (MotionEvent) -> Boolean,
) : ViewGroup(context) {
    public var contentSizeChangeDelegate: SheetContentSizeChangeDelegate? = null

    override fun onViewAdded(child: View) {
        super.onViewAdded(child)
        (child as? SheetContentSizeChangeProvider)?.setContentSizeChangeDelegate(
            contentSizeChangeDelegate
        )
    }

    override fun onViewRemoved(child: View) {
        super.onViewRemoved(child)
        (child as? SheetContentSizeChangeProvider)?.setContentSizeChangeDelegate(null)
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        if (w != oldw || h != oldh) onSizeChangedCallback(w, h)
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val height = MeasureSpec.getSize(heightMeasureSpec)
        for (index in 0 until childCount) {
            val child = getChildAt(index)
            val childHeightMode =
                if (child is SheetContentSizeChangeProvider) MeasureSpec.AT_MOST
                else MeasureSpec.EXACTLY
            child.measure(
                MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(height, childHeightMode),
            )
        }
        setMeasuredDimension(width, height)
    }

    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        for (index in 0 until childCount) {
            val child = getChildAt(index)
            child.layout(0, 0, child.measuredWidth, child.measuredHeight)
        }
    }

    override fun dispatchTouchEvent(event: MotionEvent): Boolean {
        // Adaptation: the dialog is a separate Android window, outside LynxView's normal
        // dispatcher.
        val lynxConsumed = dispatchLynxTouchEvent(event)
        return super.dispatchTouchEvent(event) || lynxConsumed
    }

    @SuppressLint("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean = true
}
