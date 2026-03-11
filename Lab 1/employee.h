#pragma once
#include <vector>
#include <string>
#include <ostream>
using std::vector;
using std::string;
using std::ostream;
struct employee {
    int num;
    char name[10];
    double hours;
};

std::ostream& operator<<(std::ostream& os, const employee& e);

vector<employee> ReadEmployeesFromBinary(const string& filename);
bool WriteEmployeesToBinary(const string& filename, const vector<employee>& employees);
vector<string> ReadReportFile(const string& filename);