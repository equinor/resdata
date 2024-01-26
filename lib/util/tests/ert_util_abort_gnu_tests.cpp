/*
 * File:   ert_util_abort_gnu_tests.c
 * Author: kflik
 *
 * Created on August 16, 2013, 9:41 AM
 */

#include <unistd.h>
#include <stdlib.h>
#include <ert/util/util.hpp>
#include <ert/util/test_util.hpp>

void test_assert_util_abort(const char *function_name, void(void *), void *arg);

void call_util_abort(void *arg) {
    util_abort("%s: I am calling util_abort - should be intercepted\n",
               __func__);
}

void test_intercept() {
    test_assert_util_abort("call_util_abort", call_util_abort, nullptr);
}

static bool test_abort_handler_called{};

void test_handler_call(void *) { util_abort("foo bar!!!"); }

void test_handler() {
    util_set_abort_handler([](const char *filename, int lineno,
                              const char *function, const char *message,
                              const char *backtrace) {
        test_abort_handler_called = true;
        test_assert_string_equal(filename, __FILE__);
        test_assert_true(lineno);
        test_assert_string_equal(function, "test_handler_call");
        test_assert_string_equal(message, "foo bar!!!");
        test_assert_true(backtrace);
    });

    test_assert_util_abort("test_handler_call", test_handler_call, nullptr);
    test_assert_true(test_abort_handler_called);
}

int main(int argc, char **argv) {
    test_intercept();
    test_handler();
    exit(0);
}
