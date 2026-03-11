#include <windows.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "employee.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: Reporter <bin_file> <report_file> <pay_per_hour>\n";
        return 1;
    }
    const char* bin_file = argv[1];
    const char* rep_file = argv[2];
    double pay = atof(argv[3]);

    std::ifstream fin(bin_file, std::ios::binary);
    if (!fin.is_open()) {
        std::cerr << "Can't open binary file.\n";
        return 2;
    }
    std::ofstream fout(rep_file);
    if (!fout) {
        std::cerr << "Can't open report file.\n";
        return 3;
    }

    fout << "Report for \"" << bin_file << "\" file\n";
    fout << "----------------------------------------------------\n";
    fout << "Num\tName\tHours\tSalary\n";

    employee emp;
    while (fin.read(reinterpret_cast<char*>(&emp), sizeof(emp))) {
        double salary = emp.hours * pay;
        fout << emp.num << "\t" << emp.name << "\t"
            << emp.hours << "\t" << std::fixed << std::setprecision(2) << salary << "\n";
    }
    fin.close();
    fout.close();
    return 0;
}