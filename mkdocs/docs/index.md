# dtmc_base

Portable C building blocks for embedded firmware and host tooling — concurrency primitives, networking abstractions, and small algorithms — designed to compile cleanly across **Zephyr**, **RTOS**, and **Linux** without scattered `#ifdef`s.

_Current release: v{{ dtmc_base_version }}_

`dtmc_base` is part of the **[Embedded Applications Lab](https://david-erb.github.io/embedded)** — a set of working applications based on a set of reusable libraries across MCU, Linux, and RTOS targets.

---

## What's inside

| Area | Examples |
|------|---------|
| Concurrency | `dttasker`, `dtsemaphore`, `dtlock`, `dtbufferqueue` |
| Networking / I/O | `dtnetportal`, `dtiox`, `dtframer`, `dtnetbridge` |
| Hardware facades | `dtgpiopin`, `dtadc`, `dtdisplay`, `dtdotstar` |
| Storage | `dtnvblob`, `dtradioconfig` |
| Diagnostics | `dtruntime`, `dttasker_registry`, `dtbusywork` |

---

## How portability works

Most of the library is plain, platform-independent C. Where OS or hardware integration is unavoidable, dtmc_base defines a small **porting contract** — nine header-only interfaces you implement once per platform:

`dtsemaphore` &nbsp;`dtlock` &nbsp;`dttasker` &nbsp;`dtcpu` &nbsp;`dtruntime` &nbsp;`dtgpiopin` &nbsp;`dtiox` &nbsp;`dtnvblob` &nbsp;`dtbufferqueue`

Everything else builds on top of those without knowing the underlying OS.

---

## Supported targets

Zephyr &nbsp;·&nbsp; FreeRTOS-style RTOS &nbsp;·&nbsp; Linux (reference port, testing, tooling)

---

## Docs

See the [dtmc_base documentation site](https://david-erb.github.io/dtmc_base/) for the full API reference and porting guide.
