// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.sheet

public interface SheetListener {
    fun emitOnWillAppear()

    fun emitOnDidAppear()

    fun emitOnWillDisappear()

    fun emitOnDidDisappear()

    fun emitOnDismissEvent()

    fun emitOnNativeDismissEvent()

    fun emitOnNativeDismissPreventedEvent(channel: SheetDismissChannel)

    fun emitOnDetentChanged(index: Int)

    fun onSnapSettled(index: Int, height: Int) = Unit

    fun onNativeDismissStarted(channel: SheetDismissChannel) = Unit
}

public data class SheetArea(val left: Int, val top: Int, val right: Int, val bottom: Int) {
    public val width: Int
        get() = right - left

    public val height: Int
        get() = bottom - top
}

/** Native host policies, captured per controller rather than process-wide. */
public interface SheetHost {
    fun resolveArea(width: Int, height: Int, previous: SheetArea?): SheetArea? = previous

    fun refreshArea(width: Int, height: Int, previous: SheetArea?) = Unit

    fun observeAreaChanges(onChanged: () -> Unit): AutoCloseable = AutoCloseable {}
}

/** Optional diagnostics use opaque identities; navigation types never enter the engine. */
public interface SheetTrace {
    val operation: Any?

    fun beginPresent()

    fun beginProgrammaticDismiss()

    fun beginDismiss(channel: SheetDismissChannel)

    fun ensureDismiss(channel: SheetDismissChannel)

    fun isActiveDismissFrom(channel: SheetDismissChannel): Boolean

    fun nativeDismissPrevented(channel: SheetDismissChannel)

    fun start(isAnimated: Boolean)

    fun complete(operation: Any?)

    fun cancel(reason: String, operation: Any? = this.operation)

    fun dismissCommittedAndComplete(operation: Any?)

    fun dispose(reason: String)

    fun withOperation(operation: Any?, block: () -> Unit)
}
