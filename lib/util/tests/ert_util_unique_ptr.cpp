#include <ert/util/ert_unique_ptr.hpp>
#include <ert/util/stringlist.hpp>

void test_stringlist() {
    ERT::ert_unique_ptr<stringlist_type, stringlist_free> stringlist(
        stringlist_alloc_new());
    stringlist_append_copy(stringlist.get(), "Hello");
}

int main(int argc, char **argv) { test_stringlist(); }
