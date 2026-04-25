??? note "`dtadc`&nbsp;&nbsp;&nbsp;Analog-to-digital converter facade with model-numbered dispatch."
    Defines a vtable interface for ADC devices: activate (with a per-scan
    callback), deactivate, status query, and string formatting. A global
    registry maps integer model numbers to concrete vtables, allowing
    platform-specific ADC implementations (ESP-IDF one-shot, Zephyr SAADC,
    etc.) to be swapped without changing call sites. Companion macros
    DTADC_DECLARE_API and DTADC_INIT_VTABLE reduce boilerplate when wiring
    in a new implementation.

    [dtmc_base_library/include/dtmc_base/dtadc.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtadc.h){target="_blank"}
??? note "`dtadc_scan`&nbsp;&nbsp;&nbsp;Single ADC acquisition record with binary serialization."
    Represents one ADC sampling event: a nanosecond timestamp, a monotonic
    sequence number, and a variable-length array of int32 channel readings.
    The packx functions serialize and deserialize the scan to and from a
    compact binary format, enabling transmission over serial links or network
    portals with explicit offset tracking and bounds checking.

    [dtmc_base_library/include/dtmc_base/dtadc_scan.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtadc_scan.h){target="_blank"}
??? note "`dtadc_scanlist`&nbsp;&nbsp;&nbsp;Batched ADC scan collection with binary serialization."
    Groups a variable number of dtadc_scan_t records and their shared channel
    count into a single transferable unit. Provides init, pack-length query,
    pack, unpack, and dispose, following the same packx conventions used
    throughout the library. Suited for batching ADC samples before
    transmission or logging.

    [dtmc_base_library/include/dtmc_base/dtadc_scanlist.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtadc_scanlist.h){target="_blank"}
??? note "`dtbuffer_distributor`&nbsp;&nbsp;&nbsp;Bidirectional dtbuffer relay over a netportal topic."
    Connects a dtbufferqueue to a dtnetportal subscription on a named topic.
    Incoming messages are chunked for transmission via dtchunker and placed
    into the queue; outbound calls pull from the queue and publish. Separating
    queuing and transport concerns lets producers and consumers run on
    different tasks without coupling them to a specific network backend.

    [dtmc_base_library/include/dtmc_base/dtbuffer_distributor.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtbuffer_distributor.h){target="_blank"}
??? note "`dtbufferqueue`&nbsp;&nbsp;&nbsp;Thread-safe FIFO queue for dtbuffer_t pointers."
    Provides bounded-capacity put and get operations with millisecond timeouts
    and optional overwrite-on-full behavior. The opaque handle hides the
    underlying synchronization mechanism, allowing the same queue API to work
    across RTOS and POSIX environments. Used throughout the library to decouple
    producers from consumers across task boundaries.

    [dtmc_base_library/include/dtmc_base/dtbufferqueue.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtbufferqueue.h){target="_blank"}
??? note "`dtbusywork`&nbsp;&nbsp;&nbsp;CPU load generator for per-core utilization measurement."
    Spawns configurable background tasks that spin continuously at the lowest
    scheduler priority, consuming idle CPU time. A snapshot function samples
    cumulative dttasker CPU-usage counters; subtracting busy-idle from total
    elapsed time yields an accurate utilization figure even on RTOS schedulers
    that lack built-in profiling. Summarize produces a human-readable report.

    [dtmc_base_library/include/dtmc_base/dtbusywork.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtbusywork.h){target="_blank"}
??? note "`dtcpu`&nbsp;&nbsp;&nbsp;Platform-specific CPU timing, busy-wait, and identification."
    Provides a lightweight abstraction over hardware timing: a mark-and-elapsed
    pair for microsecond-resolution interval measurement, a busy-wait primitive,
    a random integer generator, and a permanent unique-identifier string for
    the running CPU or platform instance. Implemented separately for each
    supported target (ESP-IDF, Zephyr, Linux, RP2040, etc.).

    [dtmc_base_library/include/dtmc_base/dtcpu.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtcpu.h){target="_blank"}
??? note "`dtdebouncer`&nbsp;&nbsp;&nbsp;Time-qualified debounce filter for binary signals."
    Accepts raw boolean samples and emits a stable state only after the
    candidate has held steady for a configurable millisecond interval. An
    optional callback fires on each accepted transition. The instance tracks
    separate counts for raw edges, debounced edges, rising transitions, and
    falling transitions, supporting both callback-driven and polled usage
    patterns without GPIO or platform dependencies.

    [dtmc_base_library/include/dtmc_base/dtdebouncer.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtdebouncer.h){target="_blank"}
