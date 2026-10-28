// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.serval.sheet.testing

import com.lynx.serval.sheet.lynx.SheetContentShadowNode
import com.lynx.serval.sheet.lynx.SheetElement
import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxShadowNode
import com.lynx.tasm.behavior.LynxGeneratorName

// A separate module makes the processor resolve inherited props and methods from the published API.
open class ConsumerSheet(context: LynxContext) : SheetElement(context)

open class ConsumerShadowNode : SheetContentShadowNode()

@LynxGeneratorName(packageName = "com.lynx.serval.sheet.testing")
@LynxBehavior(tagName = ["test-sheet"], isCreateAsync = false)
class ConsumerSheetRegistration(context: LynxContext) : ConsumerSheet(context)

@LynxGeneratorName(packageName = "com.lynx.serval.sheet.testing")
@LynxShadowNode(tagName = "test-sheet")
class ConsumerShadowRegistration : ConsumerShadowNode()
