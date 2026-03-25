#!/usr/bin/env python3
"""Generate serval-svg support artifacts and analyze SVG compatibility.

This tool intentionally emits two different knowledge layers:

- a public support contract for `serval-svg`
- an internal audit matrix for `serval-svg-expert`

The current repository contains several cases where a tag is recognized by the
parser but not fully rendered, or is only rendered on a subset of backends.
The audit layer keeps those details explicit, while the public layer exposes
only the product-facing support conclusion.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from collections import OrderedDict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
PARSER_FILE = ROOT / "src" / "parser" / "SrSVGDOM.cc"
ELEMENT_DIR = ROOT / "src" / "element"
EXAMPLE_DIRS = [ROOT / "examples", ROOT / "test_cases"]

PLATFORMS = ("android", "ios", "harmony", "skity")
PUBLIC_PLATFORM_STATUS = {"full": "supported", "partial": "partial", "none": "unsupported"}
ANALYSIS_IGNORED_ATTRS = {"version"}
PUBLIC_TEXT_REPLACEMENTS = OrderedDict(
    [
        (
            "Width is parsed but not used by SrSVGUse::renderRealNode().",
            "Width is currently not applied when rendering a referenced node.",
        ),
        (
            "Height is parsed but not used by SrSVGUse::renderRealNode().",
            "Height is currently not applied when rendering a referenced node.",
        ),
        (
            "Image geometry uses raw length values in onDraw(), so percentage and non-px units are not fully audited.",
            "Image geometry is reliable for common cases, but percentage and non-px units are not fully covered.",
        ),
        (
            "Image drawing bypasses shape fill and stroke state and calls DrawImage() directly.",
            "Image rendering does not use shape fill or stroke painting.",
        ),
        (
            "Image drawing does not consume SrSVGShape render_state opacity.",
            "Image rendering does not fully participate in the generic opacity pipeline.",
        ),
        (
            "Image does not provide a real AsPath() implementation, so clip-path geometry is unreliable.",
            "clip-path behavior on image is unreliable in the current renderer.",
        ),
        (
            "Current text rendering uses paragraph backends and only consumes fill color and font size.",
            "Current text support mainly covers fill color and font size.",
        ),
        (
            "Text color is not multiplied through the generic SrSVGShape opacity pipeline.",
            "Text opacity is limited in the current renderer.",
        ),
        (
            "Text does not provide a real AsPath() implementation for clip-path geometry.",
            "clip-path behavior on text is unreliable in the current renderer.",
        ),
        (
            "Linear gradients are materialized through the canvas UpdateLinearGradient() bridge and then referenced via url(#id).",
            "Linear gradients are supported through the renderer's gradient pipeline.",
        ),
        (
            "ClipPath inherits container parsing, but its AsPath() path union does not apply the container transform.",
            "clipPath container transforms are not fully applied.",
        ),
        (
            "Filters only execute when the canvas backend reports SupportsFilters().",
            "Filters only run on backends with runtime filter support.",
        ),
        (
            "Android, iOS, and Skity can draw images when the embedding app provides an image callback or resource manager.",
            "Android, iOS, and Skity can render images when the host app provides image loading support.",
        ),
        (
            "Android and iOS have paragraph bridges.",
            "Android and iOS provide the current text support path.",
        ),
        (
            "Skity and Harmony paragraph factories currently return no drawable paragraph output.",
            "Skity and Harmony do not currently provide usable text rendering.",
        ),
        (
            "Skity supports pure blur and blur-based drop-shadow heuristics.",
            "Skity provides limited blur and drop-shadow style filter behavior.",
        ),
        (
            "Skity uses feOffset as part of its drop-shadow style filter composition.",
            "Skity uses feOffset only in limited shadow-style filter behavior.",
        ),
        (
            "Skity reads color-matrix values only for its shadow-color heuristic.",
            "Skity uses color-matrix values only in limited shadow-style filter behavior.",
        ),
        (
            "Use renders the referenced node directly from the ID map instead of delegating to platform DrawUse().",
            "",
        ),
        (
            "Harmony DrawImage() is currently empty.",
            "Harmony does not currently provide image rendering.",
        ),
        (
            "Clip paths are converted to canvas clip operations through SrSVGNode::OnPrepareToRender().",
            "clipPath is supported through the renderer's clip pipeline.",
        ),
        (
            "Inline style is only meaningful for the limited text properties that the paragraph bridge consumes.",
            "Inline style only affects the limited text properties currently supported.",
        ),
        (
            "CSS transform inside style is handled specially on the root svg node.",
            "CSS transform inside style has special handling on the root svg element.",
        ),
        (
            "The root node sets the view box transform and then renders child containers.",
            "The root svg element applies viewBox and then renders its children.",
        ),
        (
            "TSpan support is limited to paragraph text style accumulation.",
            "tspan support is limited to incremental text styling.",
        ),
        (
            "Mask rendering uses SaveLayer plus destination-in compositing on all current backends.",
            "Masking is supported across current backends.",
        ),
    ]
)


def ordered_unique(values: Iterable[str]) -> list[str]:
    seen: set[str] = set()
    result: list[str] = []
    for value in values:
        if value not in seen:
            seen.add(value)
            result.append(value)
    return result


def ordered_merge(*groups: Iterable[str]) -> list[str]:
    merged: list[str] = []
    for group in groups:
        merged.extend(group)
    return ordered_unique(merged)


GRAPHIC_ATTRS = [
    "id",
    "fill",
    "stroke",
    "opacity",
    "stroke-width",
    "stroke-dasharray",
    "stroke-dashoffset",
    "stroke-linecap",
    "stroke-linejoin",
    "stroke-miterlimit",
    "fill-opacity",
    "stroke-opacity",
    "clip-path",
    "mask",
    "filter",
    "transform",
    "color",
    "style",
]

SHAPE_ATTRS = ordered_merge(GRAPHIC_ATTRS, ["fill-rule"])
GROUP_ATTRS = list(GRAPHIC_ATTRS)
LINE_GEOMETRY_ATTRS = ["x1", "y1", "x2", "y2"]
RECT_GEOMETRY_ATTRS = ["x", "y", "rx", "ry", "width", "height"]
CIRCLE_GEOMETRY_ATTRS = ["cx", "cy", "r"]
ELLIPSE_GEOMETRY_ATTRS = ["cx", "cy", "rx", "ry"]
POINTS_ATTRS = ["points"]
PATH_ATTRS = ["d"]


def tag_platform(android: str, ios: str, harmony: str, skity: str) -> dict[str, str]:
    return OrderedDict(
        [
            ("android", android),
            ("ios", ios),
            ("harmony", harmony),
            ("skity", skity),
        ]
    )


@dataclass(frozen=True)
class TagSpec:
    node_class: str
    category: str
    recognized: str
    parsed: str
    rendered: str
    render_mode: str
    platform_ready: dict[str, str]
    supported_attributes: list[str]
    partial_attributes: OrderedDict[str, str]
    parsed_but_not_rendered_attributes: OrderedDict[str, str]
    notes: list[str]
    source_files: list[str]


TAG_SPECS: "OrderedDict[str, TagSpec]" = OrderedDict(
    [
        (
            "svg",
            TagSpec(
                node_class="SrSVGSVG",
                category="root-container",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="direct",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=ordered_merge(
                    GROUP_ATTRS,
                    ["viewBox", "preserveAspectRatio"],
                ),
                partial_attributes=OrderedDict(
                    [
                        (
                            "width",
                            "Root width is commonly present in input SVG, but the renderer uses the external viewport context instead of parsing svg@width.",
                        ),
                        (
                            "height",
                            "Root height is commonly present in input SVG, but the renderer uses the external viewport context instead of parsing svg@height.",
                        ),
                        (
                            "opacity",
                            "Container opacity is inherited into children instead of using isolated group compositing.",
                        )
                    ]
                ),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[
                    "The root node sets the view box transform and then renders child containers.",
                    "CSS transform inside style is handled specially on the root svg node.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGSVG.cc",
                    "svg/src/element/SrSVGContainer.cc",
                    "svg/src/element/SrSVGNode.cc",
                ],
            ),
        ),
        (
            "g",
            TagSpec(
                node_class="SrSVGG",
                category="container",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="direct",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=list(GROUP_ATTRS),
                partial_attributes=OrderedDict(
                    [
                        (
                            "opacity",
                            "Group opacity is inherited into children instead of using isolated group compositing.",
                        )
                    ]
                ),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[
                    "Group attributes such as fill, stroke, mask, and color are propagated to child nodes during render.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGContainer.cc",
                    "svg/src/element/SrSVGNode.cc",
                    "svg/include/element/SrSVGG.h",
                ],
            ),
        ),
        (
            "defs",
            TagSpec(
                node_class="SrSVGDefs",
                category="definition-container",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="definition",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=["id"],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        (
                            "transform",
                            "Defs is a non-rendering container; its own transform is parsed but not applied as a render step.",
                        )
                    ]
                ),
                notes=[
                    "Defs is used as a non-rendering container for reusable nodes.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGContainer.cc",
                    "svg/src/element/SrSVGNode.cc",
                    "svg/include/element/SrSVGDefs.h",
                ],
            ),
        ),
        (
            "rect",
            TagSpec(
                node_class="SrSVGRect",
                category="shape",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="direct",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=ordered_merge(SHAPE_ATTRS, RECT_GEOMETRY_ATTRS),
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGRect.cc",
                    "svg/src/element/SrSVGShape.cc",
                    "svg/src/element/SrSVGNode.cc",
                ],
            ),
        ),
        (
            "circle",
            TagSpec(
                node_class="SrSVGCircle",
                category="shape",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="direct",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=ordered_merge(SHAPE_ATTRS, CIRCLE_GEOMETRY_ATTRS),
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGCircle.cc",
                    "svg/src/element/SrSVGShape.cc",
                    "svg/src/element/SrSVGNode.cc",
                ],
            ),
        ),
        (
            "ellipse",
            TagSpec(
                node_class="SrSVGEllipse",
                category="shape",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="direct",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=ordered_merge(SHAPE_ATTRS, ELLIPSE_GEOMETRY_ATTRS),
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGEllipse.cc",
                    "svg/src/element/SrSVGShape.cc",
                    "svg/src/element/SrSVGNode.cc",
                ],
            ),
        ),
        (
            "line",
            TagSpec(
                node_class="SrSVGLine",
                category="shape",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="direct",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=ordered_merge(
                    SHAPE_ATTRS,
                    LINE_GEOMETRY_ATTRS,
                ),
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        (
                            "fill",
                            "Line rendering only issues a draw call when stroke is present.",
                        ),
                        (
                            "fill-rule",
                            "Line rendering is stroke-driven; fill-rule does not affect the output.",
                        ),
                    ]
                ),
                notes=[
                    "A line is effectively stroke-only in the current renderer.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGLine.cc",
                    "svg/src/element/SrSVGShape.cc",
                    "svg/src/element/SrSVGNode.cc",
                ],
            ),
        ),
        (
            "polygon",
            TagSpec(
                node_class="SrSVGPolygon",
                category="shape",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="direct",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=ordered_merge(SHAPE_ATTRS, POINTS_ATTRS),
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGPolygon.cc",
                    "svg/src/element/SrSVGShape.cc",
                    "svg/src/element/SrSVGNode.cc",
                ],
            ),
        ),
        (
            "polyline",
            TagSpec(
                node_class="SrSVGPolyLine",
                category="shape",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="direct",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=ordered_merge(SHAPE_ATTRS, POINTS_ATTRS),
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGPolyLine.cc",
                    "svg/src/element/SrSVGShape.cc",
                    "svg/src/element/SrSVGNode.cc",
                ],
            ),
        ),
        (
            "path",
            TagSpec(
                node_class="SrSVGPath",
                category="shape",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="direct",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=ordered_merge(SHAPE_ATTRS, PATH_ATTRS),
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGPath.cc",
                    "svg/src/element/SrSVGShape.cc",
                    "svg/src/element/SrSVGNode.cc",
                ],
            ),
        ),
        (
            "use",
            TagSpec(
                node_class="SrSVGUse",
                category="reference",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="direct",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=ordered_merge(
                    [
                        "id",
                        "href",
                        "xlink:href",
                        "x",
                        "y",
                        "fill",
                        "stroke",
                        "stroke-width",
                        "stroke-dasharray",
                        "stroke-dashoffset",
                        "stroke-linecap",
                        "stroke-linejoin",
                        "stroke-miterlimit",
                        "fill-opacity",
                        "stroke-opacity",
                        "clip-path",
                        "mask",
                        "filter",
                        "transform",
                        "color",
                        "style",
                    ]
                ),
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        (
                            "width",
                            "Width is parsed but not used by SrSVGUse::renderRealNode().",
                        ),
                        (
                            "height",
                            "Height is parsed but not used by SrSVGUse::renderRealNode().",
                        ),
                        (
                            "opacity",
                            "Opacity is parsed on use but is not forwarded to the referenced node.",
                        ),
                    ]
                ),
                notes=[
                    "Use renders the referenced node directly from the ID map instead of delegating to platform DrawUse().",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGUse.cc",
                    "svg/src/element/SrSVGNode.cc",
                ],
            ),
        ),
        (
            "image",
            TagSpec(
                node_class="SrSVGImage",
                category="image",
                recognized="yes",
                parsed="yes",
                rendered="partial",
                render_mode="direct",
                platform_ready=tag_platform("partial", "partial", "none", "partial"),
                supported_attributes=[
                    "id",
                    "href",
                    "xlink:href",
                    "preserveAspectRatio",
                    "transform",
                    "style",
                ],
                partial_attributes=OrderedDict(
                    [
                        (
                            "x",
                            "Image geometry uses raw length values in onDraw(), so percentage and non-px units are not fully audited.",
                        ),
                        (
                            "y",
                            "Image geometry uses raw length values in onDraw(), so percentage and non-px units are not fully audited.",
                        ),
                        (
                            "width",
                            "Image geometry uses raw length values in onDraw(), so percentage and non-px units are not fully audited.",
                        ),
                        (
                            "height",
                            "Image geometry uses raw length values in onDraw(), so percentage and non-px units are not fully audited.",
                        ),
                    ]
                ),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        (
                            "fill",
                            "Image drawing bypasses shape fill and stroke state and calls DrawImage() directly.",
                        ),
                        (
                            "stroke",
                            "Image drawing bypasses shape fill and stroke state and calls DrawImage() directly.",
                        ),
                        (
                            "opacity",
                            "Image drawing does not consume SrSVGShape render_state opacity.",
                        ),
                        (
                            "fill-opacity",
                            "Image drawing does not consume SrSVGShape render_state opacity.",
                        ),
                        (
                            "stroke-opacity",
                            "Image drawing does not consume SrSVGShape render_state opacity.",
                        ),
                        (
                            "clip-path",
                            "Image does not provide a real AsPath() implementation, so clip-path geometry is unreliable.",
                        ),
                    ]
                ),
                notes=[
                    "Android, iOS, and Skity can draw images when the embedding app provides an image callback or resource manager.",
                    "Harmony DrawImage() is currently empty.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGImage.cc",
                    "svg/platform/android/src/main/cpp/SrAndroidCanvas.cc",
                    "svg/platform/iOS/SrIOSCanvas.mm",
                    "svg/platform/skity/SrSkityCanvas.cc",
                    "svg/platform/harmony/servalsvg/sr_harmony_canvas.cc",
                ],
            ),
        ),
        (
            "text",
            TagSpec(
                node_class="SrSVGText",
                category="text",
                recognized="yes",
                parsed="yes",
                rendered="partial",
                render_mode="direct",
                platform_ready=tag_platform("partial", "partial", "none", "none"),
                supported_attributes=[
                    "id",
                    "fill",
                    "color",
                    "font-size",
                    "x",
                    "y",
                    "text-anchor",
                    "transform",
                    "style",
                ],
                partial_attributes=OrderedDict(
                    [
                        (
                            "style",
                            "Inline style is only meaningful for the limited text properties that the paragraph bridge consumes.",
                        )
                    ]
                ),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        (
                            "stroke",
                            "Current text rendering uses paragraph backends and only consumes fill color and font size.",
                        ),
                        (
                            "stroke-width",
                            "Current text rendering uses paragraph backends and only consumes fill color and font size.",
                        ),
                        (
                            "stroke-linecap",
                            "Current text rendering uses paragraph backends and only consumes fill color and font size.",
                        ),
                        (
                            "stroke-linejoin",
                            "Current text rendering uses paragraph backends and only consumes fill color and font size.",
                        ),
                        (
                            "stroke-dasharray",
                            "Current text rendering uses paragraph backends and only consumes fill color and font size.",
                        ),
                        (
                            "stroke-dashoffset",
                            "Current text rendering uses paragraph backends and only consumes fill color and font size.",
                        ),
                        (
                            "opacity",
                            "Text color is not multiplied through the generic SrSVGShape opacity pipeline.",
                        ),
                        (
                            "fill-opacity",
                            "Text color is not multiplied through the generic SrSVGShape opacity pipeline.",
                        ),
                        (
                            "clip-path",
                            "Text does not provide a real AsPath() implementation for clip-path geometry.",
                        ),
                    ]
                ),
                notes=[
                    "Android and iOS have paragraph bridges.",
                    "Skity and Harmony paragraph factories currently return no drawable paragraph output.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGText.cc",
                    "svg/platform/android/src/main/cpp/SrAndroidParagraphFactory.cc",
                    "svg/platform/iOS/SrIOSParagraph.mm",
                    "svg/include/platform/skity/SrSkityParagraph.h",
                    "svg/include/platform/harmony/sr_harmony_paragraph.h",
                ],
            ),
        ),
        (
            "tspan",
            TagSpec(
                node_class="SrSVGTextSpan",
                category="text-span",
                recognized="yes",
                parsed="yes",
                rendered="partial",
                render_mode="direct",
                platform_ready=tag_platform("partial", "partial", "none", "none"),
                supported_attributes=[
                    "id",
                    "fill",
                    "color",
                    "font-size",
                    "style",
                ],
                partial_attributes=OrderedDict(
                    [
                        (
                            "style",
                            "Inline style is only meaningful for the limited text properties that the paragraph bridge consumes.",
                        )
                    ]
                ),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        (
                            "transform",
                            "TSpan inherits generic node parsing but the paragraph builder does not apply per-span transforms.",
                        ),
                        (
                            "stroke",
                            "Current text rendering uses paragraph backends and only consumes fill color and font size.",
                        ),
                        (
                            "opacity",
                            "Current text rendering does not apply the generic opacity pipeline per span.",
                        ),
                    ]
                ),
                notes=[
                    "TSpan support is limited to paragraph text style accumulation.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGText.cc",
                    "svg/include/element/SrSVGText.h",
                ],
            ),
        ),
        (
            "linearGradient",
            TagSpec(
                node_class="SrSVGLinearGradient",
                category="gradient-definition",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="definition",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=[
                    "id",
                    "gradientTransform",
                    "spreadMethod",
                    "x1",
                    "x2",
                    "y1",
                    "y2",
                    "gradientUnits",
                ],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[
                    "Linear gradients are materialized through the canvas UpdateLinearGradient() bridge and then referenced via url(#id).",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGLinearGradient.cc",
                    "svg/platform/android/src/main/java/SVGRender.java",
                    "svg/platform/iOS/SrIOSCanvas.mm",
                    "svg/platform/harmony/servalsvg/sr_harmony_canvas.cc",
                    "svg/platform/skity/SrSkityCanvas.cc",
                ],
            ),
        ),
        (
            "radialGradient",
            TagSpec(
                node_class="SrSVGRadialGradient",
                category="gradient-definition",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="definition",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=[
                    "id",
                    "gradientTransform",
                    "gradientUnits",
                    "spreadMethod",
                    "cx",
                    "cy",
                    "r",
                    "fx",
                    "fy",
                ],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[
                    "Android ignores a distinct radial-gradient focus point in the final shader and uses cx/cy for the center.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGRadialGradient.cc",
                    "svg/platform/android/src/main/java/SVGRender.java",
                    "svg/platform/iOS/SrIOSCanvas.mm",
                    "svg/platform/harmony/servalsvg/sr_harmony_canvas.cc",
                    "svg/platform/skity/SrSkityCanvas.cc",
                ],
            ),
        ),
        (
            "stop",
            TagSpec(
                node_class="SrSVGStop",
                category="gradient-stop",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="definition",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=["offset", "stop-color", "stop-opacity", "style"],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGStop.cc",
                ],
            ),
        ),
        (
            "clipPath",
            TagSpec(
                node_class="SrSVGClipPath",
                category="clip-definition",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="definition",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=["id", "clipPathUnits", "clip-rule"],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        (
                            "transform",
                            "ClipPath inherits container parsing, but its AsPath() path union does not apply the container transform.",
                        )
                    ]
                ),
                notes=[
                    "Clip paths are converted to canvas clip operations through SrSVGNode::OnPrepareToRender().",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGClipPath.cc",
                    "svg/src/element/SrSVGNode.cc",
                ],
            ),
        ),
        (
            "mask",
            TagSpec(
                node_class="SrSVGMask",
                category="mask-definition",
                recognized="yes",
                parsed="yes",
                rendered="yes",
                render_mode="definition",
                platform_ready=tag_platform("full", "full", "full", "full"),
                supported_attributes=[
                    "id",
                    "mask-type",
                    "maskUnits",
                    "maskContentUnits",
                    "x",
                    "y",
                    "width",
                    "height",
                    "transform",
                ],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(),
                notes=[
                    "Mask rendering uses SaveLayer plus destination-in compositing on all current backends.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGMask.cc",
                    "svg/src/element/SrSVGNode.cc",
                    "svg/platform/android/src/main/java/SVGRender.java",
                    "svg/platform/iOS/SrIOSCanvas.mm",
                    "svg/platform/harmony/servalsvg/sr_harmony_canvas.cc",
                    "svg/platform/skity/SrSkityCanvas.cc",
                ],
            ),
        ),
        (
            "filter",
            TagSpec(
                node_class="SrSVGFilter",
                category="filter-definition",
                recognized="yes",
                parsed="yes",
                rendered="partial",
                render_mode="definition",
                platform_ready=tag_platform("none", "none", "none", "partial"),
                supported_attributes=["id"],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        (
                            "x",
                            "Filter region coordinates are parsed but current backends do not use them to drive filter evaluation.",
                        ),
                        (
                            "y",
                            "Filter region coordinates are parsed but current backends do not use them to drive filter evaluation.",
                        ),
                        (
                            "width",
                            "Filter region coordinates are parsed but current backends do not use them to drive filter evaluation.",
                        ),
                        (
                            "height",
                            "Filter region coordinates are parsed but current backends do not use them to drive filter evaluation.",
                        ),
                        (
                            "filterUnits",
                            "Filter units are parsed but current backends do not consume them.",
                        ),
                        (
                            "primitiveUnits",
                            "Primitive units are parsed but current backends do not consume them.",
                        ),
                    ]
                ),
                notes=[
                    "Filters only execute when the canvas backend reports SupportsFilters().",
                    "At present that is the Skity backend.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGFilter.cc",
                    "svg/src/element/SrSVGNode.cc",
                    "svg/include/canvas/SrCanvas.h",
                    "svg/include/platform/skity/SrSkityCanvas.h",
                    "svg/platform/skity/SrSkityCanvas.cc",
                ],
            ),
        ),
        (
            "feGaussianBlur",
            TagSpec(
                node_class="SrSVGFeGaussianBlur",
                category="filter-primitive",
                recognized="yes",
                parsed="yes",
                rendered="partial",
                render_mode="definition",
                platform_ready=tag_platform("none", "none", "none", "partial"),
                supported_attributes=["stdDeviation"],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        ("in", "Filter input graph selection is parsed but not evaluated."),
                        ("result", "Filter result chaining is parsed but not evaluated."),
                        ("x", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("y", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("width", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("height", "Primitive subregion coordinates are parsed but not evaluated."),
                    ]
                ),
                notes=[
                    "Skity supports pure blur and blur-based drop-shadow heuristics.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGFilterPrimitives.cc",
                    "svg/platform/skity/SrSkityCanvas.cc",
                ],
            ),
        ),
        (
            "feOffset",
            TagSpec(
                node_class="SrSVGFeOffset",
                category="filter-primitive",
                recognized="yes",
                parsed="yes",
                rendered="partial",
                render_mode="definition",
                platform_ready=tag_platform("none", "none", "none", "partial"),
                supported_attributes=["dx", "dy"],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        ("in", "Filter input graph selection is parsed but not evaluated."),
                        ("result", "Filter result chaining is parsed but not evaluated."),
                        ("x", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("y", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("width", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("height", "Primitive subregion coordinates are parsed but not evaluated."),
                    ]
                ),
                notes=[
                    "Skity uses feOffset as part of its drop-shadow style filter composition.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGFilterPrimitives.cc",
                    "svg/platform/skity/SrSkityCanvas.cc",
                ],
            ),
        ),
        (
            "feColorMatrix",
            TagSpec(
                node_class="SrSVGFeColorMatrix",
                category="filter-primitive",
                recognized="yes",
                parsed="yes",
                rendered="partial",
                render_mode="definition",
                platform_ready=tag_platform("none", "none", "none", "partial"),
                supported_attributes=["type", "values"],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        ("in", "Filter input graph selection is parsed but not evaluated."),
                        ("result", "Filter result chaining is parsed but not evaluated."),
                        ("x", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("y", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("width", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("height", "Primitive subregion coordinates are parsed but not evaluated."),
                    ]
                ),
                notes=[
                    "Skity reads color-matrix values only for its shadow-color heuristic.",
                ],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGFilterPrimitives.cc",
                    "svg/platform/skity/SrSkityCanvas.cc",
                ],
            ),
        ),
        (
            "feComposite",
            TagSpec(
                node_class="SrSVGFeComposite",
                category="filter-primitive",
                recognized="yes",
                parsed="yes",
                rendered="no",
                render_mode="definition",
                platform_ready=tag_platform("none", "none", "none", "none"),
                supported_attributes=[],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        ("in", "Filter input graph selection is parsed but not evaluated."),
                        ("in2", "Composite inputs are parsed but not evaluated."),
                        ("operator", "Composite operator is parsed but not evaluated."),
                        ("k1", "Arithmetic coefficients are parsed but not evaluated."),
                        ("k2", "Arithmetic coefficients are parsed but not evaluated."),
                        ("k3", "Arithmetic coefficients are parsed but not evaluated."),
                        ("k4", "Arithmetic coefficients are parsed but not evaluated."),
                        ("result", "Filter result chaining is parsed but not evaluated."),
                        ("x", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("y", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("width", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("height", "Primitive subregion coordinates are parsed but not evaluated."),
                    ]
                ),
                notes=[],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGFilterPrimitives.cc",
                ],
            ),
        ),
        (
            "feBlend",
            TagSpec(
                node_class="SrSVGFeBlend",
                category="filter-primitive",
                recognized="yes",
                parsed="yes",
                rendered="no",
                render_mode="definition",
                platform_ready=tag_platform("none", "none", "none", "none"),
                supported_attributes=[],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        ("in", "Filter input graph selection is parsed but not evaluated."),
                        ("in2", "Blend inputs are parsed but not evaluated."),
                        ("mode", "Blend mode is parsed but not evaluated."),
                        ("result", "Filter result chaining is parsed but not evaluated."),
                        ("x", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("y", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("width", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("height", "Primitive subregion coordinates are parsed but not evaluated."),
                    ]
                ),
                notes=[],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGFilterPrimitives.cc",
                ],
            ),
        ),
        (
            "feFlood",
            TagSpec(
                node_class="SrSVGFeFlood",
                category="filter-primitive",
                recognized="yes",
                parsed="yes",
                rendered="no",
                render_mode="definition",
                platform_ready=tag_platform("none", "none", "none", "none"),
                supported_attributes=[],
                partial_attributes=OrderedDict(),
                parsed_but_not_rendered_attributes=OrderedDict(
                    [
                        ("flood-color", "Flood color is parsed but not evaluated by any backend."),
                        ("flood-opacity", "Flood opacity is parsed but not evaluated by any backend."),
                        ("in", "Filter input graph selection is parsed but not evaluated."),
                        ("result", "Filter result chaining is parsed but not evaluated."),
                        ("x", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("y", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("width", "Primitive subregion coordinates are parsed but not evaluated."),
                        ("height", "Primitive subregion coordinates are parsed but not evaluated."),
                    ]
                ),
                notes=[],
                source_files=[
                    "svg/src/parser/SrSVGDOM.cc",
                    "svg/src/element/SrSVGFilterPrimitives.cc",
                ],
            ),
        ),
    ]
)


def parse_parser_entries() -> OrderedDict[str, str]:
    content = PARSER_FILE.read_text(encoding="utf-8")
    pattern = re.compile(
        r'else if \(strcmp\(el, "([^"]+)"\) == 0\) \{\s*node = element::([A-Za-z0-9_]+)::Make\(\);',
        re.MULTILINE,
    )
    entries = OrderedDict()
    for tag, node_class in pattern.findall(content):
        entries[tag] = node_class
    first_match = re.search(
        r'if \(strcmp\(el, "([^"]+)"\) == 0\) \{\s*node = element::([A-Za-z0-9_]+)::Make\(\);',
        content,
        re.MULTILINE,
    )
    if first_match:
        first_tag, first_class = first_match.groups()
        entries = OrderedDict([(first_tag, first_class), *entries.items()])
    return entries


def parse_local_attributes(file_path: Path) -> list[str]:
    content = file_path.read_text(encoding="utf-8")
    attrs = re.findall(r'strcmp\(name, "([^"]+)"\)', content)
    attrs.extend(re.findall(r'strcmp\("([^"]+)", name\)', content))
    return ordered_unique(attrs)


def collect_example_hits(tag: str) -> list[str]:
    pattern = re.compile(rf"<\s*{re.escape(tag)}(?:[\s>/])")
    hits: list[str] = []
    for base_dir in EXAMPLE_DIRS:
        if not base_dir.exists():
            continue
        for file_path in sorted(base_dir.rglob("*.svg")):
            try:
                text = file_path.read_text(encoding="utf-8")
            except UnicodeDecodeError:
                continue
            if pattern.search(text):
                hits.append(file_path.relative_to(ROOT).as_posix())
    return hits


def validate_specs() -> tuple[OrderedDict[str, str], dict[str, list[str]]]:
    parser_entries = parse_parser_entries()
    parser_tags = set(parser_entries)
    spec_tags = set(TAG_SPECS)

    missing_from_specs = sorted(parser_tags - spec_tags)
    missing_from_parser = sorted(spec_tags - parser_tags)

    if missing_from_specs or missing_from_parser:
        messages = []
        if missing_from_specs:
            messages.append(f"missing spec entries: {', '.join(missing_from_specs)}")
        if missing_from_parser:
            messages.append(f"missing parser entries: {', '.join(missing_from_parser)}")
        raise RuntimeError("Tag spec drift detected: " + "; ".join(messages))

    for tag, node_class in parser_entries.items():
        expected = TAG_SPECS[tag].node_class
        if node_class != expected:
            raise RuntimeError(
                f"Parser node class drift for <{tag}>: expected {expected}, got {node_class}"
            )

    local_attr_map: dict[str, list[str]] = {}
    for file_path in sorted(ELEMENT_DIR.glob("*.cc")):
        local_attr_map[file_path.name] = parse_local_attributes(file_path)

    return parser_entries, local_attr_map


def build_audit_matrix() -> OrderedDict[str, object]:
    parser_entries, local_attr_map = validate_specs()

    tags = OrderedDict()
    for tag, spec in TAG_SPECS.items():
        evidence = OrderedDict(
            [
                ("parser_entry", "svg/src/parser/SrSVGDOM.cc"),
                ("source_files", spec.source_files),
                (
                    "local_attribute_extracts",
                    OrderedDict(
                        [
                            (name, local_attr_map.get(Path(name).name, []))
                            for name in spec.source_files
                            if name.startswith("svg/src/element/")
                        ]
                    ),
                ),
            ]
        )

        tag_data = OrderedDict(
            [
                ("node_class", parser_entries[tag]),
                ("category", spec.category),
                (
                    "status",
                    OrderedDict(
                        [
                            ("recognized", spec.recognized),
                            ("parsed", spec.parsed),
                            ("rendered", spec.rendered),
                            ("render_mode", spec.render_mode),
                        ]
                    ),
                ),
                ("platform_ready", spec.platform_ready),
                (
                    "attributes",
                    OrderedDict(
                        [
                            ("supported", spec.supported_attributes),
                            ("partial", spec.partial_attributes),
                            ("parsed_but_not_rendered", spec.parsed_but_not_rendered_attributes),
                        ]
                    ),
                ),
                ("notes", spec.notes),
                ("examples", collect_example_hits(tag)),
                ("evidence", evidence),
            ]
        )
        tags[tag] = tag_data

    return OrderedDict(
        [
            ("schema_version", 2),
            ("generator", "svg/tools/svg_support_audit.py"),
            ("role", "internal_audit_matrix"),
            (
                "support_levels",
                OrderedDict(
                    [
                        ("recognized", "parser can instantiate the node"),
                        ("parsed", "attributes are captured into node state"),
                        ("rendered", "current render path uses the node meaningfully"),
                        ("platform_ready", "backend-specific readiness: full, partial, or none"),
                    ]
                ),
            ),
            (
                "platform_capabilities",
                OrderedDict(
                    [
                        (
                            "android",
                            [
                                "Core shapes, gradients, masks, clip paths, and text have dedicated bridges.",
                                "Filters are not executed because the Android canvas backend does not override SupportsFilters().",
                                "Image rendering depends on a ResourceManager callback.",
                            ],
                        ),
                        (
                            "ios",
                            [
                                "Core shapes, gradients, masks, clip paths, and text have Quartz/TextKit implementations.",
                                "Filters are not executed because the iOS canvas backend does not override SupportsFilters().",
                                "Image rendering depends on an image callback.",
                            ],
                        ),
                        (
                            "harmony",
                            [
                                "Core shapes, gradients, masks, and clip paths are implemented.",
                                "Text and image backends are currently incomplete.",
                                "Filters are not executed because the Harmony canvas backend does not override SupportsFilters().",
                            ],
                        ),
                        (
                            "skity",
                            [
                                "Core shapes, gradients, masks, clip paths, and image rendering are implemented.",
                                "Text paragraphs are currently stubbed.",
                                "Filter execution is available but limited to a small subset of primitives and heuristics.",
                            ],
                        ),
                    ]
                ),
            ),
            (
                "global_notes",
                [
                    "Inline style support is property-by-property. A style declaration only works when its property is parsed by the receiving node.",
                    "A tag being recognized by the parser does not imply full spec support.",
                    "The repository currently relies more on sample SVG assets than on engine-level unit tests for SVG behavior.",
                ],
            ),
            ("tags", tags),
        ]
    )


def public_platform_status(audit_status: str) -> str:
    return PUBLIC_PLATFORM_STATUS[audit_status]


def public_tag_support(tag_info: OrderedDict[str, object]) -> str:
    rendered = tag_info["status"]["rendered"]
    platform_states = set(tag_info["platform_ready"].values())
    if rendered == "no":
        return "unsupported"
    if rendered == "partial":
        return "partial"
    if platform_states == {"full"}:
        return "supported"
    return "partial"


def public_global_caveats() -> list[str]:
    return [
        "Only the tags and attributes listed here are part of the public support contract.",
        "Unlisted attributes should be treated as unsupported until the expert workflow audits and publishes them.",
        "Platform differences are part of the support answer. Do not collapse them into a single optimistic claim.",
    ]


def sanitize_public_text(text: str) -> str:
    sanitized = text
    for original, replacement in PUBLIC_TEXT_REPLACEMENTS.items():
        sanitized = sanitized.replace(original, replacement)
    sanitized = sanitized.replace("SupportsFilters()", "runtime filter support")
    sanitized = sanitized.replace("AsPath()", "path conversion")
    sanitized = sanitized.replace("onDraw()", "the current rendering path")
    sanitized = re.sub(r"Sr[A-Za-z0-9_:]+(?:\(\))?", "the current renderer", sanitized)
    sanitized = re.sub(r"Draw[A-Za-z0-9_]+\(\)", "the current drawing path", sanitized)
    sanitized = re.sub(r"Update[A-Za-z0-9_]+\(\)", "the current renderer bridge", sanitized)
    return sanitized


def sanitize_public_attr_map(attr_map: OrderedDict[str, str]) -> OrderedDict[str, str]:
    return OrderedDict((attr, sanitize_public_text(reason)) for attr, reason in attr_map.items())


def build_support_matrix() -> OrderedDict[str, object]:
    audit_matrix = build_audit_matrix()
    tags = OrderedDict()

    for tag, tag_info in audit_matrix["tags"].items():
        tags[tag] = OrderedDict(
            [
                ("support", public_tag_support(tag_info)),
                (
                    "platforms",
                    OrderedDict(
                        (platform, public_platform_status(status))
                        for platform, status in tag_info["platform_ready"].items()
                    ),
                ),
                ("supported_attributes", tag_info["attributes"]["supported"]),
                (
                    "partially_supported_attributes",
                    sanitize_public_attr_map(tag_info["attributes"]["partial"]),
                ),
                (
                    "unsupported_attributes",
                    sanitize_public_attr_map(tag_info["attributes"]["parsed_but_not_rendered"]),
                ),
                (
                    "caveats",
                    [
                        note
                        for note in ordered_unique(
                            sanitize_public_text(note) for note in tag_info["notes"]
                        )
                        if note
                    ],
                ),
            ]
        )

    return OrderedDict(
        [
            ("schema_version", 2),
            ("generator", "svg/tools/svg_support_audit.py"),
            ("role", "public_support_contract"),
            (
                "support_classes",
                OrderedDict(
                    [
                        (
                            "supported",
                            "Supported for the listed platforms when used with the documented attribute set.",
                        ),
                        (
                            "partial",
                            "Available with backend limits, semantic caveats, or reduced attribute coverage.",
                        ),
                        ("unsupported", "Not part of the supported surface."),
                    ]
                ),
            ),
            (
                "platform_classes",
                OrderedDict(
                    [
                        ("supported", "Available on this backend"),
                        ("partial", "Available with backend-specific caveats"),
                        ("unsupported", "Not available on this backend"),
                    ]
                ),
            ),
            (
                "policy",
                OrderedDict(
                    [
                        (
                            "tag_rule",
                            "Only tags listed in this file are claimed by the public serval-svg skill.",
                        ),
                        (
                            "attribute_rule",
                            "Only attributes listed under supported_attributes or partially_supported_attributes are claimed. Unlisted attributes are not part of the public contract.",
                        ),
                    ]
                ),
            ),
            ("global_caveats", public_global_caveats()),
            ("tags", tags),
        ]
    )


def canonicalize_attr_name(attr_name: str) -> str:
    if attr_name.startswith("{"):
        uri, local = attr_name[1:].split("}", 1)
        if "xlink" in uri and local == "href":
            return "xlink:href"
        return local
    return attr_name


def canonicalize_tag(tag_name: str) -> str:
    if tag_name.startswith("{"):
        return tag_name.split("}", 1)[1]
    return tag_name


def read_svg_text(args: argparse.Namespace) -> str:
    if args.svg_file:
        if args.svg_file == "-":
            return sys.stdin.read()
        return Path(args.svg_file).read_text(encoding="utf-8")
    if args.svg_text is not None:
        return args.svg_text
    return sys.stdin.read()


def finalize_analysis(
    profile: str,
    platform: str,
    elements: list[OrderedDict[str, object]],
    issues: list[OrderedDict[str, object]],
) -> OrderedDict[str, object]:
    errors = sum(1 for issue in issues if issue["severity"] == "error")
    warnings = sum(1 for issue in issues if issue["severity"] == "warning")
    verdict = "supported"
    if errors:
        verdict = "unsupported"
    elif warnings:
        verdict = "partial"

    return OrderedDict(
        [
            ("profile", profile),
            ("platform", platform),
            ("verdict", verdict),
            ("error_count", errors),
            ("warning_count", warnings),
            ("elements", elements),
            ("issues", issues),
        ]
    )


def analyze_audit_svg(matrix: OrderedDict[str, object], svg_text: str, platform: str) -> OrderedDict[str, object]:
    root = ET.fromstring(svg_text)
    tags = matrix["tags"]

    issues: list[OrderedDict[str, object]] = []
    elements: list[OrderedDict[str, object]] = []

    for element in root.iter():
        tag_name = canonicalize_tag(element.tag)
        tag_info = tags.get(tag_name)
        attr_names = [canonicalize_attr_name(name) for name in element.attrib]

        element_summary = OrderedDict(
            [
                ("tag", tag_name),
                ("attributes", attr_names),
            ]
        )

        if tag_info is None:
            issues.append(
                OrderedDict(
                    [
                        ("severity", "error"),
                        ("kind", "unsupported-tag"),
                        ("tag", tag_name),
                        ("message", f"<{tag_name}> is not recognized by the current parser."),
                    ]
                )
            )
            elements.append(element_summary)
            continue

        platform_status = tag_info["platform_ready"][platform] if platform != "all" else None
        if tag_info["status"]["rendered"] == "no":
            issues.append(
                OrderedDict(
                    [
                        ("severity", "error"),
                        ("kind", "unrendered-tag"),
                        ("tag", tag_name),
                        ("message", f"<{tag_name}> is parsed but not rendered by the current engine."),
                    ]
                )
            )
        elif platform_status == "none":
            issues.append(
                OrderedDict(
                    [
                        ("severity", "error"),
                        ("kind", "platform-unsupported-tag"),
                        ("tag", tag_name),
                        ("message", f"<{tag_name}> is not available on the {platform} backend."),
                    ]
                )
            )
        elif platform_status == "partial" or tag_info["status"]["rendered"] == "partial":
            issues.append(
                OrderedDict(
                    [
                        ("severity", "warning"),
                        ("kind", "partial-tag"),
                        ("tag", tag_name),
                        ("message", f"<{tag_name}> is only partially supported{'' if platform == 'all' else f' on {platform}'}."),
                    ]
                )
            )

        supported = set(tag_info["attributes"]["supported"])
        partial = tag_info["attributes"]["partial"]
        parsed_not_rendered = tag_info["attributes"]["parsed_but_not_rendered"]

        for attr_name in attr_names:
            if attr_name in partial:
                issues.append(
                    OrderedDict(
                        [
                            ("severity", "warning"),
                            ("kind", "partial-attribute"),
                            ("tag", tag_name),
                            ("attribute", attr_name),
                            ("message", partial[attr_name]),
                        ]
                    )
                )
            elif attr_name in parsed_not_rendered:
                issues.append(
                    OrderedDict(
                        [
                            ("severity", "error"),
                            ("kind", "parsed-not-rendered-attribute"),
                            ("tag", tag_name),
                            ("attribute", attr_name),
                            ("message", parsed_not_rendered[attr_name]),
                        ]
                    )
                )
            elif attr_name not in supported:
                issues.append(
                    OrderedDict(
                        [
                            ("severity", "warning"),
                            ("kind", "unknown-attribute"),
                            ("tag", tag_name),
                            ("attribute", attr_name),
                            (
                                "message",
                                f"{attr_name} is not listed in the curated support matrix for <{tag_name}>.",
                            ),
                        ]
                    )
                )

        elements.append(element_summary)

    return finalize_analysis("audit", platform, elements, issues)


def analyze_support_svg(matrix: OrderedDict[str, object], svg_text: str, platform: str) -> OrderedDict[str, object]:
    root = ET.fromstring(svg_text)
    tags = matrix["tags"]

    issues: list[OrderedDict[str, object]] = []
    elements: list[OrderedDict[str, object]] = []

    for element in root.iter():
        tag_name = canonicalize_tag(element.tag)
        tag_info = tags.get(tag_name)
        attr_names = [canonicalize_attr_name(name) for name in element.attrib]

        element_summary = OrderedDict(
            [
                ("tag", tag_name),
                ("attributes", attr_names),
            ]
        )

        if tag_info is None:
            issues.append(
                OrderedDict(
                    [
                        ("severity", "error"),
                        ("kind", "unsupported-tag"),
                        ("tag", tag_name),
                        ("message", f"<{tag_name}> is not part of the published serval-svg support surface."),
                    ]
                )
            )
            elements.append(element_summary)
            continue

        if tag_info["support"] == "unsupported":
            issues.append(
                OrderedDict(
                    [
                        ("severity", "error"),
                        ("kind", "unsupported-tag"),
                        ("tag", tag_name),
                        ("message", f"<{tag_name}> is not part of the supported serval-svg surface."),
                    ]
                )
            )
        elif tag_info["support"] == "partial":
            issues.append(
                OrderedDict(
                    [
                        ("severity", "warning"),
                        ("kind", "partial-tag"),
                        ("tag", tag_name),
                        ("message", f"<{tag_name}> is only partially supported by serval-svg."),
                    ]
                )
            )

        if platform != "all":
            platform_status = tag_info["platforms"][platform]
            if platform_status == "unsupported":
                issues.append(
                    OrderedDict(
                        [
                            ("severity", "error"),
                            ("kind", "platform-unsupported-tag"),
                            ("tag", tag_name),
                            ("message", f"<{tag_name}> is not supported on the {platform} backend."),
                        ]
                    )
                )
            elif platform_status == "partial":
                issues.append(
                    OrderedDict(
                        [
                            ("severity", "warning"),
                            ("kind", "platform-partial-tag"),
                            ("tag", tag_name),
                            ("message", f"<{tag_name}> is only partially supported on the {platform} backend."),
                        ]
                    )
                )

        supported = set(tag_info["supported_attributes"])
        partial = tag_info["partially_supported_attributes"]
        unsupported = tag_info["unsupported_attributes"]

        for attr_name in attr_names:
            if attr_name in ANALYSIS_IGNORED_ATTRS:
                continue
            if attr_name in partial:
                issues.append(
                    OrderedDict(
                        [
                            ("severity", "warning"),
                            ("kind", "partial-attribute"),
                            ("tag", tag_name),
                            ("attribute", attr_name),
                            ("message", partial[attr_name]),
                        ]
                    )
                )
            elif attr_name in unsupported:
                issues.append(
                    OrderedDict(
                        [
                            ("severity", "error"),
                            ("kind", "unsupported-attribute"),
                            ("tag", tag_name),
                            ("attribute", attr_name),
                            ("message", unsupported[attr_name]),
                        ]
                    )
                )
            elif attr_name not in supported:
                issues.append(
                    OrderedDict(
                        [
                            ("severity", "warning"),
                            ("kind", "unlisted-attribute"),
                            ("tag", tag_name),
                            ("attribute", attr_name),
                            (
                                "message",
                                f"{attr_name} is not part of the published support contract for <{tag_name}>.",
                            ),
                        ]
                    )
                )

        elements.append(element_summary)

    return finalize_analysis("public", platform, elements, issues)


def print_human_analysis(result: OrderedDict[str, object]) -> None:
    print(f"Profile: {result['profile']}")
    print(f"Platform: {result['platform']}")
    print(f"Verdict: {result['verdict']}")
    print(f"Errors: {result['error_count']}")
    print(f"Warnings: {result['warning_count']}")
    print()

    if not result["issues"]:
        print("No compatibility issues found in the selected support profile.")
        return

    print("Issues:")
    for issue in result["issues"]:
        attr = issue.get("attribute")
        location = f"<{issue['tag']}>"
        if attr:
            location += f" @{attr}"
        print(f"- [{issue['severity']}] {location}: {issue['message']}")


def cmd_matrix(args: argparse.Namespace) -> int:
    matrix = build_support_matrix()
    output = json.dumps(matrix, ensure_ascii=False, indent=2) + "\n"
    if args.output:
        Path(args.output).write_text(output, encoding="utf-8")
    else:
        sys.stdout.write(output)
    return 0


def cmd_audit_matrix(args: argparse.Namespace) -> int:
    matrix = build_audit_matrix()
    output = json.dumps(matrix, ensure_ascii=False, indent=2) + "\n"
    if args.output:
        Path(args.output).write_text(output, encoding="utf-8")
    else:
        sys.stdout.write(output)
    return 0


def cmd_analyze(args: argparse.Namespace) -> int:
    matrix = build_support_matrix() if args.profile == "public" else build_audit_matrix()
    svg_text = read_svg_text(args)
    if args.profile == "public":
        result = analyze_support_svg(matrix, svg_text, args.platform)
    else:
        result = analyze_audit_svg(matrix, svg_text, args.platform)
    if args.format == "json":
        sys.stdout.write(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
    else:
        print_human_analysis(result)
    return 1 if result["verdict"] == "unsupported" else 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    matrix_parser = subparsers.add_parser(
        "matrix", help="Generate the public support contract JSON"
    )
    matrix_parser.add_argument("--output", help="Write JSON to a file")
    matrix_parser.set_defaults(func=cmd_matrix)

    audit_matrix_parser = subparsers.add_parser(
        "audit-matrix", help="Generate the internal audit matrix JSON"
    )
    audit_matrix_parser.add_argument("--output", help="Write JSON to a file")
    audit_matrix_parser.set_defaults(func=cmd_audit_matrix)

    analyze_parser = subparsers.add_parser(
        "analyze", help="Analyze an SVG snippet or file against a support profile"
    )
    analyze_parser.add_argument("--svg-file", help="Read SVG from a file path, or '-' for stdin")
    analyze_parser.add_argument("--svg-text", help="Analyze the provided SVG text directly")
    analyze_parser.add_argument(
        "--profile",
        choices=("public", "audit"),
        default="public",
        help="Use the public support contract or the internal audit matrix",
    )
    analyze_parser.add_argument(
        "--platform",
        choices=("all", *PLATFORMS),
        default="all",
        help="Target a specific backend instead of the aggregate view",
    )
    analyze_parser.add_argument(
        "--format",
        choices=("human", "json"),
        default="human",
        help="Output format",
    )
    analyze_parser.set_defaults(func=cmd_analyze)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
