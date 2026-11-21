// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.sheet.lynx

import android.content.Context
import android.graphics.Color
import android.view.View
import android.view.ViewGroup
import com.lynx.react.bridge.Callback
import com.lynx.react.bridge.Dynamic
import com.lynx.react.bridge.ReadableMap
import com.lynx.react.bridge.ReadableType
import com.lynx.serval.sheet.SheetColors
import com.lynx.serval.sheet.SheetConfiguration
import com.lynx.serval.sheet.SheetDismissChannel
import com.lynx.serval.sheet.SheetSession
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.LynxUIMethod
import com.lynx.tasm.behavior.ui.LynxBaseUI
import com.lynx.tasm.behavior.ui.LynxUI
import com.lynx.tasm.behavior.ui.UIGroup
import com.lynx.tasm.behavior.ui.view.AndroidView
import com.lynx.tasm.event.LynxCustomEvent

/** Consumer modules register this implementation under their element name. */
public open class SheetElement(context: LynxContext) : UIGroup<AndroidView>(context) {
    private lateinit var content: SheetContentView
    private var session: SheetSession? = null
    private var configuration =
        SheetConfiguration(detents = listOf(1.0), nativeContainerBackgroundColor = Color.WHITE)
    private var initialOpen = false
    private var initialIndex = 0
    private var initialized = false
    private var fit = false
    private var dismissible = true
    private var clickToClose = true
    private val errors = mutableMapOf<String, String>()

    init {
        setUserInteractionEnabled(false)
    }