??? note "`dtdisplay`&nbsp;&nbsp;&nbsp;Display device facade with model-numbered vtable dispatch."
    Defines a uniform interface for blitting raster images to a display,
    attaching and detaching from hardware resources, and creating raster
    buffers that match the display's pixel format. A global registry maps
    integer model numbers to concrete vtables, supporting SDL2, LVGL, and
    hardware LCD backends (e.g., ILI9341) without changes at call sites.

    [dtmc_base_library/include/dtmc_base/dtdisplay.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtdisplay.h){target="_blank"}
??? note "`dtdotstar`&nbsp;&nbsp;&nbsp;APA102/DotStar RGB LED strip facade with model-numbered dispatch."
    Defines a vtable interface for driving addressable LED strips: connect
    (initialize the bus), dither (map linear lumen values to gamma-corrected
    RGB), transmit (push the current frame to hardware), enqueue (stage a
    frame for deferred transmission), and a post callback for async completion
    notification. A global registry allows platform implementations (ESP-IDF
    SPI, Zephyr, etc.) to be substituted at initialization time.

    [dtmc_base_library/include/dtmc_base/dtdotstar.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtdotstar.h){target="_blank"}
??? note "`dtdotstar_dummy`&nbsp;&nbsp;&nbsp;Null-hardware test double for the dtdotstar LED interface."
    Implements the full dtdotstar vtable (connect, dither, transmit, enqueue,
    set_post_cb, dispose) without any SPI or DMA dependency. Call counters
    on the struct let unit tests verify that the correct sequence of
    operations was invoked without requiring physical APA102/DotStar hardware.

    [dtmc_base_library/include/dtmc_base/dtdotstar_dummy.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtdotstar_dummy.h){target="_blank"}
??? note "`dtflowmonitor`&nbsp;&nbsp;&nbsp;Data-flow rate monitor with heartbeat logging and timeout alerting."
    Tracks the arrival time and byte count of incoming data streams. A
    configurable no-data timeout triggers a warning log when the stream goes
    silent. Periodic heartbeat events record throughput (scan count and byte
    count) since the last interval, providing a lightweight diagnostic trace
    for serial or network data paths without external tooling.

    [dtmc_base_library/include/dtmc_base/dtflowmonitor.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtflowmonitor.h){target="_blank"}
??? note "`dtframer`&nbsp;&nbsp;&nbsp;Message framing facade with model-numbered vtable dispatch."
    Defines a uniform interface for encoding a topic-and-payload pair into a
    byte stream and for decoding an incoming byte stream back into topic and
    payload, delivering complete messages via a callback. The opaque handle
    and global vtable registry allow framing algorithms (simple, CRC,
    compressed, encrypted) to be swapped at initialization without changing
    surrounding transport or application code.

    [dtmc_base_library/include/dtmc_base/dtframer.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtframer.h){target="_blank"}
??? note "`dtframer_simple`&nbsp;&nbsp;&nbsp;Lightweight topic-version-payload framer without CRC or compression."
    A concrete dtframer implementation that encodes topic, protocol version,
    and payload into a framed byte stream and decodes it back. A framing mode
    hint (STREAM vs. BUS) tunes maximum topic and payload sizes for UART-style
    streams or CAN-style short messages. Suited for environments where the
    transport layer already provides error detection.

    [dtmc_base_library/include/dtmc_base/dtframer_simple.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtframer_simple.h){target="_blank"}
??? note "`dtgpiopin`&nbsp;&nbsp;&nbsp;Cross-platform GPIO pin facade with interrupt support."
    Defines a vtable-dispatched interface for a single configured GPIO pin:
    attach an ISR callback with edge selection, enable or disable interrupt
    delivery, read or write the logic level, and format pin state as a string.
    A global model-number registry allows platform implementations (ESP-IDF,
    Zephyr, PicoSDK, RPi Linux, PYNQ, Arduino) to register and be resolved
    at runtime without recompiling call sites.

    [dtmc_base_library/include/dtmc_base/dtgpiopin.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtgpiopin.h){target="_blank"}
