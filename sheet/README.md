# Native Sheet

Shared native Sheet presentation, sizing, interaction, and Lynx content hosting for iOS and Android.

`ServalSheet` / `serval_sheet` contains the native implementation. It has no dependency on Lynx or a navigation library. `ServalSheetLynx` / `serval_sheet_lynx` supplies the common content hosting and native Sheet element implementation. Consumer repositories register their own elements and translate their public API.

## Platform support

| Platform | Shared native engine | Lynx Sheet binding | FormSheet integration |
| --- | --- | --- | --- |
| iOS | Custom controller | `XElement/Sheet` | Shared engine adapter |
| Android | Dialog and custom Behavior | `LynxXElement:Sheet` | Shared engine adapter |
| HarmonyOS | Not implemented | Not implemented | Not implemented |

The HarmonyOS packages elsewhere in serval-kit do not provide Sheet support.
Adding HarmonyOS requires a native driver, Lynx content hosting and element
registration, plus a FormSheet platform binding. The same public command/event
contract can be retained, with Harmony-specific presentation and gesture behavior
validated on that platform.

## Integration

iOS consumers depend on `ServalSheetLynx` version `0.1.0`. To develop against source, add both local Pods to the application's Podfile:

```ruby
pod 'ServalSheet', :path => ENV.fetch('SERVAL_KIT_ROOT')
pod 'ServalSheetLynx', :path => ENV.fetch('SERVAL_KIT_ROOT')
```

Android consumers depend on `org.lynxsdk.lynx:serval_sheet_lynx:0.1.0`. The companion `serval_sheet` artifact is a transitive dependency. Both modules support Maven publication through their `publishReleasePublicationToMavenLocal` tasks. The Android artifacts target Kotlin language/API 1.5 and Android SDK 33 for consumer compatibility. Consumers must also supply their Lynx SDK; the support module compiles against SDK 4.0.1 without packaging another runtime.

The native implementation uses a custom iOS controller and an Android Material dialog with a shared custom Behavior. `default` appearance and animation refer to this library's defaults. The library does not promise UIKit system Sheet appearance or gesture parity.

## Native element

The Lynx integration registers `sheet`. On iOS, opt into `XElement/Sheet`;
it includes the element and shadow-node registration. It is a separate subspec
so the Sheet dependency does not raise the minimum iOS version of other XElement
consumers. On Android, `LynxXElement` includes the Sheet registration and the
`:LynxXElement:Sheet` module supplies the implementation.

```ruby
pod 'XElement/Sheet'
```

Both shared Pods require iOS 15.0. The current consumer dependency changes use
version `0.1.0`; publish both Pods and both Android artifacts before shipping
those consumer changes, or use the local source overrides during development.

Example native element:

```tsx
<sheet
  id="details"
  initial-open={true}
  snap-points={[0.3, 0.6, 0.9]}
  initial-snap-index={1}
  animation="default"
  dismissible={true}
  appearance={{ backgroundColor: '#FFFFFF', cornerRadius: 24, grabberVisible: true }}
  backdrop={{ color: '#000000', opacity: 0.4, clickToClose: true }}
  bindopenchange={handleOpenChange}
  bindsnapchange={handleSnapChange}
  bindtransitionend={handleTransitionEnd}
>
  <scroll-view scroll-y>{children}</scroll-view>
</sheet>
```

Initialization is consumed once per native instance. Later operations use node methods:

```ts
lynx.createSelectorQuery().select('#details').invoke({
  method: 'snapTo',
  params: { index: 2, animate: true, requestId: 'expand-details' },
}).exec()
```

Methods are `present`, `dismiss`, and `snapTo`. An invocation acknowledges acceptance; `transitionend` reports completion or cancellation. Requests carry an optional `requestId`; native operations carry a `transitionId`. `snapchange` reports stable arrival, while the internal `targetchange` event supports adapters with an earlier callback contract.

The first release accepts one to three strictly increasing fractions in `(0,1]`, or `snap-points="fit"`. Fractions resolve against the presentation host's available height. Color strings accept `transparent`, `#RGB`, `#RRGGBB`, and `#RRGGBBAA` on both platforms. `heightPx` reports physical pixels; corner radius uses logical display units. Native Sheet supports `default` and `none` program animation policies. Custom spring/tween descriptions and custom JSX backdrops are not exposed in this initial implementation.

`dismissible=false` blocks user dismissal while retaining programmatic `dismiss`. It does not disable resizing between snap points. Backdrop opacity controls visuals; a transparent backdrop still blocks background interaction. A footer remains ordinary content.

Supported host names are `page` and `top` on iOS. Android uses the owning Activity's dialog window. Arbitrary custom host identifiers are rejected. The native `SRSheetHost`/`SheetHost` interfaces let integrations supply their own host-area policies.

## API boundary

- The native controller owns the surface, backdrop, gesture/scroll coordination, and animation.
- The session owns command scheduling and semantic operation results.
- Shared content hosting maintains the Lynx logical subtree while mounting native children inside the presenter. Fixed content receives an exact viewport; fit content measures independently of the previous panel height.
- The FormSheet adapter retains its existing prop names, native-dismiss request latch, lifecycle callbacks, and navigation diagnostics. Its host policy is injected rather than imported by the native engine.

## Verification

Android native tests and both AARs:

```sh
cd sheet/platform/android
./gradlew :serval_sheet:testDebugUnitTest :serval_sheet:assembleRelease :serval_sheet_lynx:assembleRelease :sheet_binding_test:assembleDebug
```

Set `JAVA_HOME` to JDK 17 or newer and configure an Android SDK. Tests include geometry/dimming regressions and native dialog lifecycle cases. The separate consumer fixture verifies generated property setters, UI methods, and registration through two levels of subclassing.

iOS native simulator tests:

```sh
ruby sheet/testing/ios/generate.rb
xcodebuild test \
  -project sheet/build/ios-tests/SheetTests.xcodeproj \
  -scheme SheetTests \
  -destination 'platform=iOS Simulator,name=iPhone 16 Pro' \
  CODE_SIGNING_ALLOWED=NO
```

The generator requires the `xcodeproj` gem. Select a simulator installed on the machine. Tests cover commands, operation results, dismissal policies, queued request withdrawal, disposal, and fit-content growth. Runtime content events, scroll handoff, accessibility, keyboard behavior, and mixed-element presentation require integration/device validation.

The borrowed native implementation retains its original [MIT license](LICENSE.screens) alongside the repository license.
