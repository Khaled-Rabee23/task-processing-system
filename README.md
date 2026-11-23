# Multithreaded Task Processing System

A C project that simulates a task-processing pipeline using POSIX threads (`pthreads`), FIFO queues, mutexes, and C11 atomic variables.

## Overview

The program reads task definitions from `data/tasks.txt`. A **receptor** thread loads each task into a shared waiting queue. A **dispatcher** sends tasks to the queue of the next processing unit. Five worker threads process their respective unit queues and forward tasks through the sequence specified for each task.

Each task records its entry time. When its processing path completes, the program reports the elapsed time and final value.

## Architecture

- **Receptor thread** — reads task records and adds them to the waiting queue.
- **Dispatcher thread** — removes tasks from the waiting queue and routes them to the first requested unit.
- **Five worker threads** — one worker for each processing unit (`0`–`4`).
- **Waiting queue** — shared FIFO queue between the receptor and dispatcher.
- **Unit queues** — one FIFO queue per processing unit.
- **Synchronization** — each queue has a `pthread_mutex_t`; shared completion counters use C11 atomics.

## Processing units

| Unit | Operation |
|---|---|
| 0 | Add 7, then take modulo 10,000 |
| 1 | Multiply by 2, then take modulo 10,000 |
| 2 | Raise the value to the fifth power modulo 10,000 |
| 3 | Subtract the value from 19 |
| 4 | Report elapsed time and the current value |

The operation is applied when a task visits a unit. A task's route is defined by the unit IDs in its input record.

## Project structure

```text
task-processing-system/
├── data/
│   └── tasks.txt       # Sample task input
├── src/
│   └── main.c          # Application source code
├── .gitignore
├── Makefile
└── README.md
```

## Requirements

- Linux or another POSIX-compatible environment
- GCC or Clang
- POSIX threads support

On Debian/Ubuntu, install the compiler toolchain with:

```bash
sudo apt update
sudo apt install build-essential
```

## Build

From the project root:

```bash
make
```

This creates the executable at `build/task_processor`.

## Run

```bash
make run
```

Or run it directly:

```bash
./build/task_processor
```

The program reads `data/tasks.txt` using a path relative to the project root. Run it from the project root directory.

## Input format

Each task is represented by:

```text
task_id initial_value number_of_units unit_id_1 unit_id_2 ... unit_id_n
```

Example:

```text
10 25 3 0 1 4
```

This describes task `10`, starting with value `25`, visiting three units in this order: `0`, `1`, and `4`.

Unit IDs must be in the range `0` through `4`. The number of unit IDs must match `number_of_units`.

## Clean build files

```bash
make clean
```

## Notes

- The executable supplied with the original files is not included in the repository. Build it from source with `make` so the project is reproducible on other machines.
- `SS_final_project.c.txt` was a duplicate text copy of the C source and has not been included.
- The original C implementation has been retained with only the input-file path and identifying header adjusted for this directory layout.
- Review the implementation and input validation before using this as a production system. This repository is organized for sharing and reproducible building; it is not a claim that the original concurrency implementation has undergone a full correctness or security audit.