??? note "`dtgpiopin_dummy`&nbsp;&nbsp;&nbsp;Deterministic GPIO pin stand-in for unit testing."
    Implements the full dtgpiopin vtable (attach, enable, read, write,
    concat_format, dispose) in software. Configurable behavior flags control
    whether writes trigger edge callbacks and whether enabling the pin emits
    an initial edge, making it possible to exercise interrupt-driven logic
    under a single-threaded test runner without real GPIO hardware or SDKs.

    [dtmc_base_library/include/dtmc_base/dtgpiopin_dummy.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtgpiopin_dummy.h){target="_blank"}
??? note "`dthttpd`&nbsp;&nbsp;&nbsp;HTTP server facade with vtable dispatch and POST callback model."
    Defines a uniform interface for running an embedded HTTP server: loop
    (accept connections until stopped), stop (signal shutdown), join (wait
    for clean exit with timeout), and dispose. A typed POST callback delivers
    request path, payload, and response-buffer ownership to the application
    without binding it to a specific server library or network stack.

    [dtmc_base_library/include/dtmc_base/dthttpd.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dthttpd.h){target="_blank"}
??? note "`dtinterval`&nbsp;&nbsp;&nbsp;Periodic callback timer facade with model-numbered dispatch."
    Defines a vtable interface for a repeating interval timer: bind a callback,
    start firing, and pause. The opaque handle and global registry allow
    concrete implementations (wall-clock scheduled, ESP-IDF timer, Zephyr
    timer) to be selected at initialization. The callback receives a context
    pointer and a should_pause output flag, enabling self-cancellation without
    external coordination.

    [dtmc_base_library/include/dtmc_base/dtinterval.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtinterval.h){target="_blank"}
??? note "`dtinterval_scheduled`&nbsp;&nbsp;&nbsp;Wall-clock periodic callback timer implementing the dtinterval facade."
    Fires a registered callback at a fixed millisecond period using
    dtruntime_now_milliseconds and dtruntime_sleep_milliseconds.

    It's named "scheduled" because it is running on task schedule, not an ISR or hardware timer.
    Suitable for any platform that exposes a millisecond wall-clock without dedicated timer peripherals.

    Each sleep is trimmed by the time already consumed inside the callback, preventing
    execution overhead from accumulating into the period. If the callback
    overruns by more than one interval, missed ticks are skipped and the
    schedule resumes on the original phase.

    [dtmc_base_library/include/dtmc_base/dtinterval_scheduled.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtinterval_scheduled.h){target="_blank"}
??? note "`dtiox`&nbsp;&nbsp;&nbsp;Byte-stream I/O exchange facade for UART, CAN, and socket backends."
    Defines a vtable interface for non-blocking asynchronous byte I/O: attach
    and detach hardware resources, enable or disable the channel, read received
    bytes from an internal buffer, write bytes for asynchronous transmission,
    and bind a semaphore for RX-ready notification. A global model-number
    registry supports a wide range of backends (ESP-IDF UART/CAN, Zephyr,
    PicoSDK, Linux TTY/CAN/TCP, Modbus RTU and TCP) without changing
    surrounding transport code.

    [dtmc_base_library/include/dtmc_base/dtiox.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtiox.h){target="_blank"}
??? note "`dtiox_dummy`&nbsp;&nbsp;&nbsp;In-memory test double for the dtiox byte-stream I/O interface."
    Implements the full dtiox vtable (attach, detach, enable, read, write,
    set_rx_semaphore, concat_format, dispose) using internal ring buffers.
    An optional loopback flag causes writes to appear immediately as received
    data. The push_rx_bytes helper injects arbitrary byte sequences to exercise
    receive-path logic without physical UART or socket hardware.

    [dtmc_base_library/include/dtmc_base/dtiox_dummy.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtiox_dummy.h){target="_blank"}
??? note "`dtiox_modbus`&nbsp;&nbsp;&nbsp;Wire-level Modbus holding-register protocol for blob exchange."
    Defines the register map, command codes, and status values for a
    master/slave blob-transfer protocol layered on top of Modbus TCP/RTU
    holding registers. Masters write blobs by issuing PUT_BLOB and poll for
    slave-originated data with GIVE_ME_ANY_DATA. A helper macro converts
    byte counts to the minimum register count needed to carry the payload.

    [dtmc_base_library/include/dtmc_base/dtiox_modbus.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtiox_modbus.h){target="_blank"}