    override fun createView(context: Context?): AndroidView {
        val runtime = context as LynxContext
        content =
            SheetContentView(
                runtime,
                { width, height ->
                    val shadow = runtime.findShadowNodeBySign(sign) as? SheetContentShadowNode
                    shadow?.fitContent = fit
                    shadow?.updateState(0f, 0f, width.toFloat(), height.toFloat())
                    runtime.lynxView?.post {
                        val root = runtime.lynxView ?: return@post
                        root.forceLayout()
                        root.measure(
                            View.MeasureSpec.makeMeasureSpec(root.width, View.MeasureSpec.EXACTLY),
                            View.MeasureSpec.makeMeasureSpec(root.height, View.MeasureSpec.EXACTLY),
                        )
                        root.layout(root.left, root.top, root.right, root.bottom)
                    }
                },
                { event ->
                    setUserInteractionEnabled(true)
                    try {
                        runtime.touchEventDispatcher.onTouchEvent(event, this)
                    } finally {
                        setUserInteractionEnabled(false)
                    }
                },
            )
        return object : AndroidView(runtime) {
            override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) =
                Unit

            override fun onAttachedToWindow() {
                super.onAttachedToWindow()
                reconcile()
            }

            override fun onDetachedFromWindow() {
                session?.dispose()
                session = null
                super.onDetachedFromWindow()
            }
        }
    }

    override fun insertView(child: LynxUI<*>?) {
        child ?: return
        (child.view.parent as? ViewGroup)?.removeView(child.view)
        content.addView(child.view, getIndex(child).coerceAtLeast(0))
    }

    override fun removeView(child: LynxBaseUI?) {
        if (child is LynxUI<*>) content.removeView(child.view)
    }

    override fun removeAll() {
        content.removeAllViews()
        super.removeAll()
    }

    private fun emit(name: String, detail: Map<String, Any>) {
        val event = LynxCustomEvent(sign, name)
        detail.forEach { (key, value) -> event.addDetail(key, value) }
        lynxContext.eventEmitter.sendCustomEvent(event)
    }

    private fun invalid(field: String, code: String) {
        errors[field] = code
        emit("error", mapOf("code" to code, "message" to "Invalid $field"))
    }

    @LynxProp(name = "initial-open")
    public fun initialOpen(value: Boolean) {
        if (!initialized) initialOpen = value
    }

    @LynxProp(name = "initial-snap-index")
    public fun initialSnapIndex(value: Dynamic?) {
        if (initialized) return
        if (value == null || value.type == ReadableType.Null) {
            initialIndex = 0
            errors.remove("initial-snap-index")
            return
        }
        val index = if (value.type == ReadableType.Number) value.asDouble() else Double.NaN
        if (!index.isFinite() || index < 0 || index > Int.MAX_VALUE || index % 1.0 != 0.0)
            return invalid("initial-snap-index", "invalid-snap-points")
        initialIndex = index.toInt()
        errors.remove("initial-snap-index")
    }

    @LynxProp(name = "snap-points")
    public fun snapPoints(value: Dynamic?) {
        if (value == null || value.type == ReadableType.Null) {
            fit = false
            configuration = configuration.copy(detents = listOf(1.0))
            errors.remove("snap-points")
            return
        }
        if (value.type == ReadableType.String && value.asString() == "fit") {
            fit = true
            configuration = configuration.copy(detents = listOf(-1.0))
            errors.remove("snap-points")
            return
        }
        if (value.type != ReadableType.Array) return invalid("snap-points", "invalid-snap-points")
        val array = value.asArray()
        val points = runCatching { List(array.size()) { array.getDouble(it) } }.getOrNull()
        if (
            points == null ||
                points.size !in 1..3 ||
                points.any { !it.isFinite() || it <= 0 || it > 1 } ||
                points != points.distinct().sorted()
        )
            return invalid("snap-points", "invalid-snap-points")
        errors.remove("snap-points")
        fit = false
        configuration = configuration.copy(detents = points)
    }

    @LynxProp(name = "dismissible", defaultBoolean = true)
    public fun dismissible(value: Boolean) {
        dismissible = value
    }

    @LynxProp(name = "animation")
    public fun animation(value: String?) {
        if (value != null && value !in listOf("default", "platform", "none"))
            return invalid("animation", "unsupported-capability")
        errors.remove("animation")
        configuration = configuration.copy(animated = value != "none")
    }

    @LynxProp(name = "presentation-host")
    public fun presentationHost(value: String?) {
        if (value != null && value !in listOf("page", "top"))
            return invalid("presentation-host", "unavailable-host")
        errors.remove("presentation-host")
    }

    @LynxProp(name = "appearance")
    public fun appearance(value: ReadableMap?) {
        val background =
            if (value?.hasKey("backgroundColor") == true)
                SheetColors.decode(value.getString("backgroundColor"))
                    ?: return invalid("appearance", "invalid-state")
            else Color.WHITE
        val radius =
            if (value?.hasKey("cornerRadius") == true) value.getDouble("cornerRadius") else -1.0
        if (!radius.isFinite() || (value?.hasKey("cornerRadius") == true && radius < 0))
            return invalid("appearance", "invalid-state")
        errors.remove("appearance")
        configuration =
            configuration.copy(
                nativeContainerBackgroundColor = background,
                preferredCornerRadius =
                    if (radius >= 0) radius.toFloat() * lynxContext.resources.displayMetrics.density
                    else -1f,
                prefersGrabberVisible =
                    value?.hasKey("grabberVisible") == true && value.getBoolean("grabberVisible"),
            )
    }

    @LynxProp(name = "backdrop")
    public fun backdrop(value: Dynamic?) {
        if (
            value == null ||
                value.type == ReadableType.Null ||
                (value.type == ReadableType.String &&
                    value.asString() in listOf("default", "platform"))
        ) {
            configuration = configuration.copy(backdropColor = Color.BLACK, backdropOpacity = 0.5f)
            clickToClose = true
            errors.remove("backdrop")
            return
        }
        if (value.type != ReadableType.Map) return invalid("backdrop", "unsupported-capability")
        val map = value.asMap()
        val opacity = if (map.hasKey("opacity")) map.getDouble("opacity").toFloat() else 0.5f
        if (!opacity.isFinite() || opacity !in 0f..1f) return invalid("backdrop", "invalid-state")
        val color =
            if (map.hasKey("color"))
                SheetColors.decode(map.getString("color"))
                    ?: return invalid("backdrop", "invalid-state")
            else Color.BLACK
        configuration = configuration.copy(backdropOpacity = opacity, backdropColor = color)
        clickToClose = !map.hasKey("clickToClose") || map.getBoolean("clickToClose")
        errors.remove("backdrop")
    }

    override fun onPropsUpdated() {
        super.onPropsUpdated()
        reconcile()
    }

    override fun onLayoutUpdated() {
        super.onLayoutUpdated()
        if (fit) {
            val height =
                (0 until childCount).maxOfOrNull { index ->
                    val child = getChildAt(index)
                    child.top + child.height
                } ?: 0
            session?.contentHeightChanged(height)
        }
    }

    private fun reconcile() {
        if (errors.isNotEmpty() || !::content.isInitialized) return
        configuration =
            configuration.copy(
                preventNativeDismissChannels =
                    if (!dismissible) SheetDismissChannel.values().toSet()
                    else if (!clickToClose) setOf(SheetDismissChannel.BACKDROP) else emptySet()
            )
        val state = lynxContext.findShadowNodeBySign(sign) as? SheetContentShadowNode
        state?.fitContent = fit
        if (session == null) session = SheetSession(lynxContext, content, ::emit)
        session?.configure(configuration)
        if (!initialized && view.isAttachedToWindow) {
            initialized = true
            if (initialOpen) session?.present(initialIndex)
        }
    }

    private fun invoke(kind: String, params: ReadableMap?, callback: Callback) {
        reconcile()
        val current = session
        if (errors.isNotEmpty() || current == null || !view.isAttachedToWindow) {
            callback.invoke(4, mapOf("code" to "invalid-state"))
            return
        }
        val parsed =
            runCatching {
                    if (kind == "snap") require(params?.hasKey("index") == true)
                    val number =
                        if (params?.hasKey("index") == true) params.getDouble("index")
                        else (if (current.isOpen) current.currentIndex else initialIndex).toDouble()
                    require(
                        number.isFinite() &&
                            number >= 0 &&
                            number <= Int.MAX_VALUE &&
                            number % 1.0 == 0.0
                    )
                    val request =
                        if (params?.hasKey("requestId") == true) params.getString("requestId")
                        else null
                    val animate =
                        if (params?.hasKey("animate") == true) params.getBoolean("animate")
                        else configuration.animated
                    Triple(number.toInt(), request, animate)
                }
                .getOrElse {
                    callback.invoke(4, mapOf("accepted" to false, "code" to "invalid-state"))
                    return
                }
        val (index, request, animate) = parsed
        val result =
            when (kind) {
                "present" -> current.present(index, request, animate)
                "dismiss" -> current.dismiss(request, animate)
                else -> current.snapTo(index, request, animate)
            }
        callback.invoke(if (result["accepted"] == true) 0 else 4, result)
    }

    @LynxUIMethod
    public fun present(params: ReadableMap?, callback: Callback) =
        invoke("present", params, callback)

    @LynxUIMethod
    public fun dismiss(params: ReadableMap?, callback: Callback) =
        invoke("dismiss", params, callback)

    @LynxUIMethod
    public fun snapTo(params: ReadableMap?, callback: Callback) = invoke("snap", params, callback)

    override fun destroy() {
        session?.dispose()
        session = null
        super.destroy()
    }
}
