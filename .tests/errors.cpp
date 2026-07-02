#include <iostream>

#include <errors.hpp>
#include <exception>

int basic_test() {

    // No error
    auto noError = yst::CustomError{};
    if (noError) {
        std::cerr << "Test failed: default error should evaluate to false" << std::endl;
        return 1;
    }

    // Real error
    auto error = yst::CustomError(10, "Some helpful message");
    if (!error) {
        std::cerr << "Test failed: error should evaluate to true" << std::endl;
        return 2;
    }

    if (error.str() != "[10] Error: Some helpful message") {
        std::cerr << "Test failed: incorrect str() output" << std::endl;
        return 3;
    }

    // Static helpter
    auto unknown = yst::CustomError::Unknown();
    if (unknown.str() != "[-1] Error: Something went wrong") {
        return 4;
    }

    return 0;
}

int main(){
    try {
        return basic_test();
    } catch (std::exception& ex) {
        std::cerr << "[exception]: " << ex.what() << std::endl;
        return -1;
    }
}

