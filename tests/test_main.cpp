/**
 * @file test_main.cpp
 * @brief Exécuter toutes les suites de tests avec un bilan commun.
 */

#include "test_util.hpp"

#include <exception>
#include <iostream>

void test_board(TestSuite& suite);
void test_moves(TestSuite& suite);
void test_rules(TestSuite& suite);
void test_fen(TestSuite& suite);
void test_ai(TestSuite& suite);
void test_genetic(TestSuite& suite);
void test_opening_book(TestSuite& suite);
void test_perft(TestSuite& suite);
void test_protocol(TestSuite& suite);

int main() {
    TestSuite suite;
    try {
        test_board(suite);
        test_moves(suite);
        test_rules(suite);
        test_fen(suite);
        test_ai(suite);
        test_genetic(suite);
        test_opening_book(suite);
        test_perft(suite);
        test_protocol(suite);
    } catch (const std::exception& error) {
        std::cerr << "Test failed: " << error.what() << '\n';
        return 1;
    }

    std::cout << "All tests passed (" << suite.assertions() << " assertions)\n";
    return 0;
}
