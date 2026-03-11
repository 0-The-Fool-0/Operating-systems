#include <windows.h>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "employee.h"

using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;

int generate_report(const string& bin_file, const string& rep_file, double pay) {
    ifstream fin(bin_file, std::ios::binary);
    if (!fin.is_open()) {
        return 2;
    }
    ofstream fout(rep_file);
    if (!fout) {
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

TEST(ReporterTest, GeneratesReportFile) {
    const char* test_bin = "test_employee.bin";
    const char* test_report = "test_report.txt";
    employee emp = { 1, "John", 40.0 };
    {
        ofstream fout(test_bin, std::ios::binary);
        fout.write(reinterpret_cast<const char*>(&emp), sizeof(emp));
    }

    int result = generate_report(test_bin, test_report, 10.0);

    EXPECT_EQ(result, 0);

    ifstream fin(test_report);
    string content, line;
    while (std::getline(fin, line)) {
        content += line + "\n";
    }
    EXPECT_NE(content.find("John"), string::npos);
    EXPECT_NE(content.find("400.00"), string::npos);

    remove(test_bin);
    remove(test_report);
}

TEST(ReporterTest, ReturnsErrorWhenBinaryFileMissing) {
    const char* missing_bin = "nonexistent.bin";
    const char* test_report = "should_not_create.txt";

    int result = generate_report(missing_bin, test_report, 10.0);

    EXPECT_EQ(result, 2);

    std::ifstream fin(test_report);
    EXPECT_FALSE(fin.is_open());

    remove(test_report);
}

TEST(EmployeeTest, WriteEmployeesToBinaryReturnsFalseOnError) {
    vector<employee> employees = { {1, "Test", 10.0} };
    string bad_path = "/invalid_path/test_employees.bin";
    EXPECT_FALSE(WriteEmployeesToBinary(bad_path, employees));
}

TEST(EmployeeTest, ReadEmployeesFromBinaryReturnsEmptyOnMissingFile) {
    string missing_file = "nonexistent.bin";
    vector<employee> result = ReadEmployeesFromBinary(missing_file);
    EXPECT_TRUE(result.empty());
}

TEST(EmployeeTest, ReadReportFileReadsLinesCorrectly) {
    const string report_file = "test_report.txt";
    ofstream fout(report_file);
    fout << "Line one\n";
    fout << "Line two\n";
    fout.close();

    vector<string> lines = ReadReportFile(report_file);
    ASSERT_EQ(lines.size(), 2);
    EXPECT_EQ(lines[0], "Line one");
    EXPECT_EQ(lines[1], "Line two");

    remove(report_file.c_str());
}

TEST(EmployeeTest, ReadReportFileReturnsEmptyOnMissingFile) {
    string missing_file = "nonexistent_report.txt";
    vector<string> lines = ReadReportFile(missing_file);
    EXPECT_TRUE(lines.empty());
}