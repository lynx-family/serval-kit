// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Portions Copyright (c) 2026 Software Mansion. See sheet/LICENSE.screens.
package com.lynx.serval.sheet

import android.animation.Animator
import android.animation.AnimatorListenerAdapter
import android.animation.ValueAnimator
import android.annotation.SuppressLint
import android.content.Context
import android.content.res.ColorStateList
import android.graphics.Color
import android.graphics.Rect
import android.graphics.drawable.Drawable
import android.graphics.drawable.GradientDrawable
import android.os.Build
import android.view.GestureDetector
import android.view.Gravity
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.View
import android.view.ViewConfiguration
import android.view.ViewGroup
import android.view.ViewGroup.LayoutParams
import android.view.Window
import android.view.WindowManager
import android.view.animation.AccelerateDecelerateInterpolator
import android.widget.FrameLayout
import androidx.coordinatorlayout.widget.CoordinatorLayout
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import com.google.android.material.bottomsheet.BottomSheetDialog
import kotlin.math.abs
import kotlin.math.roundToInt

public class SheetController(
    private val context: Context,
    private val contentView: View,
    private val eventEmitter: SheetListener,
    private val host: SheetHost = object : SheetHost {},
    private val navigationTrace: SheetTrace? = null,
) {
    public val isOpen: Boolean
        get() = isPresented && !nativeDismissedForCurrentRequest

    public val currentIndex: Int
        get() = lastEmittedDetentIndex.coerceAtLeast(0)

    public val currentHeight: Int
        get() = currentBottomSheet()?.let { visibleSheetHeight(it, it.height) } ?: 0

    private val resources
        get() = context.resources

    private var detents = SheetDetents.parse(emptyList())
    private var contentHeight: Int? = null
    private var maximumContentHeight = Int.MAX_VALUE
    private var disposed = false
    private val sheetContainer =
        object : FrameLayout(context) {
            override fun dispatchTouchEvent(event: MotionEvent): Boolean {
                trackPotentialDismissDrag(event)
                // The supplied content view owns Lynx touch forwarding.
                return super.dispatchTouchEvent(event)
            }

            override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
                val limit =
                    if (MeasureSpec.getMode(heightMeasureSpec) == MeasureSpec.UNSPECIFIED) {
                            maximumContentHeight
                        } else {
                            minOf(MeasureSpec.getSize(heightMeasureSpec), maximumContentHeight)
                        }
                        .coerceAtLeast(1)
                if (!detents.isFitToContents) {
                    super.onMeasure(widthMeasureSpec, heightMeasureSpec)
                    return
                }
                super.onMeasure(
                    widthMeasureSpec,
                    MeasureSpec.makeMeasureSpec(limit, MeasureSpec.AT_MOST),
                )
                // The public content host fills its constraints; its children (or the
                // content-size delegate) supply the intrinsic height instead.
                val intrinsicHeight =
                    contentHeight
                        ?: (contentView as? ViewGroup)?.let { content ->
                            (0 until content.childCount).maxOfOrNull {
                                content.getChildAt(it).measuredHeight
                            }
                        }
                        ?: contentView.measuredHeight
                val height = intrinsicHeight.coerceIn(1, limit)
                super.onMeasure(
                    widthMeasureSpec,
                    MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY),
                )
            }
        }
    private val grabber = View(context)
    private var dialog: BottomSheetDialog? = null
    private var sheetBehavior: SheetBehavior<FrameLayout>? = null
    private var latestConfig: SheetConfiguration? = null
    private var isPresented = false
    private var nativeDismissedForCurrentRequest = false
    private var previousRequestedOpen = false
    private var lastEmittedDetentIndex = -1
    private var currentDetentsCount = 1
    private var hasEmittedWillDisappear = false
    private var defaultBottomSheetBackground: Drawable? = null
    private var stateBeforeDrag = -1
    private var maxOffsetDuringDrag = Float.MIN_VALUE
    private var pendingDismissPreventedEmission = false
    private var recoveringPreventedDragDismiss = false
    private var dismissDragStartRawY = Float.NaN
    private var maxDownwardDismissDragDistance = 0f
    private val dismissDragDistanceThreshold =
        ViewConfiguration.get(context).scaledTouchSlop.toFloat()
    private var currentDetentHeights: List<Int> = emptyList()
    private var presentationAnimationPending = false
    private var presentationAnimationRunning = false
    private var dismissAnimationRunning = false
    private var pendingBackdropAlpha = 0f
    private var presentationTargetDetentIndex: Int? = null
    private var hasEmittedDidAppear = false
    private var nativeDismissEventEmitted = false
    private var programmaticDismissPending = false
    private var dismissAnimator: ValueAnimator? = null
    private var presentationAnimationStartedAt = 0L
    private var currentSheetArea: SheetArea? = null
    private var formSheetAreaObservation: AutoCloseable? = null
    private var observedDialogDecorView: View? = null
    private val dialogLayoutChangeListener =
        View.OnLayoutChangeListener {
            _,
            left,
            top,
            right,
            bottom,
            oldLeft,
            oldTop,
            oldRight,
            oldBottom ->
            if (right - left == oldRight - oldLeft && bottom - top == oldBottom - oldTop) {
                return@OnLayoutChangeListener
            }
            if (!disposed && isPresented && !dismissAnimationRunning) {
                host.refreshArea(right - left, bottom - top, currentSheetArea)
                latestConfig?.let { configurePresentedSheet(it, applyInitialDetent = false) }
            }
        }

    private val bottomSheetCallback =
        object : SheetBehavior.Callback() {
            override fun onStateChanged(bottomSheet: View, newState: Int) {
                if (disposed || bottomSheet !== currentBottomSheet()) return
                if (newState == SheetBehavior.STATE_HIDDEN) {
                    dialog?.cancel()
                    return
                }
                when (newState) {
                    SheetBehavior.STATE_DRAGGING -> {
                        stateBeforeDrag = lastEmittedDetentIndex
                        maxOffsetDuringDrag = Float.MIN_VALUE
                        pendingDismissPreventedEmission = false
                    }
                    SheetBehavior.STATE_SETTLING -> Unit
                    else -> {
                        val index = stateToDetentIndex(newState, currentDetentsCount)
                        if (index >= 0 && index != lastEmittedDetentIndex) {
                            lastEmittedDetentIndex = index
                            eventEmitter.emitOnDetentChanged(index)
                            eventEmitter.onSnapSettled(
                                index,
                                currentDetentHeights.getOrElse(index) { bottomSheet.height },
                            )
                        }
                        maybeMarkDismissAttempt(index)
                        if (!disposed && index >= 0 && pendingDismissPreventedEmission) {
                            pendingDismissPreventedEmission = false
                            navigationTrace?.ensureDismiss(SheetDismissChannel.DRAG)
                            navigationTrace?.nativeDismissPrevented(SheetDismissChannel.DRAG)
                            eventEmitter.emitOnNativeDismissPreventedEvent(SheetDismissChannel.DRAG)
                        }
                        if (
                            index >= 0 &&
                                navigationTrace?.isActiveDismissFrom(SheetDismissChannel.DRAG) ==
                                    true
                        ) {
                            navigationTrace?.cancel("drag_settled_without_dismiss")
                        }
                        if (index == 0) recoveringPreventedDragDismiss = false
                    }
                }
                updateSheetSurfaceLayout(bottomSheet)
                updateDimming(bottomSheet)
            }

            override fun onSlide(bottomSheet: View, slideOffset: Float) {
                if (disposed || bottomSheet !== currentBottomSheet()) return
                val config = latestConfig
                if (
                    config?.preventsNativeDismiss(SheetDismissChannel.DRAG) == true &&
                        config.preventNativeDismissDragFeedback
                ) {
                    maxOffsetDuringDrag = maxOf(maxOffsetDuringDrag, slideOffset)
                }
                updateSheetSurfaceLayout(bottomSheet)
                updateDimming(bottomSheet)
            }
        }

    init {
        (contentView.parent as? ViewGroup)?.removeView(contentView)
        sheetContainer.addView(
            contentView,
            FrameLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT),
        )
        val grabberHeight = dp(4)
        grabber.background =
            GradientDrawable().apply {
                shape = GradientDrawable.RECTANGLE
                cornerRadius = grabberHeight / 2f
                setColor(Color.argb(90, 80, 80, 80))
            }
        sheetContainer.addView(
            grabber,
            FrameLayout.LayoutParams(
                    dp(32),
                    grabberHeight,
                    Gravity.TOP or Gravity.CENTER_HORIZONTAL,
                )
                .apply { topMargin = dp(8) },
        )
        formSheetAreaObservation =
            host.observeAreaChanges() {
                if (!disposed && isPresented && !dismissAnimationRunning) {
                    latestConfig?.let { configurePresentedSheet(it, applyInitialDetent = false) }
                }
            }
    }

    public fun apply(config: SheetConfiguration) {
        if (disposed) return
        latestConfig = config
        detents = SheetDetents.parse(config.detents)
        val requestedOpen = !previousRequestedOpen && config.isOpen
        previousRequestedOpen = config.isOpen
        if (requestedOpen || !config.isOpen) nativeDismissedForCurrentRequest = false
        when {
            dismissAnimationRunning -> {
                if (requestedOpen && isPresented && dialog?.isShowing == true)
                    cancelDismissAndPresent(config)
            }
            config.isOpen && !isPresented && dialog == null && !nativeDismissedForCurrentRequest ->
                present(config)
            !config.isOpen && isPresented ->
                dialog?.let {
                    navigationTrace?.beginProgrammaticDismiss()
                    dismissWithAnimation(it, isNativeDismiss = false)
                }
            config.isOpen && isPresented ->
                configurePresentedSheet(config, applyInitialDetent = false)
        }
    }

    public fun snapTo(index: Int, animate: Boolean = true): Boolean {
        if (
            disposed ||
                !isPresented ||
                dismissAnimationRunning ||
                index !in 0 until currentDetentsCount
        )
            return false
        val behavior = sheetBehavior ?: return false
        behavior.moveToState(stateForIndex(index, currentDetentsCount), animate)
        return true
    }

    public fun onContentHeightChanged(height: Int) {
        if (disposed || height < 0 || contentHeight == height) return
        contentHeight = height
        if (detents.isFitToContents && isPresented && !dismissAnimationRunning) {
            latestConfig?.let { configurePresentedSheet(it, applyInitialDetent = false) }
        }
    }

    public fun dispose() {
        if (disposed) return
        disposed = true
        navigationTrace?.dispose("controller_disposed")
        formSheetAreaObservation?.close()
        formSheetAreaObservation = null
        val sourceDialog = dialog
        cancelAnimations(sourceDialog)
        stopObservingDialogLayout()
        sheetBehavior?.setCallback(INACTIVE_CALLBACK)
        sourceDialog?.setOnCancelListener(null)
        sourceDialog?.setOnDismissListener(null)
        sourceDialog?.setOnKeyListener(null)
        dialog = null
        sheetBehavior = null
        isPresented = false
        latestConfig = null
        currentSheetArea = null
        sourceDialog?.dismiss()
        (sheetContainer.parent as? ViewGroup)?.removeView(sheetContainer)
        (contentView.parent as? ViewGroup)?.removeView(contentView)
    }

    private fun present(config: SheetConfiguration) {
        navigationTrace?.beginPresent()
        val newDialog =
            object : BottomSheetDialog(context) {
                override fun cancel() {
                    if (disposed || dialog !== this) return
                    val behavior = sheetBehavior
                    if (
                        isPresented &&
                            behavior?.state == SheetBehavior.STATE_HIDDEN &&
                            latestConfig?.preventsNativeDismiss(SheetDismissChannel.DRAG) == true
                    ) {
                        recoverPreventedDragDismiss(behavior)
                        return
                    }
                    super.cancel()
                }
            }
        dialog = newDialog
        hasEmittedWillDisappear = false
        lastEmittedDetentIndex = -1
        defaultBottomSheetBackground = null
        recoveringPreventedDragDismiss = false
        stateBeforeDrag = -1
        pendingDismissPreventedEmission = false
        presentationAnimationPending = true
        presentationAnimationRunning = false
        dismissAnimationRunning = false
        pendingBackdropAlpha = 0f
        presentationTargetDetentIndex = null
        hasEmittedDidAppear = false
        nativeDismissEventEmitted = false
        programmaticDismissPending = false
        dismissAnimator = null

        (sheetContainer.parent as? ViewGroup)?.removeView(sheetContainer)
        newDialog.setContentView(sheetContainer)
        val bottomSheet =
            newDialog.findViewById<FrameLayout>(
                com.google.android.material.R.id.design_bottom_sheet
            )
                ?: run {
                    navigationTrace?.cancel("bottom_sheet_missing")
                    dialog = null
                    return
                }
        val layoutParams = bottomSheet.layoutParams as CoordinatorLayout.LayoutParams
        sheetBehavior = SheetBehavior<FrameLayout>().also { layoutParams.behavior = it }
        bottomSheet.layoutParams = layoutParams
        // Material couples outside taps, back and drag-to-hide. Route each channel explicitly.
        newDialog.setCancelable(false)
        newDialog.setCanceledOnTouchOutside(false)
        newDialog.window?.let(::configureDialogWindow)
        installBackdrop(newDialog)
        installBottomSheetOutsideSurfaceHandler(newDialog, bottomSheet)
        bottomSheet.translationY =
            if (config.animated) resources.displayMetrics.heightPixels.toFloat() else 0f
        newDialog.setOnCancelListener {
            if (!disposed && dialog === newDialog) {
                navigationTrace?.ensureDismiss(SheetDismissChannel.DRAG)
                installDismissTrace(newDialog, navigationTrace?.operation)
                navigationTrace?.start(isAnimated = true)
                nativeDismissedForCurrentRequest = true
                eventEmitter.onNativeDismissStarted(SheetDismissChannel.DRAG)
                emitNativeDismissEventIfNeeded(true)
                emitOnWillDisappearIfNeeded()
            }
        }
        installDismissTrace(newDialog, null)
        newDialog.setOnKeyListener { _, keyCode, keyEvent ->
            if (keyCode != KeyEvent.KEYCODE_BACK) {
                false
            } else {
                if (keyEvent.action == KeyEvent.ACTION_UP && !keyEvent.isCanceled) {
                    requestNativeDismiss(SheetDismissChannel.BACK, newDialog)
                }
                true
            }
        }

        eventEmitter.emitOnWillAppear()
        if (disposed || dialog !== newDialog) return
        val currentConfig = latestConfig ?: config
        if (!currentConfig.isOpen) {
            navigationTrace?.cancel("present_request_withdrawn")
            dialog = null
            sheetBehavior = null
            (sheetContainer.parent as? ViewGroup)?.removeView(sheetContainer)
            return
        }
        configurePresentedSheet(currentConfig, applyInitialDetent = true)
        if (disposed || dialog !== newDialog) return
        isPresented = true
        newDialog.show()
        newDialog.window?.let(::configureDialogWindow)
        observeDialogLayout(newDialog)
        installWindowInsetsListener(newDialog)
        newDialog.window?.decorView?.post {
            if (
                !disposed &&
                    dialog === newDialog &&
                    isPresented &&
                    latestConfig?.isOpen == true &&
                    !dismissAnimationRunning
            ) {
                latestConfig?.let { configurePresentedSheet(it, applyInitialDetent = false) }
                startPresentationAnimation(newDialog)
            }
        }
    }

    private fun configurePresentedSheet(config: SheetConfiguration, applyInitialDetent: Boolean) {
        if (disposed) return
        val currentDialog = dialog ?: return
        val bottomSheet = currentBottomSheet() ?: return
        val parent = bottomSheet.parent as? View
        val parentWidth =
            parent?.width?.takeIf { it > 0 }
                ?: currentDialog.window?.decorView?.width?.takeIf { it > 0 }
                ?: resources.displayMetrics.widthPixels
        val parentHeight =
            parent?.height?.takeIf { it > 0 }
                ?: currentDialog.window?.decorView?.height?.takeIf { it > 0 }
                ?: resources.displayMetrics.heightPixels
        val formSheetArea = host.resolveArea(parentWidth, parentHeight, currentSheetArea)
        currentSheetArea = formSheetArea
        maximumContentHeight = (formSheetArea?.height ?: parentHeight).coerceAtLeast(1)
        val geometry = detents.resolveGeometry(windowHeight = maximumContentHeight)
        val sheetDetentHeights = geometry.sheetHeights
        currentDetentHeights = sheetDetentHeights
        grabber.visibility = if (config.prefersGrabberVisible) View.VISIBLE else View.GONE
        applyAppearance(bottomSheet, config, parentWidth, parentHeight)
        val behavior = sheetBehavior ?: return
        val layoutParams = bottomSheet.layoutParams as CoordinatorLayout.LayoutParams
        layoutParams.width = formSheetArea?.width ?: LayoutParams.MATCH_PARENT
        layoutParams.gravity =
            if (formSheetArea == null) Gravity.BOTTOM else Gravity.BOTTOM or Gravity.RIGHT
        layoutParams.leftMargin = 0
        layoutParams.topMargin = 0
        layoutParams.rightMargin = formSheetArea?.let { parentWidth - it.right } ?: 0
        layoutParams.bottomMargin = formSheetArea?.let { parentHeight - it.bottom } ?: 0
        bottomSheet.layoutParams = layoutParams
        behavior.setAvailableVerticalBounds(
            formSheetArea?.top ?: 0,
            formSheetArea?.bottom ?: parentHeight,
        )
        currentDialog.setCancelable(false)
        currentDialog.setCanceledOnTouchOutside(false)
        behavior.isHideable =
            config.gestureEnabled &&
                (!config.preventsNativeDismiss(SheetDismissChannel.DRAG) ||
                    !config.preventNativeDismissDragFeedback)
        behavior.setCallback(bottomSheetCallback)
        currentDetentsCount = detents.count

        if (detents.isFitToContents) {
            if (presentationAnimationPending || presentationAnimationRunning)
                presentationTargetDetentIndex = 0
            bottomSheet.layoutParams.height = LayoutParams.WRAP_CONTENT
            sheetContainer.layoutParams.height = LayoutParams.WRAP_CONTENT
            contentView.layoutParams.height = LayoutParams.MATCH_PARENT
            behavior.setDetents(emptyList(), fitToContents = true)
            behavior.state = SheetBehavior.STATE_EXPANDED
            lastEmittedDetentIndex = 0
            sheetContainer.requestLayout()
            bottomSheet.requestLayout()
            updateDimming(bottomSheet)
            requestSheetLayoutUpdate(bottomSheet)
            return
        }

        val largestSheetHeight = sheetDetentHeights.last().coerceAtLeast(1)
        bottomSheet.layoutParams.height = largestSheetHeight
        sheetContainer.layoutParams.height = largestSheetHeight
        contentView.layoutParams.height = LayoutParams.MATCH_PARENT
        behavior.setDetents(sheetDetentHeights, fitToContents = false)
        val requestedIndex = if (applyInitialDetent) config.initialDetentIndex else null
        val targetIndex =
            requestedIndex?.let {
                (if (it == -1) detents.count - 1 else it).coerceIn(0, detents.count - 1)
            }
        if ((presentationAnimationPending || presentationAnimationRunning) && targetIndex != null) {
            presentationTargetDetentIndex = targetIndex
        }
        if (detents.count == 1) {
            if (presentationAnimationPending || presentationAnimationRunning)
                presentationTargetDetentIndex = 0
            behavior.state = SheetBehavior.STATE_EXPANDED
        } else {
            requestedIndex?.let { behavior.state = stateForIndex(it, detents.count) }
        }
        val appliedIndex = stateToDetentIndex(behavior.state, detents.count)
        if (appliedIndex >= 0) lastEmittedDetentIndex = appliedIndex
        updateDimming(bottomSheet)
        sheetContainer.requestLayout()
        bottomSheet.requestLayout()
        requestSheetLayoutUpdate(bottomSheet)
    }

    private fun updateDimming(bottomSheet: View) {
        val currentDialog = dialog ?: return
        val backdrop =
            currentDialog.findViewById<View>(com.google.android.material.R.id.touch_outside)
                ?: return
        val window = currentDialog.window ?: return
        val visibleHeight =
            resolveSheetDimHeight(
                visibleSheetHeight = visibleSheetHeight(bottomSheet, window.decorView.height),
                detentHeights = currentDetentHeights,
                presentationTargetDetentIndex = presentationTargetDetentIndex,
            )
        val dimAmount =
            calculateSheetDimAmount(
                visibleSheetHeight = visibleHeight,
                detentHeights = currentDetentHeights,
                largestUndimmedDetentIndex = latestConfig?.largestUndimmedDetentIndex ?: -1,
                maximumDimAmount = latestConfig?.backdropOpacity ?: DEFAULT_DIM_AMOUNT,
            )
        if (presentationAnimationPending || presentationAnimationRunning) {
            val targetChanged = abs(pendingBackdropAlpha - dimAmount) > DIM_AMOUNT_EPSILON
            pendingBackdropAlpha = dimAmount
            if (presentationAnimationRunning && targetChanged) {
                val elapsed = android.os.SystemClock.uptimeMillis() - presentationAnimationStartedAt
                backdrop
                    .animate()
                    .alpha(dimAmount)
                    .setDuration((SHEET_ANIMATION_DURATION_MS - elapsed).coerceAtLeast(0L))
                    .setInterpolator(SHEET_ANIMATION_INTERPOLATOR)
                    .start()
            }
            return
        }
        if (!dismissAnimationRunning && abs(backdrop.alpha - dimAmount) > DIM_AMOUNT_EPSILON)
            backdrop.alpha = dimAmount
    }

    private fun applyAppearance(
        bottomSheet: FrameLayout,
        config: SheetConfiguration,
        parentWidth: Int,
        parentHeight: Int,
    ) {
        if (defaultBottomSheetBackground == null)
            defaultBottomSheetBackground = bottomSheet.background
        sheetContainer.setBackgroundColor(Color.TRANSPARENT)
        contentView.setBackgroundColor(config.nativeContainerBackgroundColor ?: Color.TRANSPARENT)
        bottomSheet.backgroundTintList = ColorStateList.valueOf(Color.TRANSPARENT)
        val density = resources.displayMetrics.density
        val isLargeScreen =
            parentWidth / density >= LARGE_SCREEN_MIN_DP &&
                parentHeight / density >= LARGE_SCREEN_MIN_DP
        val usesFloatingLargeScreenArea =
            isLargeScreen && currentSheetArea?.let { it.width < parentWidth } == true
        val cornerRadiusDp =
            when {
                config.preferredCornerRadius >= 0 -> config.preferredCornerRadius
                isLargeScreen -> DEFAULT_LARGE_SCREEN_CORNER_RADIUS_DP
                else -> null
            }
        if (cornerRadiusDp != null) {
            val radius = cornerRadiusDp * density
            bottomSheet.background =
                GradientDrawable().apply {
                    shape = GradientDrawable.RECTANGLE
                    cornerRadii =
                        if (usesFloatingLargeScreenArea) FloatArray(8) { radius }
                        else floatArrayOf(radius, radius, radius, radius, 0f, 0f, 0f, 0f)
                    setColor(Color.TRANSPARENT)
                }
            bottomSheet.clipToOutline = true
        } else {
            bottomSheet.background = defaultBottomSheetBackground
            bottomSheet.clipToOutline = false
        }
    }

    private fun stateForIndex(rawIndex: Int, count: Int): Int {
        val index = (if (rawIndex == -1) count - 1 else rawIndex).coerceIn(0, count - 1)
        return when (count) {
            2 -> if (index == 0) SheetBehavior.STATE_COLLAPSED else SheetBehavior.STATE_EXPANDED
            3 ->
                when (index) {
                    0 -> SheetBehavior.STATE_COLLAPSED
                    1 -> SheetBehavior.STATE_HALF_EXPANDED
                    else -> SheetBehavior.STATE_EXPANDED
                }
            else -> SheetBehavior.STATE_EXPANDED
        }
    }

    private fun stateToDetentIndex(state: Int, count: Int): Int =
        when (count) {
            1 ->
                if (state == SheetBehavior.STATE_EXPANDED || state == SheetBehavior.STATE_COLLAPSED)
                    0
                else -1
            2 ->
                when (state) {
                    SheetBehavior.STATE_COLLAPSED -> 0
                    SheetBehavior.STATE_EXPANDED -> 1
                    else -> -1
                }
            3 ->
                when (state) {
                    SheetBehavior.STATE_COLLAPSED -> 0
                    SheetBehavior.STATE_HALF_EXPANDED -> 1
                    SheetBehavior.STATE_EXPANDED -> 2
                    else -> -1
                }
            else -> -1
        }

    private fun emitOnWillDisappearIfNeeded() {
        if (!disposed && !hasEmittedWillDisappear) {
            hasEmittedWillDisappear = true
            eventEmitter.emitOnWillDisappear()
        }
    }

    // With hiding disabled, a downward drag returning to the lowest detent is
    // the dismiss-attempt signal, provided it did not first move upward.
    private fun maybeMarkDismissAttempt(settledIndex: Int) {
        val config = latestConfig
        if (
            config?.preventsNativeDismiss(SheetDismissChannel.DRAG) != true ||
                !config.preventNativeDismissDragFeedback ||
                pendingDismissPreventedEmission
        )
            return
        if (stateBeforeDrag != 0 || settledIndex != 0) return
        if (maxDownwardDismissDragDistance <= dismissDragDistanceThreshold) return
        val lowestOffset = if (currentDetentsCount == 1) 1f else 0f
        if (maxOffsetDuringDrag > lowestOffset + DISMISS_ATTEMPT_OFFSET_THRESHOLD) return
        pendingDismissPreventedEmission = true
    }

    private fun requestNativeDismiss(
        channel: SheetDismissChannel,
        sourceDialog: BottomSheetDialog,
    ) {
        if (disposed || dialog !== sourceDialog || !isPresented) return
        navigationTrace?.beginDismiss(channel)
        if (latestConfig?.preventsNativeDismiss(channel) == true) {
            navigationTrace?.nativeDismissPrevented(channel)
            eventEmitter.emitOnNativeDismissPreventedEvent(channel)
        } else {
            eventEmitter.onNativeDismissStarted(channel)
            dismissWithAnimation(sourceDialog, isNativeDismiss = true)
        }
    }

    @Suppress("DEPRECATION")
    private fun configureDialogWindow(window: Window) {
        WindowCompat.setDecorFitsSystemWindows(window, false)
        window.addFlags(WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN)
        window.clearFlags(WindowManager.LayoutParams.FLAG_DIM_BEHIND)
        window.statusBarColor = Color.TRANSPARENT
        window.navigationBarColor = Color.TRANSPARENT
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
            window.isNavigationBarContrastEnforced = false
        val attributes = window.attributes
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) attributes.setFitInsetsTypes(0)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            attributes.layoutInDisplayCutoutMode =
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                    WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
                } else {
                    WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
                }
        }
        attributes.windowAnimations = 0
        window.attributes = attributes
    }

    private fun startPresentationAnimation(
        sourceDialog: BottomSheetDialog,
        startOffscreen: Boolean = true,
    ) {
        if (disposed || dialog !== sourceDialog || !isPresented || dismissAnimationRunning) return
        if (latestConfig?.animated == false) {
            finishPresentationWithoutAnimation(sourceDialog)
            return
        }
        val traceOperation = navigationTrace?.operation
        val bottomSheet =
            sourceDialog.findViewById<View>(com.google.android.material.R.id.design_bottom_sheet)
                ?: return finishPresentationWithoutAnimation(sourceDialog)
        val backdrop =
            sourceDialog.findViewById<View>(com.google.android.material.R.id.touch_outside)
                ?: return finishPresentationWithoutAnimation(sourceDialog)
        val parentHeight =
            (bottomSheet.parent as? View)?.height?.takeIf { it > 0 }
                ?: sourceDialog.window?.decorView?.height?.takeIf { it > 0 }
                ?: resources.displayMetrics.heightPixels
        bottomSheet.animate().cancel()
        backdrop.animate().cancel()
        if (startOffscreen) {
            bottomSheet.translationY = (parentHeight - bottomSheet.top).coerceAtLeast(0).toFloat()
            backdrop.alpha = 0f
        }
        presentationAnimationPending = false
        presentationAnimationRunning = true
        presentationAnimationStartedAt = android.os.SystemClock.uptimeMillis()
        navigationTrace?.start(isAnimated = true)
        backdrop
            .animate()
            .alpha(pendingBackdropAlpha)
            .setDuration(SHEET_ANIMATION_DURATION_MS)
            .setInterpolator(SHEET_ANIMATION_INTERPOLATOR)
            .start()
        bottomSheet
            .animate()
            .translationY(0f)
            .setDuration(SHEET_ANIMATION_DURATION_MS)
            .setInterpolator(SHEET_ANIMATION_INTERPOLATOR)
            .withEndAction {
                if (
                    !disposed && dialog === sourceDialog && isPresented && !dismissAnimationRunning
                ) {
                    presentationAnimationRunning = false
                    presentationTargetDetentIndex = null
                    backdrop.animate().cancel()
                    backdrop.alpha = pendingBackdropAlpha
                    emitOnDidAppearIfNeeded(traceOperation)
                }
            }
            .start()
    }

    private fun finishPresentationWithoutAnimation(sourceDialog: BottomSheetDialog) {
        presentationAnimationPending = false
        presentationAnimationRunning = false
        presentationTargetDetentIndex = null
        if (!disposed && dialog === sourceDialog && isPresented && !dismissAnimationRunning) {
            // Restore final geometry before notifying JS; this path also handles interrupted
            // presentation.
            sourceDialog
                .findViewById<View>(com.google.android.material.R.id.design_bottom_sheet)
                ?.apply {
                    animate().withEndAction(null).cancel()
                    translationY = 0f
                }
            sourceDialog.findViewById<View>(com.google.android.material.R.id.touch_outside)?.apply {
                animate().cancel()
                alpha = pendingBackdropAlpha
            }
            navigationTrace?.start(isAnimated = false)
            emitOnDidAppearIfNeeded()
        }
    }

    private fun emitOnDidAppearIfNeeded(traceOperation: Any? = navigationTrace?.operation) {
        if (!disposed && !hasEmittedDidAppear && latestConfig?.isOpen == true) {
            hasEmittedDidAppear = true
            withTrace(traceOperation) { eventEmitter.emitOnDidAppear() }
            navigationTrace?.complete(traceOperation)
        }
    }

    private fun installDismissTrace(sourceDialog: BottomSheetDialog, traceOperation: Any?) {
        sourceDialog.setOnDismissListener {
            if (disposed || dialog !== sourceDialog || !isPresented) return@setOnDismissListener
            withTrace(traceOperation) { emitOnWillDisappearIfNeeded() }
            if (disposed || dialog !== sourceDialog) return@setOnDismissListener
            isPresented = false
            val emitProgrammaticDismiss = programmaticDismissPending
            programmaticDismissPending = false
            navigationTrace?.dismissCommittedAndComplete(traceOperation)
            cancelAnimations(sourceDialog)
            stopObservingDialogLayout()
            sheetBehavior?.setCallback(INACTIVE_CALLBACK)
            dialog = null
            sheetBehavior = null
            currentSheetArea = null
            presentationAnimationPending = false
            presentationAnimationRunning = false
            presentationTargetDetentIndex = null
            dismissAnimationRunning = false
            (sheetContainer.parent as? ViewGroup)?.removeView(sheetContainer)
            withTrace(traceOperation) { eventEmitter.emitOnDidDisappear() }
            if (emitProgrammaticDismiss && !disposed)
                withTrace(traceOperation) { eventEmitter.emitOnDismissEvent() }
            // Dialog posts OnDismiss: a newer open request must wait for the old window to detach.
            if (
                emitProgrammaticDismiss &&
                    !disposed &&
                    dialog == null &&
                    !nativeDismissedForCurrentRequest
            ) {
                latestConfig?.takeIf { it.isOpen }?.let(::present)
            }
        }
    }

    private fun dismissWithAnimation(sourceDialog: BottomSheetDialog, isNativeDismiss: Boolean) {
        val traceOperation = navigationTrace?.operation
        if (disposed || dialog !== sourceDialog || !isPresented || dismissAnimationRunning) return
        installDismissTrace(sourceDialog, traceOperation)
        dismissAnimationRunning = true
        presentationAnimationPending = false
        presentationAnimationRunning = false
        presentationTargetDetentIndex = null
        if (isNativeDismiss) nativeDismissedForCurrentRequest = true
        programmaticDismissPending = !isNativeDismiss
        emitOnWillDisappearIfNeeded()
        if (disposed || !dismissAnimationRunning || dialog !== sourceDialog || !isPresented) return
        val bottomSheet =
            sourceDialog.findViewById<View>(com.google.android.material.R.id.design_bottom_sheet)
        val backdrop =
            sourceDialog.findViewById<View>(com.google.android.material.R.id.touch_outside)
        if (latestConfig?.animated == false || bottomSheet == null || backdrop == null) {
            navigationTrace?.start(isAnimated = false)
            emitNativeDismissEventIfNeeded(isNativeDismiss, traceOperation)
            if (!disposed && dialog === sourceDialog) sourceDialog.dismiss()
            return
        }
        val parentHeight =
            (bottomSheet.parent as? View)?.height?.takeIf { it > 0 }
                ?: sourceDialog.window?.decorView?.height?.takeIf { it > 0 }
                ?: resources.displayMetrics.heightPixels
        bottomSheet.animate().cancel()
        backdrop.animate().cancel()
        navigationTrace?.start(isAnimated = true)
        val startVisualTop = bottomSheet.top + bottomSheet.translationY
        val startBackdropAlpha = backdrop.alpha
        dismissAnimator =
            ValueAnimator.ofFloat(0f, 1f).apply {
                duration = SHEET_ANIMATION_DURATION_MS
                interpolator = SHEET_ANIMATION_INTERPOLATOR
                addUpdateListener { animator ->
                    val progress = animator.animatedValue as Float
                    val visualTop = startVisualTop + (parentHeight - startVisualTop) * progress
                    bottomSheet.translationY = visualTop - bottomSheet.top
                    backdrop.alpha = startBackdropAlpha * (1f - progress)
                }
                addListener(
                    object : AnimatorListenerAdapter() {
                        private var wasCancelled = false

                        override fun onAnimationCancel(animation: Animator) {
                            wasCancelled = true
                            navigationTrace?.cancel("dismiss_animation_cancelled", traceOperation)
                        }

                        override fun onAnimationEnd(animation: Animator) {
                            if (
                                !wasCancelled &&
                                    !disposed &&
                                    dialog === sourceDialog &&
                                    isPresented &&
                                    dismissAnimationRunning
                            ) {
                                emitNativeDismissEventIfNeeded(isNativeDismiss, traceOperation)
                                if (!disposed && dialog === sourceDialog && dismissAnimationRunning)
                                    sourceDialog.dismiss()
                            }
                        }
                    }
                )
                start()
            }
    }

    private fun emitNativeDismissEventIfNeeded(
        isNativeDismiss: Boolean,
        traceOperation: Any? = navigationTrace?.operation,
    ) {
        if (!disposed && isNativeDismiss && !nativeDismissEventEmitted) {
            nativeDismissEventEmitted = true
            withTrace(traceOperation) { eventEmitter.emitOnNativeDismissEvent() }
        }
    }

    private fun withTrace(context: Any?, block: () -> Unit) {
        val trace = navigationTrace
        if (trace != null) trace.withOperation(context, block) else block()
    }

    private fun cancelDismissAndPresent(config: SheetConfiguration) {
        val sourceDialog = dialog ?: return
        val bottomSheet = currentBottomSheet() ?: return
        val visualTop = bottomSheet.top + bottomSheet.translationY
        dismissAnimator?.cancel()
        dismissAnimator = null
        dismissAnimationRunning = false
        nativeDismissedForCurrentRequest = false
        nativeDismissEventEmitted = false
        programmaticDismissPending = false
        hasEmittedWillDisappear = false
        presentationAnimationPending = true
        navigationTrace?.beginPresent()
        configurePresentedSheet(config, applyInitialDetent = false)
        bottomSheet.translationY = visualTop - bottomSheet.top
        startPresentationAnimation(sourceDialog, startOffscreen = false)
    }

    private fun cancelAnimations(sourceDialog: BottomSheetDialog?) {
        dismissAnimator?.cancel()
        dismissAnimator?.removeAllUpdateListeners()
        dismissAnimator?.removeAllListeners()
        dismissAnimator = null
        sourceDialog
            ?.findViewById<View>(com.google.android.material.R.id.design_bottom_sheet)
            ?.animate()
            ?.apply {
                withEndAction(null)
                cancel()
            }
        sourceDialog
            ?.findViewById<View>(com.google.android.material.R.id.touch_outside)
            ?.animate()
            ?.cancel()
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun installBackdrop(sourceDialog: BottomSheetDialog) {
        val touchOutside =
            sourceDialog.findViewById<View>(com.google.android.material.R.id.touch_outside)
                ?: return
        touchOutside.setBackgroundColor(latestConfig?.backdropColor ?: Color.BLACK)
        touchOutside.alpha = 0f
        var startedOutsideSheet = false
        val detector =
            GestureDetector(
                context,
                object : GestureDetector.SimpleOnGestureListener() {
                    override fun onDown(event: MotionEvent): Boolean {
                        startedOutsideSheet = isOutsideSheet(sourceDialog, event)
                        return startedOutsideSheet
                    }

                    override fun onSingleTapUp(event: MotionEvent): Boolean {
                        if (!startedOutsideSheet || !isOutsideSheet(sourceDialog, event))
                            return false
                        requestNativeDismiss(SheetDismissChannel.BACKDROP, sourceDialog)
                        return true
                    }
                },
            )
        touchOutside.setOnClickListener(null)
        touchOutside.setOnTouchListener { _, event ->
            detector.onTouchEvent(event)
            true
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun installBottomSheetOutsideSurfaceHandler(
        sourceDialog: BottomSheetDialog,
        bottomSheet: View,
    ) {
        var startedOutsideSurface = false
        val detector =
            GestureDetector(
                context,
                object : GestureDetector.SimpleOnGestureListener() {
                    override fun onDown(event: MotionEvent): Boolean = true

                    override fun onSingleTapUp(event: MotionEvent): Boolean {
                        if (!startedOutsideSurface || !isOutsideSheet(sourceDialog, event))
                            return false
                        requestNativeDismiss(SheetDismissChannel.BACKDROP, sourceDialog)
                        return true
                    }
                },
            )
        bottomSheet.setOnTouchListener { _, event ->
            if (event.actionMasked == MotionEvent.ACTION_DOWN) {
                startedOutsideSurface = isOutsideSheet(sourceDialog, event)
                if (startedOutsideSurface)
                    bottomSheet.parent?.requestDisallowInterceptTouchEvent(true)
            }
            if (!startedOutsideSurface) {
                false
            } else {
                detector.onTouchEvent(event)
                if (
                    event.actionMasked == MotionEvent.ACTION_UP ||
                        event.actionMasked == MotionEvent.ACTION_CANCEL
                ) {
                    bottomSheet.parent?.requestDisallowInterceptTouchEvent(false)
                    startedOutsideSurface = false
                }
                true
            }
        }
    }

    private fun isOutsideSheet(sourceDialog: BottomSheetDialog, event: MotionEvent): Boolean {
        if (disposed || dialog !== sourceDialog) return false
        val bounds = Rect()
        if (!sheetContainer.getGlobalVisibleRect(bounds)) return false
        return !bounds.contains(event.rawX.toInt(), event.rawY.toInt())
    }

    private fun recoverPreventedDragDismiss(behavior: SheetBehavior<*>) {
        if (disposed || recoveringPreventedDragDismiss) return
        recoveringPreventedDragDismiss = true
        navigationTrace?.ensureDismiss(SheetDismissChannel.DRAG)
        navigationTrace?.nativeDismissPrevented(SheetDismissChannel.DRAG)
        eventEmitter.emitOnNativeDismissPreventedEvent(SheetDismissChannel.DRAG)
        if (!disposed && sheetBehavior === behavior)
            behavior.state = stateForIndex(0, currentDetentsCount)
    }

    private fun requestSheetLayoutUpdate(bottomSheet: View) {
        dialog?.window?.decorView?.let(ViewCompat::requestApplyInsets)
        bottomSheet.post {
            if (
                !disposed &&
                    isPresented &&
                    !dismissAnimationRunning &&
                    bottomSheet === currentBottomSheet()
            ) {
                // A content-size callback can run inside ancestor layout, which clears its
                // requestLayout flags on return. Reissue after that traversal so the new
                // intrinsic height reaches the dialog parents instead of retaining 1px.
                sheetContainer.requestLayout()
                bottomSheet.requestLayout()
                updateSheetSurfaceLayout(bottomSheet)
                updateDimming(bottomSheet)
            }
        }
    }

    private fun currentBottomSheet(): FrameLayout? =
        dialog?.findViewById(com.google.android.material.R.id.design_bottom_sheet)

    private fun installWindowInsetsListener(sourceDialog: BottomSheetDialog) {
        val decorView = sourceDialog.window?.decorView ?: return
        val bottomSheet =
            sourceDialog.findViewById<FrameLayout>(
                com.google.android.material.R.id.design_bottom_sheet
            )
        val coordinator = bottomSheet?.parent as? View
        val container = coordinator?.parent as? View
        val clearMaterialSystemBarInsets = {
            container?.fitsSystemWindows = false
            coordinator?.fitsSystemWindows = false
            bottomSheet?.fitsSystemWindows = false
            container?.setPadding(0, 0, 0, 0)
            coordinator?.setPadding(0, 0, 0, 0)
            bottomSheet?.setPadding(0, 0, 0, 0)
            container?.requestLayout()
        }
        clearMaterialSystemBarInsets()
        ViewCompat.setOnApplyWindowInsetsListener(decorView) { _, insets ->
            if (!disposed && dialog === sourceDialog) {
                clearMaterialSystemBarInsets()
                updateSheetSurfaceLayout(bottomSheet)
                decorView.post {
                    if (!disposed && dialog === sourceDialog) {
                        // Child listeners may reapply Material padding after the decor listener.
                        clearMaterialSystemBarInsets()
                        val parent = bottomSheet?.parent as? View
                        val parentWidth = parent?.width?.takeIf { it > 0 } ?: decorView.width
                        val parentHeight = parent?.height?.takeIf { it > 0 } ?: decorView.height
                        host.refreshArea(parentWidth, parentHeight, currentSheetArea)
                        updateSheetSurfaceLayout(bottomSheet)
                    }
                }
            }
            insets
        }
        ViewCompat.requestApplyInsets(decorView)
    }

    private fun observeDialogLayout(sourceDialog: BottomSheetDialog) {
        stopObservingDialogLayout()
        observedDialogDecorView =
            sourceDialog.window?.decorView?.also {
                it.addOnLayoutChangeListener(dialogLayoutChangeListener)
            }
    }

    private fun stopObservingDialogLayout() {
        observedDialogDecorView?.let {
            it.removeOnLayoutChangeListener(dialogLayoutChangeListener)
            ViewCompat.setOnApplyWindowInsetsListener(it, null)
        }
        observedDialogDecorView = null
    }

    private fun trackPotentialDismissDrag(event: MotionEvent) {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                dismissDragStartRawY = event.rawY
                maxDownwardDismissDragDistance = 0f
            }
            MotionEvent.ACTION_MOVE,
            MotionEvent.ACTION_UP,
            MotionEvent.ACTION_CANCEL -> {
                if (!dismissDragStartRawY.isNaN()) {
                    maxDownwardDismissDragDistance =
                        maxOf(maxDownwardDismissDragDistance, event.rawY - dismissDragStartRawY)
                    if (
                        lastEmittedDetentIndex == 0 &&
                            maxDownwardDismissDragDistance > dismissDragDistanceThreshold
                    ) {
                        navigationTrace?.ensureDismiss(SheetDismissChannel.DRAG)
                        navigationTrace?.start(isAnimated = true)
                    }
                }
            }
        }
    }

    private fun updateSheetSurfaceLayout(bottomSheet: View?) {
        if (disposed) return
        if (detents.isFitToContents && bottomSheet != null && bottomSheet.height > 0)
            currentDetentHeights = listOf(bottomSheet.height)
        if (
            bottomSheet != null &&
                (bottomSheet.paddingLeft != 0 ||
                    bottomSheet.paddingTop != 0 ||
                    bottomSheet.paddingRight != 0 ||
                    bottomSheet.paddingBottom != 0)
        )
            bottomSheet.setPadding(0, 0, 0, 0)
        val params = sheetContainer.layoutParams as FrameLayout.LayoutParams
        val containerHeight =
            if (!detents.isFitToContents && bottomSheet != null) {
                val parentHeight = (bottomSheet.parent as? View)?.height ?: 0
                visibleSheetSurfaceHeight(
                    parentHeight = availableSheetBottom(parentHeight),
                    sheetTop = bottomSheet.top,
                    maximumHeight = bottomSheet.height,
                    unmeasuredHeight = params.height,
                )
            } else params.height
        if (
            params.height != containerHeight ||
                params.leftMargin != 0 ||
                params.rightMargin != 0 ||
                params.bottomMargin != 0
        ) {
            params.height = containerHeight
            params.leftMargin = 0
            params.rightMargin = 0
            params.bottomMargin = 0
            sheetContainer.layoutParams = params
        }
        val grabberParams = grabber.layoutParams as FrameLayout.LayoutParams
        if (grabberParams.topMargin != dp(8)) {
            grabberParams.topMargin = dp(8)
            grabber.layoutParams = grabberParams
        }
    }

    private fun visibleSheetHeight(bottomSheet: View, fallbackParentHeight: Int): Int {
        val parentHeight =
            (bottomSheet.parent as? View)?.height?.takeIf { it > 0 } ?: fallbackParentHeight
        return (availableSheetBottom(parentHeight) - bottomSheet.top).coerceAtLeast(0)
    }

    private fun availableSheetBottom(parentHeight: Int): Int =
        currentSheetArea?.bottom?.coerceIn(0, parentHeight) ?: parentHeight

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).roundToInt()

    private companion object {
        // Settling runnables in the historical behavior may outlive the dialog.
        // Do not let their callback retain a disposed controller or its content.
        val INACTIVE_CALLBACK =
            object : SheetBehavior.Callback() {
                override fun onStateChanged(bottomSheet: View, newState: Int) = Unit

                override fun onSlide(bottomSheet: View, slideOffset: Float) = Unit
            }
        val SHEET_ANIMATION_INTERPOLATOR = AccelerateDecelerateInterpolator()
        const val SHEET_ANIMATION_DURATION_MS = 300L
        const val DEFAULT_DIM_AMOUNT = 0.5f
        const val DIM_AMOUNT_EPSILON = 0.001f
        const val DISMISS_ATTEMPT_OFFSET_THRESHOLD = 0.1f
        const val LARGE_SCREEN_MIN_DP = 480
        const val DEFAULT_LARGE_SCREEN_CORNER_RADIUS_DP = 20f
    }
}
