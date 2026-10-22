// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.sheet

import android.content.Context
import android.view.View

/** Command protocol shared by native element bindings. Access on the UI thread. */
public class SheetSession(
    context: Context,
    content: View,
    private val emit: (String, Map<String, Any>) -> Unit,
    host: SheetHost = object : SheetHost {},
) {
    private var configuration = SheetConfiguration()
    private var generation = 0L
    private var active: Map<String, Any>? = null
    private var pending: Map<String, Any>? = null
    private var disposed = false
    private var disposing = false
    private var lastSnap = -1
    private var configurationPending = false
    private var resizingPresentation = false
    private val controller: SheetController =
        SheetController(
            context,
            content,
            object : SheetListener {
                override fun emitOnWillAppear() = lifecycle("willAppear")

                override fun emitOnDidAppear() {
                    lifecycle("didAppear")
                    presentationFinished()
                }

                override fun emitOnWillDisappear() = lifecycle("willDisappear")

                override fun emitOnDidDisappear() {
                    lifecycle("didDisappear")
                    finish("dismiss")
                }

                override fun emitOnDismissEvent() = Unit

                override fun onNativeDismissStarted(channel: SheetDismissChannel) {
                    if (active?.get("operation") != "dismiss") {
                        val old = active
                        active = null
                        if (old != null) emit("transitionend", old + ("result" to "cancelled"))
                        active = operation("dismiss", 0, null, channel.eventValue)
                        emit("transitionstart", active!!)
                        emit("openchange", active!! + ("open" to false))
                    }
                }

                override fun emitOnNativeDismissEvent() {
                    nativeDismissFinished()
                }

                override fun emitOnNativeDismissPreventedEvent(channel: SheetDismissChannel) =
                    emit("dismissprevented", mapOf("channel" to channel.eventValue))

                override fun emitOnDetentChanged(index: Int) =
                    emit("targetchange", mapOf("index" to index))

                override fun onSnapSettled(index: Int, height: Int) {
                    if (active?.get("operation") == "present" && !resizingPresentation) return
                    if (lastSnap != index) {
                        lastSnap = index
                        val fraction = configuration.detents.getOrNull(index)?.takeIf { it >= 0 }
                        emit(
                            "snapchange",
                            mapOf("index" to index, "heightPx" to height) +
                                (fraction?.let { mapOf("fraction" to it) } ?: emptyMap()),
                        )
                    }
                    finish(if (resizingPresentation) "present" else "snap")
                }
            },
            host,
        )

    public val isOpen: Boolean
        get() = controller.isOpen

    public val currentIndex: Int
        get() = controller.currentIndex

    private fun presentationFinished() {
        if (lastSnap != controller.currentIndex) {
            lastSnap = controller.currentIndex
            val fraction = configuration.detents.getOrNull(lastSnap)?.takeIf { it >= 0 }
            emit(
                "snapchange",
                mapOf("index" to lastSnap, "heightPx" to controller.currentHeight) +
                    (fraction?.let { mapOf("fraction" to it) } ?: emptyMap()),
            )
        }
        finish("present")
    }

    private fun nativeDismissFinished() {
        configuration = configuration.copy(isOpen = false)
        controller.apply(configuration)
    }

    public fun configure(value: SheetConfiguration) {
        if (disposed) return
        configuration = value.copy(isOpen = configuration.isOpen)
        if (active == null) controller.apply(configuration) else configurationPending = true
    }

    public fun present(
        index: Int,
        requestId: String? = null,
        animate: Boolean = configuration.animated,
    ): Map<String, Any> = submit("present", index, requestId, animate)

    public fun dismiss(
        requestId: String? = null,
        animate: Boolean = configuration.animated,
    ): Map<String, Any> = submit("dismiss", 0, requestId, animate)

    public fun snapTo(
        index: Int,
        requestId: String? = null,
        animate: Boolean = configuration.animated,
    ): Map<String, Any> = submit("snap", index, requestId, animate)

    private fun operation(kind: String, index: Int, requestId: String?, reason: String) =
        mapOf<String, Any>(
            "operation" to kind,
            "index" to index,
            "transitionId" to ++generation,
            "reason" to reason,
        ) + (requestId?.let { mapOf("requestId" to it) } ?: emptyMap())

    private fun submit(
        kind: String,
        index: Int,
        requestId: String?,
        animate: Boolean,
    ): Map<String, Any> {
        if (disposed || disposing || (kind == "snap" && !isOpen))
            return mapOf("accepted" to false, "code" to "invalid-state")
        if (kind != "dismiss" && index !in 0 until configuration.detents.size.coerceAtLeast(1))
            return mapOf("accepted" to false, "code" to "invalid-snap-points")
        val current = active
        if (current?.get("operation") == kind && current["index"] == index) {
            val superseded = pending
            pending = null
            superseded?.let { emit("transitionend", it + ("result" to "cancelled")) }
            return mapOf(
                "accepted" to true,
                "coalesced" to true,
                "transitionId" to current.getValue("transitionId"),
            )
        }
        if (
            active == null &&
                ((kind == "dismiss" && !isOpen) ||
                    (kind != "dismiss" && isOpen && currentIndex == index))
        )
            return mapOf("accepted" to true, "noop" to true)
        val op = operation(kind, index, requestId, "command") + ("animate" to animate)
        val superseded = pending
        pending = op
        superseded?.let { emit("transitionend", it + ("result" to "cancelled")) }
        drain()
        return mapOf("accepted" to true, "transitionId" to op.getValue("transitionId"))
    }

    private fun drain() {
        if (disposed || active != null) return
        val op = pending ?: return
        pending = null
        active = op
        val kind = op["operation"]
        val index = op["index"] as Int
        val animate = op["animate"] as Boolean
        emit("transitionstart", op)
        if (disposed) return
        if (kind == "snap" || (kind == "present" && isOpen)) {
            resizingPresentation = kind == "present"
            if (controller.currentIndex == index) {
                finish(kind as String)
                return
            }
            if (!controller.snapTo(index, animate)) cancelActive()
        } else {
            val open = kind == "present"
            configuration =
                configuration.copy(isOpen = open, initialDetentIndex = index, animated = animate)
            emit("openchange", op + ("open" to open))
            if (!disposed) controller.apply(configuration)
        }
    }

    private fun lifecycle(phase: String) {
        if (!disposed) emit("lifecycle", mapOf("phase" to phase))
    }

    private fun finish(kind: String) {
        val op = active ?: return
        if (op["operation"] != kind) return
        active = null
        resizingPresentation = false
        emit("transitionend", op + ("result" to "completed"))
        if (configurationPending) {
            configurationPending = false
            controller.apply(configuration)
        }
        drain()
    }

    private fun cancelActive() {
        val op = active ?: return
        active = null
        emit("transitionend", op + ("result" to "cancelled"))
        drain()
    }

    public fun contentHeightChanged(height: Int) = controller.onContentHeightChanged(height)

    public fun dispose() {
        if (disposed || disposing) return
        disposing = true
        pending?.let { emit("transitionend", it + ("result" to "cancelled")) }
        pending = null
        active?.let { emit("transitionend", it + ("result" to "cancelled")) }
        active = null
        disposed = true
        controller.dispose()
    }
}