??? note "`dtlightsensor`&nbsp;&nbsp;&nbsp;Ambient light sensor facade with model-numbered vtable dispatch."
    Defines a minimal interface for light sensors: activate the sensor, take
    an int64 lux sample, and dispose. A global registry maps model numbers to
    concrete vtables, enabling dummy (test) and Zephyr sensor implementations
    to be resolved at runtime without compile-time coupling.

    [dtmc_base_library/include/dtmc_base/dtlightsensor.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtlightsensor.h){target="_blank"}
??? note "`dtlightsensor_dummy`&nbsp;&nbsp;&nbsp;Null-hardware test double for the dtlightsensor interface."
    Implements the dtlightsensor vtable (activate, sample, dispose) without
    physical sensor hardware. Invocation counters on the struct allow unit
    tests to confirm that the expected call sequence occurred, using the same
    create/init/configure lifecycle as production sensor implementations.

    [dtmc_base_library/include/dtmc_base/dtlightsensor_dummy.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtlightsensor_dummy.h){target="_blank"}
??? note "`dtlock`&nbsp;&nbsp;&nbsp;Platform-portable mutual exclusion lock facade."
    Wraps a system mutex behind an opaque handle with create, acquire, release,
    and dispose operations. The thin interface hides POSIX pthread_mutex,
    FreeRTOS mutex, and other platform-specific primitives behind a common
    API, keeping higher-level modules free of RTOS or OS headers.

    [dtmc_base_library/include/dtmc_base/dtlock.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtlock.h){target="_blank"}
??? note "`dtmanifold`&nbsp;&nbsp;&nbsp;Fixed-capacity publish/subscribe message router."
    Routes dtbuffer_t payloads to multiple named subscribers using a
    statically allocated subject table (up to 10 subjects, 10 recipients
    each). An optional external semaphore serializes subscribe/unsubscribe
    mutations while leaving delivery callbacks unlocked for maximum
    throughput. Each subscriber receives its own heap copy of the published
    buffer and is responsible for disposing it.

    [dtmc_base_library/include/dtmc_base/dtmanifold.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtmanifold.h){target="_blank"}
??? note "`dtmc_base`&nbsp;&nbsp;&nbsp;Library flavor and other library identification macros."
    Provides DTMC_BASE_FLAVOR string constant that
    identifies the library build at runtime.  Also includes the appropriate version file. Used by dtruntime and diagnostic
    routines to report the active library variant in environment tables and
    log output.

    This file is processed by tooling in the automated build system.
    It's important to maintain the structure and formatting of the version macros for compatibility with version parsing scripts.

    [dtmc_base_library/include/dtmc_base/dtmc_base.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtmc_base.h){target="_blank"}
??? note "`dtmc_base_constants`&nbsp;&nbsp;&nbsp;Centralized model numbers and shared string constants."
    Auto-generated from YAML. Assigns unique integer identifiers to every
    concrete implementation of each vtable-based facade (tasker, interval,
    netportal, ADC, display, IOX, framer, NV blob, timeseries, etc.) across
    all supported platforms (ESP-IDF, Zephyr, PicoSDK, Linux, BLE, Matter).
    Also defines the shared topic and role strings used by the netprofile
    subsystem.

    [dtmc_base_library/include/dtmc_base/dtmc_base_constants.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtmc_base_constants.h){target="_blank"}
??? note "`dtmcp4728`&nbsp;&nbsp;&nbsp;Facade for the MCP4728 quad 12-bit DAC over I2C."
    Defines a vtable-dispatched interface for a Microchip MCP4728 four-channel
    12-bit DAC. Each channel is independently configured with an output value, voltage reference and gain.

    A global model-number registry allows platform
    implementations to register and be resolved at runtime without recompiling
    call sites.

    [dtmc_base_library/include/dtmc_base/dtmcp4728.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtmcp4728.h){target="_blank"}
??? note "`dtmcp4728_dummy`&nbsp;&nbsp;&nbsp;Null-hardware test double for the dtmcp4728 DAC interface."
    Implements the full dtmcp4728 vtable (attach, detach, fast_write, multi_write,
    sequential_write, single_write_eeprom, general_call_reset, general_call_wakeup,
    general_call_software_update, read_all, to_string, dispose) without any I2C
    or hardware dependency. Call counters and captured channel state let unit tests
    verify the correct sequence of operations without physical MCP4728 hardware.

    [dtmc_base_library/include/dtmc_base/dtmcp4728_dummy.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtmcp4728_dummy.h){target="_blank"}
