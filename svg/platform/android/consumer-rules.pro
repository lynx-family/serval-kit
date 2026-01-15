# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

# Uncomment this to preserve the line number information for
# debugging stack traces.
#-keepattributes SourceFile,LineNumberTable

# If you keep the line number information, uncomment this to
# hide the original source file name.
#-renamesourcefileattribute SourceFile


# keep the model for the different shapes.
-keep class com.lynx.serval.svg.model.** { *; }

# keep native method
-keep class com.lynx.serval.svg.SVGRenderEngine {
    native <methods>;
}

# keep pubic methods on “svgRender”
-keep public class com.lynx.serval.svg.SVGRender {
    public *;
}

# keep pubic/protect static methods on “SVGRenderEngine”
-keep public class com.lynx.serval.svg.SVGRenderEngine {
    public static *;
}

