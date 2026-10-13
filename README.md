# Operating Systems — C++ Coursework

[Русская версия](README.ru.md)

A collection of systems-programming labs implemented as separate C++ projects. The repository progresses from file-based records and multi-threaded calculations to synchronisation, producer/consumer communication, and a client/server program for employee records.

## Labs

| Directory | Topics represented |
|---|---|
| `Lab 1/` | Binary employee records, separate Creator/Main/Reporter processes, report generation, and GoogleTest checks. |
| `Lab2 CMake C++17/` | Data-processing helpers for minimum/maximum and average calculations, a thread-oriented implementation, and tests. |
| `Lab3 Cmake C++17/` | Shared-array/marker logic and synchronisation-oriented tests. |
| `Lab4/` | File-based queue helpers with sender/receiver programs and queue tests. |
| `Lab5/` | Client/server components, employee-record repository, record-lock manager, and tests. |

Each lab has its own CMake configuration; there is no single combined executable for the entire repository.

## Build and test

Use a C++17-capable compiler and CMake. For a specific lab, run CMake from that lab's directory. For example:

```powershell
cmake -S "Lab2 CMake C++17" -B "Lab2 CMake C++17/build"
cmake --build "Lab2 CMake C++17/build" --config Debug
ctest --test-dir "Lab2 CMake C++17/build" -C Debug --output-on-failure
```

This example shows the intended workflow, not a verified build result. Check each lab's CMake configuration for compiler, platform, and dependency requirements. Some parts use Windows-oriented APIs, so portability may vary.

## Scope

These are separate academic exercises around operating-system concepts: processes, threads, synchronisation, file I/O, inter-process communication, and concurrent access to records. They should not be treated as a production library or service.
