#pragma once
#include <string>

class MyClass {
public:
    MyClass(int value) : value_(value) {}
    int GetValue() const { return value_; }
    void SetValue(int value) { value_ = value; }

private:
    int value_;

public:
    std::string name;
};