??? note "`dtmodbus_helpers`&nbsp;&nbsp;&nbsp;Byte-to-register packing utilities for Modbus payloads."
    Provides two functions that convert between a byte array and an array of
    16-bit Modbus holding registers, packing two bytes per register in network
    byte order. Used by both master and slave implementations to prepare and
    extract blob payloads exchanged over the dtiox_modbus wire protocol.

    [dtmc_base_library/include/dtmc_base/dtmodbus_helpers.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtmodbus_helpers.h){target="_blank"}
??? note "`dtnetbridge`&nbsp;&nbsp;&nbsp;Two-portal message bridge with queue-buffered forwarding."
    Connects a north and south dtnetportal through paired dtbufferqueues,
    forwarding messages received on registered topics from one side to the
    other. A cancellation semaphore coordinates clean shutdown. Encapsulates
    the forwarding task lifecycle so that bridging two network backends
    (e.g., MQTT to CAN) requires only topic registration and a single
    activate call.

    [dtmc_base_library/include/dtmc_base/dtnetbridge.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtnetbridge.h){target="_blank"}
??? note "`dtnetportal`&nbsp;&nbsp;&nbsp;Publish/subscribe network portal facade with vtable dispatch."
    Defines a transport-agnostic interface for topic-based messaging: activate
    the connection, subscribe to a named topic with a receive callback, publish
    a dtbuffer payload, and query connection metadata. A global model-number
    registry supports over a dozen backends (MQTT, CoAP, BLE, TCP, UDP,
    Modbus, CAN, shared memory, stream) without changing the application layer.

    [dtmc_base_library/include/dtmc_base/dtnetportal.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtnetportal.h){target="_blank"}
??? note "`dtnetportal_iox`&nbsp;&nbsp;&nbsp;Netportal implementation over a framed serial IOX channel."
    Adapts a dtiox byte-stream device and a dtframer codec into a full
    dtnetportal (activate, subscribe, publish, get_info, dispose). An RX task
    drains the IOX FIFO and delivers decoded topic-payload pairs to subscribers.
    The rx_drain breakout function allows testing the decode path without a
    live RTOS task scheduler.

    [dtmc_base_library/include/dtmc_base/dtnetportal_iox.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtnetportal_iox.h){target="_blank"}
??? note "`dtnetportal_shm`&nbsp;&nbsp;&nbsp;Shared-memory netportal implementation."
    Implements the dtnetportal interface using POSIX shared memory as the
    transport, enabling low-latency inter-process publish/subscribe on Linux
    without network overhead. Follows the standard create/init/configure
    lifecycle and registers its vtable for model-number dispatch via the
    dtnetportal registry.

    [dtmc_base_library/include/dtmc_base/dtnetportal_shm.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtnetportal_shm.h){target="_blank"}
??? note "`dtnetprofile`&nbsp;&nbsp;&nbsp;Network node metadata record with incremental assembly and serialization."
    Accumulates descriptive attributes for a networked node into newline-
    separated string lists covering names, component versions, URLs, roles,
    subscriptions, publications, and states. Provides pack/unpack for binary
    transport and a to_string formatter for diagnostics. The incremental add
    API lets each subsystem append its own entries without pre-allocating a
    fixed-size structure.

    [dtmc_base_library/include/dtmc_base/dtnetprofile.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtnetprofile.h){target="_blank"}
??? note "`dtnetprofile_distributor`&nbsp;&nbsp;&nbsp;Publishes and collects dtnetprofile records over a netportal."
    Subscribes to the network-profile topic on a dtnetportal and places
    arriving serialized profiles into a dtbufferqueue. The distribute function
    serializes a local dtnetprofile_t and publishes it on the same topic.
    Separating serialization, queuing, and transport keeps each layer
    independently testable.

    [dtmc_base_library/include/dtmc_base/dtnetprofile_distributor.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtnetprofile_distributor.h){target="_blank"}
??? note "`dtnvblob`&nbsp;&nbsp;&nbsp;Non-volatile binary blob storage facade with model-numbered dispatch."
    Defines a minimal interface for persistent binary storage: read and write
    an opaque byte blob via an opaque handle. A global registry maps model
    numbers to concrete flash/NVS backends (ESP-IDF NVS, Zephyr flash, PicoSDK
    flash, Linux file) so higher-level modules such as dtradioconfig can access
    persistent data without platform conditionals.

    [dtmc_base_library/include/dtmc_base/dtnvblob.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtnvblob.h){target="_blank"}
