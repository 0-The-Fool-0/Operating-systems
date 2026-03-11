#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include "employee.h"

using std::cout;
using std::cin;
using std::string;
using std::vector;

void PrintBinaryFile(const string& filename) {
    vector<employee> employees = ReadEmployeesFromBinary(filename);
    if (employees.empty()) {
        std::cerr << '\"' << filename << "\" file is empty\n";
        return;
    }
    cout << "File content:\n";
    for (const auto& emp : employees) {
        cout << emp.num << " " << emp.name << " " << emp.hours << std::endl;
    }
}

void PrintReport(const string& filename) {
    vector<string> lines = ReadReportFile(filename);
    if (lines.empty()) {
        std::cerr << "Can't open report file or file is empty.\n";
        return;
    }
    for (const auto& line : lines) {
        cout << line << std::endl;
    }
}

void launchCreator(const string& creator_file, const int& record_count) {
    string creatorCommand = string("Creator.exe") + ' ' + creator_file + ' ' + std::to_string(record_count);
    vector<char> cmd1(creatorCommand.begin(), creatorCommand.end());
    cmd1.push_back('\0');

    STARTUPINFOA si_creator;
    PROCESS_INFORMATION pi_creator;
    ZeroMemory(&si_creator, sizeof(si_creator));
    si_creator.cb = sizeof(si_creator);
    ZeroMemory(&pi_creator, sizeof(pi_creator));

    if (!CreateProcessA(NULL, cmd1.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si_creator, &pi_creator)) {
        std::cerr << "Failed to start Creator.exe. Error: " << GetLastError() << "\n";
        return;
    }
    WaitForSingleObject(pi_creator.hProcess, INFINITE);
    CloseHandle(pi_creator.hProcess);
    CloseHandle(pi_creator.hThread);

    PrintBinaryFile(creator_file);
}

void launchReporter(const string& creator_file, const string& report_file, const double& pay_per_hour) {
    string reporterCommand = string("Reporter.exe") + ' ' + creator_file + ' ' + report_file + ' ' + std::to_string(pay_per_hour);
    vector<char> cmd2(reporterCommand.begin(), reporterCommand.end());
    cmd2.push_back('\0');
    STARTUPINFOA si_reporter;
    PROCESS_INFORMATION pi_reporter;
    ZeroMemory(&si_reporter, sizeof(si_reporter));
    si_reporter.cb = sizeof(si_reporter);
    ZeroMemory(&pi_reporter, sizeof(pi_reporter));

    if (!CreateProcessA(NULL, cmd2.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si_reporter, &pi_reporter)) {
        std::cerr << "Failed to start Reporter.exe. Error: " << GetLastError() << "\n";
        return;
    }
    WaitForSingleObject(pi_reporter.hProcess, INFINITE);
    CloseHandle(pi_reporter.hProcess);
    CloseHandle(pi_reporter.hThread);

    PrintReport(report_file);
}

int main() {
    string creator_file, report_file;
    int record_count;
    double pay_per_hour;
    //Creator.exe
    cout << "Enter binary file name: ";
    cin >> creator_file;
    cout << "Enter number of records: ";
    cin >> record_count;
    launchCreator(creator_file, record_count);

    // Reporter.exe
    cout << "Enter report file name: ";
    cin >> report_file;
    cout << "Enter pay per hour: ";
    cin >> pay_per_hour;
    launchReporter(creator_file, report_file, pay_per_hour);

    return 0;
}
