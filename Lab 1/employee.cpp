#include "employee.h"
#include <fstream>
#include <string>
using std::ifstream;
using std::ofstream;
using std::string;
using std::vector;

std::ostream & operator<<(std::ostream &os, const employee &e) {
    os.write(reinterpret_cast<const char*>(&e.num), sizeof(e.num));
    os.write(e.name, sizeof(e.name)); // Для массива char преобразование не нужно
    os.write(reinterpret_cast<const char*>(&e.hours), sizeof(e.hours));
    return os;
}

vector<employee> ReadEmployeesFromBinary(const string& filename) {
    vector<employee> result;
    ifstream fin(filename, std::ios::binary);
    if (!fin.is_open()) return result;
    employee emp;
    while (fin.read(reinterpret_cast<char*>(&emp), sizeof(emp))) {
        result.push_back(emp);
    }
    return result;
}

bool WriteEmployeesToBinary(const string& filename, const vector<employee>& employees) {
    ofstream fout(filename, std::ios::binary);
    if (!fout.is_open()) return false;
    for (const auto& emp : employees) {
        fout.write(reinterpret_cast<const char*>(&emp), sizeof(emp));
    }
    return true;
}

vector<string> ReadReportFile(const string& filename) {
    vector<string> lines;
    ifstream fin(filename);
    string line;
    while (std::getline(fin, line)) {
        lines.push_back(line);
    }
    return lines;
}
