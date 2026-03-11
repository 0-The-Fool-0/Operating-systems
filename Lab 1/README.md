# Lab 1 — Employee Database & Reporting (C++98)

Design a system to manage employee records and generate payroll reports based on binary data files.

---

## Objective
Create two small console programs that work with a binary file of employee records:
- Creator: creates and populates the binary file.
- Reporter: reads the binary file and produces a formatted salary report.

---

## Tasks
- Implement `Creator.exe` to create a binary file of employee records.
- Implement `Reporter.exe` to read the binary file and generate a formatted report, calculating the salary for each employee.
- Provide a main entry point (or solution) that orchestrates creator and reporter modules where applicable.
- Add error handling for missing files, invalid paths, malformed input, and report generation failures.
- Add unit tests for core functionalities: file creation, reading, and error cases (if testing framework is available).

---

## Key Files
- `Lab 1/Main.cpp`
- `Lab 1/Creator.cpp`
- `Lab 1/Reporter.cpp`
- `Lab 1/employee.h`, `Lab 1/employee.cpp`
- `Lab 1/test.cpp`

---

## Build (CMake, Windows)
```bash
cmake -S . -B build
cmake --build build --config Release
```

Toolchain/standard:
- C++98
- MSVC/MinGW on Windows (choose your environment)

---

## Run
- Creator: generates the binary employee file.
- Reporter: reads the binary file and prints the report.

Example sequence (arguments/paths may vary by your implementation):
```bash
# 1) Create data file
./build/Creator.exe

# 2) Generate report
./build/Reporter.exe
```

---

## Testing
```bash
ctest --test-dir build -V
```