project(base)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(STRING_UTILS_SOURCE
    src/string/string_utils.cc
    src/string/string_number_convert.cc
)

set(TIMER_SOURCES
    src/timer/time_utils.cc
)

set(FML_SOURCES

    src/fml/concurrent_message_loop.cc
    src/fml/cpu_affinity.cc
    src/fml/delayed_task.cc
    src/fml/message_loop.cc
    src/fml/message_loop_impl.cc
    src/fml/message_loop_task_queues.cc
    src/fml/raster_thread_merger.cc
    src/fml/shared_thread_merger.cc
    src/fml/task_runner.cc
    src/fml/task_source.cc
    src/fml/thread.cc
    src/fml/thread_host.cc
    src/fml/unique_fd.cc
    src/fml/memory/task_runner_checker.cc
    src/fml/synchronization/count_down_latch.cc
    src/fml/synchronization/semaphore.cc
    src/fml/synchronization/shared_mutex_std.cc
    src/fml/synchronization/sync_switch.cc
    src/fml/synchronization/waitable_event.cc
    src/fml/time/chrono_timestamp_provider.cc
    src/fml/time/time_point.cc
    src/fml/time/timer.cc
)

set(TRACE_SOURCES
        trace/native/instance_counter_trace.cc
        trace/native/trace_controller.cc
        trace/native/trace_event_export_symbol.cc
        trace/native/trace_event_utils_perfetto_mock.cc
        trace/native/track_event_wrapper_mock.cc
        src/base_trace/trace_event_utils.cc
)

set(ANDROID_UTILS_SOURCES )

if (DEFINED IS_HARMONY)
set(FML_SOURCES ${FML_SOURCES}
    src/fml/platform/harmony/message_loop_harmony.cc
    src/fml/platform/linux/timerfd.cc
)
set(TRACE_SOURCES ${TRACE_SOURCES}
        trace/native/platform/harmony/trace_event_utils_systrace_harmony.cc
)
elseif (DEFINED IS_ANDROID)
set(FML_SOURCES ${FML_SOURCES}
    src/fml/platform/android/cpu_affinity.cc
    src/fml/platform/android/message_loop_android.cc
    src/fml/platform/android/thread_config_setter_android.cc
)
set(TRACE_SOURCES ${TRACE_SOURCES}
        trace/native/trace_event_utils_systrace_android.cc
)
set(ANDROID_UTILS_SOURCES ${ANDROID_UTILS_SOURCES}
    src/platform/android/java_type.cc
    src/platform/android/jni_convert_helper.cc
    src/platform/android/jni_utils.cc
    src/platform/android/scoped_java_ref.cc
)
elseif (DEFINED IS_DARWIN)
set(FML_SOURCES ${FML_SOURCES}
    src/fml/platform/darwin/cf_utils_unittests.mm
    src/fml/platform/darwin/message_loop_darwin.mm
    src/fml/platform/darwin/thread_config_setter_darwin.mm
    src/fml/platform/darwin/thread_name_setter_darwin.cc
)
set(TRACE_SOURCES ${TRACE_SOURCES}
        trace/native/trace_event_utils_systrace_default.cc
)
endif ()

add_library(lynx_base STATIC ${STRING_UTILS_SOURCE} ${FML_SOURCES} ${TIMER_SOURCES} ${ANDROID_UTILS_SOURCES})

add_library(lynx_trace STATIC ${TRACE_SOURCES})

target_compile_definitions(lynx_trace PUBLIC ENABLE_TRACE_SYSTRACE=1)

