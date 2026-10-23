plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
    id("maven-publish")
}

group = "org.lynxsdk.lynx"

version = "0.1.0"

android {
    namespace = "com.lynx.serval.sheet"
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
    testOptions { unitTests.isIncludeAndroidResources = true }
    publishing { singleVariant("release") { withSourcesJar() } }
}

dependencies {
    implementation("org.jetbrains.kotlin:kotlin-stdlib:1.5.32")
    implementation("androidx.coordinatorlayout:coordinatorlayout:1.2.0")
    implementation("androidx.core:core-ktx:1.5.0")
    implementation("com.google.android.material:material:1.0.0")
    testImplementation("junit:junit:4.13.2")
    testImplementation("org.robolectric:robolectric:4.14.1")
}

afterEvaluate {
    publishing {
        publications { create<MavenPublication>("release") { from(components["release"]) } }
    }
}
