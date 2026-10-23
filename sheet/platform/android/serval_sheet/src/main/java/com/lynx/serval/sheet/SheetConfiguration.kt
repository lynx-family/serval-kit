// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.sheet

public data class SheetConfiguration(
    val isOpen: Boolean = false,
    val detents: List<Double> = emptyList(),
    val prefersGrabberVisible: Boolean = false,
    val initialDetentIndex: Int = 0,
    val preferredCornerRadius: Float = SYSTEM_DEFAULT_CORNER_RADIUS,
    val preventNativeDismissChannels: Set<SheetDismissChannel> = emptySet(),
    val preventNativeDismissDragFeedback: Boolean = false,
    val nativeContainerBackgroundColor: Int? = null,
    // TypeScript protocol: -1 always dims; -2 never dims.
    val largestUndimmedDetentIndex: Int = -1,
    val gestureEnabled: Boolean = true,
    val animated: Boolean = true,
    val backdropColor: Int = android.graphics.Color.BLACK,
    val backdropOpacity: Float = 0.5f,
) {
    public fun preventsNativeDismiss(channel: SheetDismissChannel): Boolean =
        isOpen && channel in preventNativeDismissChannels

    companion object {
        const val SYSTEM_DEFAULT_CORNER_RADIUS = -1f
    }
}

public enum class SheetDismissChannel(public val eventValue: String) {
    BACK("back"),
    DRAG("drag"),
    BACKDROP("backdrop");

    public companion object {
        public fun fromValue(value: String): SheetDismissChannel? =
            values().firstOrNull { it.eventValue == value }
    }
}
