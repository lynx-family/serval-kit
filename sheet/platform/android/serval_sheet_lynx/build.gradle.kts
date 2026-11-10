plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.kapt")
    id("maven-publish")
}

group = "org.lynxsdk.lynx"

version = "0.1.0"

android {
    namespace = "com.lynx.serval.sheet.lynx"
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
    publishing { singleVariant("release") { withSourcesJar() } }
}

dependencies {
    implementation("org.jetbrains.kotlin:kotlin-stdlib:1.5.32")
    api(project(":serval_sheet"))
    implementation("androidx.annotation:annotation:1.1.0")
    compileOnly("org.lynxsdk.lynx:lynx:4.0.1")
    kapt("org.lynxsdk.lynx:lynx-processor:4.0.1")
}

afterEvaluate {
    publishing {
        publications { create<MavenPublication>("release") { from(components["release"]) } }
    }
}