??? note "`dtpackable_object_distributor`&nbsp;&nbsp;&nbsp;Distributes packable objects over a netportal topic."
    Adapts the dtpackable serialization interface to topic-based network
    transport via a dtnetportal and a dtbufferqueue. Large objects are
    transparently chunked using dtchunker before transmission. The collect
    operation blocks until a complete object arrives or a timeout expires,
    decoupling the producer's serialization rate from the consumer's
    processing rate.

    [dtmc_base_library/include/dtmc_base/dtpackable_object_distributor.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtpackable_object_distributor.h){target="_blank"}
??? note "`dtradioconfig`&nbsp;&nbsp;&nbsp;WiFi and MQTT credential bundle with NV persistence."
    Holds the node name, WiFi SSID/password, and MQTT host, port, and
    credentials required to connect an embedded node to a broker. The
    structure is read from and written to a dtnvblob handle for persistence
    across reboots, and exposes packx serialization for over-the-air delivery
    or factory provisioning.

    [dtmc_base_library/include/dtmc_base/dtradioconfig.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtradioconfig.h){target="_blank"}
??? note "`dtruntime`&nbsp;&nbsp;&nbsp;Runtime environment inspection, timing, and device reporting."
    Provides the library flavor and version strings, millisecond wall-clock
    access and sleep, QEMU detection, and formatted table output for both the
    software environment and the registered hardware devices. Task registration
    with a dttasker_registry allows the runtime to enumerate and report all
    active tasks in a single diagnostic call.

    [dtmc_base_library/include/dtmc_base/dtruntime.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtruntime.h){target="_blank"}
??? note "`dtsemaphore`&nbsp;&nbsp;&nbsp;Cross-platform counting semaphore facade."
    Wraps platform semaphore primitives (POSIX, FreeRTOS, Zephyr) behind a
    common create/wait/post/dispose API with millisecond-resolution timeouts.
    The opaque handle and standard dterr_t error return make it a building
    block for higher-level synchronization throughout the library, including
    dtmanifold, dthttpd, dtnetportal, and dtbufferqueue.

    [dtmc_base_library/include/dtmc_base/dtsemaphore.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtsemaphore.h){target="_blank"}
??? note "`dttasker`&nbsp;&nbsp;&nbsp;Cross-platform task facade with priority bands and CPU accounting."
    Manages the full lifecycle of a task: create, start, ready signaling,
    cooperative stop request, join with timeout, and dispose. A three-band
    priority system (background, normal, urgent) maps to platform-specific
    scheduler priorities at implementation time. The info structure exposes
    stack usage, core affinity, cumulative CPU microseconds, and percent
    utilization, enabling runtime diagnostics without platform-specific
    profiling tools.

    [dtmc_base_library/include/dtmc_base/dttasker.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dttasker.h){target="_blank"}
??? note "`dttasker_registry`&nbsp;&nbsp;&nbsp;Global registry for tracking active dttasker instances."
    Maintains a dtguidable_pool of dttasker handles, supporting insertion,
    name-based lookup, task count, iteration, and formatted table output. A
    global singleton (dttasker_registry_global_instance) lets dtruntime
    enumerate all known tasks for diagnostics without requiring a registry
    pointer to be threaded through the call stack.

    [dtmc_base_library/include/dtmc_base/dttasker_registry.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dttasker_registry.h){target="_blank"}
??? note "`dttimeseries`&nbsp;&nbsp;&nbsp;Time-series data source facade with model-numbered vtable dispatch."
    Defines a minimal interface for any source that returns a double-precision
    value keyed by a microsecond timestamp: configure (via a key-value list),
    read (at a given time), to_string, and dispose. The vtable registry allows
    multiple backend models (steady-state, interpolating, recorded) to be
    selected at runtime and composed into larger signal-processing pipelines.

    [dtmc_base_library/include/dtmc_base/dttimeseries.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dttimeseries.h){target="_blank"}
??? note "`dttimeseries_beat`&nbsp;&nbsp;&nbsp;Beat-frequency signal source implementing the dttimeseries facade."
    Evaluates offset + amplitude * (sin(2*pi*f1*t) + sin(2*pi*f2*t)) for a
    microsecond timestamp t converted to seconds.

    Produces a continuously cycling waveform whose amplitude envelope pulses at
    the beat rate |f1-f2| Hz while the carrier oscillates at (f1+f2)/2 Hz.
    Selecting different f1/f2 pairs per channel gives each channel a distinct
    visual rhythm with no decay. Follows the standard create/configure/read
    lifecycle and registers its vtable for polymorphic dispatch alongside other
    dttimeseries backends.

    [dtmc_base_library/include/dtmc_base/dttimeseries_beat.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dttimeseries_beat.h){target="_blank"}
