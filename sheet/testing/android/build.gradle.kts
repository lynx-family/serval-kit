// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.kapt")
}

android {
    namespace = "com.lynx.serval.sheet.testing"
    compileSdk = 33
    defaultConfig { minSdk = 21 }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
    kotlinOptions {
        jvmTarget = "1.8"
        languageVersion = "1.5"
        apiVersion = "1.5"
    }
}

dependencies {
    implementation(project(":serval_sheet_lynx"))
    implementation("org.jetbrains.kotlin:kotlin-stdlib:1.5.32")
    compileOnly("org.lynxsdk.lynx:lynx:4.0.1")
    compileOnly("androidx.annotation:annotation:1.1.0")
    kapt("org.lynxsdk.lynx:lynx-processor:4.0.1")
}
