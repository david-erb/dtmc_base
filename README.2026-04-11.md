# dtmc_base

**dtmc_base** is a grab-bag of microprocessor-friendly building blocks: small algorithms, concurrency primitives, and lightweight networking/utilities that are designed to compile cleanly across **Zephyr**, **RTOS** environments, and **Linux**.

A large portion of the code is genuinely platform-independent C and aims to compile **without platform `#ifdef` conditionals**. Where hardware/OS integration is required, dtmc_base defines a small set of **platform API surfaces** as header-only interfaces (`.h` files). You implement those once per platform, and the rest of the library builds on top.

---

## Goals

- **Portable C** suitable for embedded firmware *and* host tooling.
- **Minimal conditional compilation**: prefer clear interfaces over scattered `#ifdef`s.
- **Testable**: keep most logic in platform-independent modules.
- **Practical**: focus on things you actually end up rewriting in every embedded project.

---

## Repository layout (typical)

> Exact folder names may differ — the intent is:
> **core logic** is separated from **platform glue**.

- `include/` – public headers
- `src/` – platform-independent implementations
- `platform/` – per-platform implementations of the API surfaces (Zephyr / FreeRTOS / Linux / etc.)
- `tests/` – unit tests for platform-independent logic (optional)
- `examples/` or `demos/` – small runnable samples (optional)
- `docs/` – project documentation (optional)

---

## The platform API surfaces

The following APIs are defined in dtmc_base as **interfaces only** (`.h` files).  
They must be implemented for each supported platform:

- `dtsemaphore` – semaphore / signaling primitive
- `dtbufferqueue` – queueing of buffers/messages between tasks/threads
- `dttasker` – task/thread creation and coordination
- `dtcpu` – CPU/clock/query utilities (timing, cores, etc.)
- `dtruntime` – runtime hooks (init, tick source, fatal handling, etc.)
- `dtgpiopin` – GPIO pin abstraction
- `dtnvblob` – non-volatile blob storage (settings/config persistence)
- `dtiox` – I/O abstraction layer (transport-agnostic IO surface)
- `dtlock` – mutex/lock primitive

Once these are provided, the rest of dtmc_base can run with the same higher-level behavior across targets.

---

## Supported platforms

dtmc_base is intended to be portable across:

- **Zephyr**
- **RTOS** targets (e.g. FreeRTOS-style environments)
- **Linux** (useful for simulation, tools, and reference implementations)

Platform support is provided by implementing the API surfaces listed above.

---

## Getting started

### 1) Pick or create a platform port
Create a platform implementation package (often in `platform/<name>/`) that provides the required APIs.

At minimum, your platform needs to supply implementations for:

- `dtsemaphore`, `dtlock`, `dttasker` (concurrency)
- `dtcpu`, `dtruntime` (timing/runtime hooks)

…and any hardware/storage interfaces you intend to use (`dtgpiopin`, `dtnvblob`, `dtiox`).

### 2) Build
Build system integration depends on your platform:

- **Zephyr**: integrate as a module/library in your West workspace.
- **RTOS**: add sources + includes to your application build.
- **Linux**: build as a static library and link into a test/demo program.

(See `docs/` for concrete build instructions per platform, if provided.)

---

## Design notes

- **Header-only API surfaces** keep the “porting contract” explicit.
- **Composition over conditionals**: platform behavior is injected behind interfaces.
- **Embedded-first constraints**: avoid heavyweight dependencies; keep APIs small.

---

## Documentation

- Start here: `docs/` (if present)
- Key concept: **platform API surfaces** + **portable implementations**

---

## License

Add your license here (e.g. MIT / BSD-2-Clause / Apache-2.0).

---

## Status

Early/active development. APIs may evolve as new targets and demos are added.