??? note "`dttimeseries_browngrav`&nbsp;&nbsp;&nbsp;Brownian random-walk signal source implementing the dttimeseries facade."
    Produces a continuous double-precision value stream by advancing a gravitationally-attracted
    Brownian walk one step per read call; the microsecond timestamp argument is not consumed.

    Provides a bounded, continuously-varying synthetic signal useful for sensor simulation and
    test fixture generation. A configurable restoring force, noise intensity, and seed control
    the character of the walk. An offset and scale factor -- defaulting to 0 and 1.0 -- map the
    raw integer output to any target numeric range without external post-processing.

    Follows the standard create/configure/read lifecycle and registers its vtable for polymorphic
    dispatch alongside other dttimeseries backends.

    [dtmc_base_library/include/dtmc_base/dttimeseries_browngrav.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dttimeseries_browngrav.h){target="_blank"}
??? note "`dttimeseries_damped_sine`&nbsp;&nbsp;&nbsp;Exponentially decaying sinusoidal time series implementing the dttimeseries facade."
    Returns amplitude * e^(-decay * t) * sin(2π * frequency * t) for a given microsecond
    timestamp t, where frequency is in Hz, decay is in 1/s (larger values decay faster),
    and t is in seconds. Models resonant mechanical or electrical systems that ring down
    after excitation. Follows the standard create lifecycle and registers its vtable for
    model-number dispatch via the dttimeseries registry.

    [dtmc_base_library/include/dtmc_base/dttimeseries_damped_sine.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dttimeseries_damped_sine.h){target="_blank"}
??? note "`dttimeseries_sine`&nbsp;&nbsp;&nbsp;Sinusoidal signal source implementing the dttimeseries facade."
    Evaluates offset + amplitude * sin(2*pi * frequency * t) for a microsecond
    timestamp t converted to seconds. Frequency is in Hz; all three parameters
    are double-precision and set at configuration time.

    Provides a stateless, analytically exact periodic signal for test fixtures and
    signal-processing pipelines. Each read is a direct formula evaluation with no
    accumulated state, so arbitrary timestamps can be queried in any order without
    drift.

    Follows the standard create/configure/read lifecycle and registers its
    vtable for polymorphic dispatch alongside other dttimeseries backends.

    [dtmc_base_library/include/dtmc_base/dttimeseries_sine.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dttimeseries_sine.h){target="_blank"}
??? note "`dttimeseries_steady`&nbsp;&nbsp;&nbsp;Constant-value time series implementing the dttimeseries facade."
    Returns a fixed double value for any timestamp query, providing a simple
    baseline or placeholder in signal-processing pipelines that consume
    dttimeseries handles. Follows the standard create lifecycle and registers
    its vtable for model-number dispatch via the dttimeseries registry.

    [dtmc_base_library/include/dtmc_base/dttimeseries_steady.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dttimeseries_steady.h){target="_blank"}
??? note "`dtuart_helpers`&nbsp;&nbsp;&nbsp;UART parameter types with validation and string conversion."
    Defines enumerations for parity, data bits, stop bits, and flow control,
    each with an is_valid predicate and a to_string formatter. A unified
    dtuart_helper_config_t bundles all parameters alongside a baud rate; a
    validate function reports the first invalid field via dterr_t. Shared by
    UART-backed dtiox implementations across ESP-IDF, Zephyr, PicoSDK, and
    Linux backends.

    [dtmc_base_library/include/dtmc_base/dtuart_helpers.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/dtuart_helpers.h){target="_blank"}
??? note "`dtmc_base`&nbsp;&nbsp;&nbsp;Library flavor and version identification macros."
    Provides DTMC_BASE_VERSION string constants that identify the library build at runtime.
    Used by dtruntime and diagnostic routines to report the active library variant in environment tables and
    log output.

    This file is processed by tooling in the automated build system.
    It's important to maintain the structure and formatting of the version macros for compatibility with version parsing scripts.

    [dtmc_base_library/include/dtmc_base/version.h]({{ config.repo_url }}/dtmc_base_library/include/dtmc_base/version.h){target="_blank"}
