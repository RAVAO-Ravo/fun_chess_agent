/**
 * @file test_util.hpp
 * @brief Fournir les assertions minimales du banc de tests interne.
 */

#pragma once

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

/**
 * @class TestSuite
 * @brief Compter les assertions et interrompre une suite au premier échec.
 */
class TestSuite {
public:
    /** @brief Vérifier une condition en conservant son emplacement source. */
    void require(bool condition, const std::string& message, const char* file, int line) {
        ++assertions_;
        if (!condition) {
            std::ostringstream out;
            out << file << ':' << line << ": " << message;
            throw std::runtime_error(out.str());
        }
    }

    /** @brief Vérifier l’égalité de deux valeurs potentiellement distinctes. */
    template <typename A, typename B>
    void requireEqual(const A& actual, const B& expected, const std::string& message, const char* file, int line) {
        ++assertions_;
        if (!(actual == expected)) {
            std::ostringstream out;
            out << file << ':' << line << ": " << message;
            throw std::runtime_error(out.str());
        }
    }

    int assertions() const {
        return assertions_;
    }

private:
    int assertions_ = 0;
};

#define REQUIRE(suite, condition) (suite).require((condition), #condition, __FILE__, __LINE__)
#define REQUIRE_EQ(suite, actual, expected) \
    (suite).requireEqual( \
        (actual), \
        (expected), \
        #actual " == " #expected, \
        __FILE__, \
        __LINE__)
