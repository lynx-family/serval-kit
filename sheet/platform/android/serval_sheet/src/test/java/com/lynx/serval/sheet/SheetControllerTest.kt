// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.sheet

import android.app.Activity
import android.os.Looper
import android.view.View
import java.time.Duration
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.Robolectric
import org.robolectric.RobolectricTestRunner
import org.robolectric.Shadows.shadowOf
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [33], manifest = Config.NONE)
class SheetControllerTest {
    private fun runSheet(test: (SheetController, MutableList<String>) -> Unit) {
        val host = Robolectric.buildActivity(Activity::class.java).setup()
        val activity = host.get()
        activity.setTheme(
            com.google.android.material.R.style.Theme_MaterialComponents_Light_NoActionBar
        )
        val events = mutableListOf<String>()
        val controller =
            SheetController(
                activity,
                View(activity),
                object : SheetListener {
                    override fun emitOnWillAppear() {
                        events += "willAppear"
                    }

                    override fun emitOnDidAppear() {
                        events += "didAppear"
                    }

                    override fun emitOnWillDisappear() {
                        events += "willDisappear"
                    }

                    override fun emitOnDidDisappear() {
                        events += "didDisappear"
                    }

                    override fun emitOnDismissEvent() {
                        events += "dismiss"
                    }

                    override fun emitOnNativeDismissEvent() {
                        events += "nativeDismiss"
                    }

                    override fun emitOnNativeDismissPreventedEvent(channel: SheetDismissChannel) {
                        events += channel.eventValue
                    }

                    override fun emitOnDetentChanged(index: Int) {
                        events += "index:$index"
                    }
                },
            )
        try {
            test(controller, events)
        } finally {
            controller.dispose()
            host.pause().stop().destroy()
        }
    }

    @Test
    fun commandDismissRemainsAvailableWhenUserDismissalIsBlocked() =
        runSheet { controller, events ->
            val config =
                SheetConfiguration(
                    isOpen = true,
                    detents = listOf(0.5, 0.9),
                    animated = false,
                    preventNativeDismissChannels = SheetDismissChannel.values().toSet(),
                )
            controller.apply(config)
            shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1))
            assertTrue(controller.isOpen)
            controller.apply(config.copy(isOpen = false))
            shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1))
            assertFalse(controller.isOpen)
            assertEquals(1, events.count { it == "dismiss" })
            assertEquals(1, events.count { it == "didAppear" })
        }

    @Test
    fun repeatedRequestedOpenDoesNotRepeatAppearance() = runSheet { controller, events ->
        val config = SheetConfiguration(isOpen = true, detents = listOf(0.5), animated = false)
        controller.apply(config)
        shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1))
        controller.apply(config.copy(prefersGrabberVisible = true))
        shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1))
        assertEquals(1, events.count { it == "willAppear" })
        assertEquals(1, events.count { it == "didAppear" })
    }

    @Test
    fun snapRejectsHiddenAndInvalidTargets() = runSheet { controller, _ ->
        assertFalse(controller.snapTo(0, false))
        controller.apply(
            SheetConfiguration(isOpen = true, detents = listOf(0.5, 0.9), animated = false)
        )
        shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1))
        assertFalse(controller.snapTo(2, false))
        assertTrue(controller.snapTo(1, false))
        assertEquals(1, controller.currentIndex)
    }

    @Test
    fun fitMeasurementsCanChangeAfterAnEmptyFirstLayout() = runSheet { controller, _ ->
        controller.apply(
            SheetConfiguration(isOpen = true, detents = listOf(-1.0), animated = false)
        )
        controller.onContentHeightChanged(0)
        shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1))
        controller.onContentHeightChanged(240)
        shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1))
        assertTrue(controller.isOpen)
        controller.onContentHeightChanged(80)
        shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1))
        assertTrue(controller.isOpen)
    }

    @Test
    fun disposingTwicePreventsFuturePresentation() = runSheet { controller, events ->
        controller.dispose()
        controller.dispose()
        controller.apply(SheetConfiguration(isOpen = true))
        assertFalse(controller.isOpen)
        assertTrue(events.isEmpty())
    }
}
