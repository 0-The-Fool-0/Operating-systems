#include <windows.h>
#include <iostream>
#include <fstream>
#include "employee.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: Creator <binary_filename> <record_count>\n";
        return 1;
    }

    const char* filename = argv[1];
    int n = atoi(argv[2]);
    std::ofstream fout(filename, std::ios::binary);
    if (!fout.is_open()) {
        std::cerr << "Can't open file for writing.\n";
        return 2;
    }

    for (int i = 0; i < n; ++i) {
        employee emp{};
        std::cout << "Enter num, name, hours for employee number" << (i + 1) << ":\n";
        fout << emp;
        fout.write(reinterpret_cast<char*>(&emp), sizeof(emp));
    }
    fout.close();
    return 0;
}