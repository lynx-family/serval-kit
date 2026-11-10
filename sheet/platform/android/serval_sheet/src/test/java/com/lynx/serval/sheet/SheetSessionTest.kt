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
@Config(sdk = [33])
class SheetSessionTest {
    private fun runSession(
        test: (SheetSession, MutableList<Pair<String, Map<String, Any>>>) -> Unit
    ) {
        val host = Robolectric.buildActivity(Activity::class.java).setup()
        val activity = host.get()
        activity.setTheme(
            com.google.android.material.R.style.Theme_MaterialComponents_Light_NoActionBar
        )
        val events = mutableListOf<Pair<String, Map<String, Any>>>()
        val session =
            SheetSession(activity, View(activity), { name, detail -> events += name to detail })
        session.configure(SheetConfiguration(detents = listOf(0.3, 0.6, 0.9), animated = false))
        try {
            test(session, events)
        } finally {
            session.dispose()
            host.pause().stop().destroy()
        }
    }

    private fun settle() = shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(2))

    @Test
    fun presentSnapDismissProduceCorrelatedTerminalResults() = runSession { session, events ->
        assertEquals(true, session.present(1, "open")["accepted"])
        settle()
        assertTrue(session.isOpen)
        assertEquals(1, session.currentIndex)
        assertEquals(
            listOf(1),
            events.filter { it.first == "snapchange" }.map { it.second["index"] },
        )
        session.snapTo(2, "expand", false)
        settle()
        session.dismiss("close", false)
        settle()
        assertFalse(session.isOpen)
        val completed = events.filter { it.first == "transitionend" }.map { it.second }
        assertEquals(listOf("open", "expand", "close"), completed.map { it["requestId"] })
        assertTrue(completed.all { it["result"] == "completed" })
    }

    @Test
    fun pendingRequestIsCancelledWhenSuperseded() = runSession { session, events ->
        session.present(0, "open", true)
        session.snapTo(2, "superseded", true)
        session.dismiss("close", false)
        settle()
        assertFalse(session.isOpen)
        val terminal = events.filter { it.first == "transitionend" }.map { it.second }
        assertEquals(3, terminal.size)
        assertEquals("cancelled", terminal.single { it["requestId"] == "superseded" }["result"])
        assertEquals("completed", terminal.single { it["requestId"] == "close" }["result"])
    }

    @Test
    fun duplicateCommandsDoNotRepeatOpenNotifications() = runSession { session, events ->
        session.present(0, "first", false)
        settle()
        assertEquals(true, session.present(0, "repeat")["noop"])
        assertEquals(true, session.snapTo(0, "repeat-snap")["noop"])
        assertEquals(1, events.count { it.first == "openchange" })
        assertEquals(1, events.count { it.first == "transitionend" })
    }

    @Test
    fun repeatingActiveOpenWithdrawsQueuedClose() = runSession { session, events ->
        session.present(1, "open", true)
        session.dismiss("withdrawn-close", false)
        assertEquals(true, session.present(1, "keep-open")["coalesced"])
        settle()
        assertTrue(session.isOpen)
        val ends = events.filter { it.first == "transitionend" }.map { it.second }
        assertEquals(2, ends.size)
        assertEquals("cancelled", ends.single { it["requestId"] == "withdrawn-close" }["result"])
    }

    @Test
    fun disposedSessionRejectsLaterCommands() = runSession { session, _ ->
        session.present(0, "open", true)
        session.snapTo(2, "queued", true)
        session.dispose()
        session.dispose()
        assertEquals(false, session.present(0)["accepted"])
    }
}
