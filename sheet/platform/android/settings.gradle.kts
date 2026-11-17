pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "ServalSheet"

include(":serval_sheet", ":serval_sheet_lynx")

include(":sheet_binding_test")
project(":sheet_binding_test").projectDir = file("../../testing/android")
