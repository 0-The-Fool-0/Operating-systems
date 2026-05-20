#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "Employee.h"

namespace lab5 {

class EmployeeRepository {
public:
    explicit EmployeeRepository(std::string file_name);

    void Initialize(const std::vector<employee>& items);
    std::vector<employee> ReadAll() const;
    bool ReadById(int id, employee& out) const;
    bool Update(const employee& value);

private:
    std::string file_name_;
    mutable std::mutex file_mutex_;
};

}  // namespace lab5
