#include <stdlib.h>

#include <vector>
#include <string>

#include <resdata/rd_sum_vector.hpp>
#include <resdata/rd_sum.hpp>
#include <resdata/rd_smspec.hpp>

#include <ert/util/util.hpp>
#include <ert/util/vector.hpp>
#include <ert/util/type_macros.hpp>

#define RD_SUM_VECTOR_TYPE_ID 8768778

struct rd_sum_vector_struct {
    UTIL_TYPE_ID_DECLARATION;
    std::vector<int> node_index_list;
    std::vector<bool> is_rate_list;
    std::vector<std::string> key_list;
    const rd_sum_type *rd_sum;
};

void rd_sum_vector_free(rd_sum_vector_type *rd_sum_vector) {
    delete rd_sum_vector;
}

UTIL_IS_INSTANCE_FUNCTION(rd_sum_vector, RD_SUM_VECTOR_TYPE_ID)

static void rd_sum_vector_add_node(rd_sum_vector_type *vector,
                                   const rd::smspec_node *node,
                                   const char *key) {
    int params_index = smspec_node_get_params_index(node);
    bool is_rate_key = smspec_node_is_rate(node);

    vector->node_index_list.push_back(params_index);
    vector->is_rate_list.push_back(is_rate_key);
    vector->key_list.push_back(key);
}

rd_sum_vector_type *rd_sum_vector_alloc(const rd_sum_type *rd_sum,
                                        bool add_keywords) {
    rd_sum_vector_type *rd_sum_vector = new rd_sum_vector_type();
    UTIL_TYPE_ID_INIT(rd_sum_vector, RD_SUM_VECTOR_TYPE_ID);
    rd_sum_vector->rd_sum = rd_sum;
    if (add_keywords) {
        const rd_smspec_type *smspec = rd_sum_get_smspec(rd_sum);
        for (int i = 0; i < rd_smspec_num_nodes(smspec); i++) {
            const rd::smspec_node &node =
                rd_smspec_iget_node_w_node_index(smspec, i);
            const char *key = smspec_node_get_gen_key1(&node);
            /*
          The TIME keyword is special case handled to not be included; that is
          to match the same special casing in the key matching function.
        */
            if (!util_string_equal(key, "TIME"))
                rd_sum_vector_add_node(rd_sum_vector, &node, key);
        }
    }
    return rd_sum_vector;
}

static void rd_sum_vector_add_invalid_key(rd_sum_vector_type *vector,
                                          const char *key) {
    vector->node_index_list.push_back(-1);
    vector->is_rate_list.push_back(false);
    vector->key_list.push_back(key);
}

/*
  This function will allocate a keyword vector for the keys in the @rd_sum
  argument passed in, it will contain all the same keys as in the input argument
  @src_vector. If the @src_vector contains keys which are not present in
  @rd_sum an entry marked as *invalid* will be added. The whole point about
  this function is to ensure that calls to:

       rd_sum_fwrite_interp_csv_line( )

  will result in nicely aligned output even if the different summary cases do
  not have the exact same keys.
*/

rd_sum_vector_type *
rd_sum_vector_alloc_layout_copy(const rd_sum_vector_type *src_vector,
                                const rd_sum_type *rd_sum) {
    rd_sum_vector_type *new_vector = rd_sum_vector_alloc(rd_sum, false);
    for (size_t i = 0; i < src_vector->key_list.size(); i++) {
        const char *key = src_vector->key_list[i].c_str();
        if (rd_sum_has_general_var(rd_sum, key))
            rd_sum_vector_add_key(new_vector, key);
        else
            rd_sum_vector_add_invalid_key(new_vector, key);
    }
    return new_vector;
}

bool rd_sum_vector_add_key(rd_sum_vector_type *rd_sum_vector, const char *key) {
    if (rd_sum_has_general_var(rd_sum_vector->rd_sum, key)) {
        const rd::smspec_node *node =
            rd_sum_get_general_var_node(rd_sum_vector->rd_sum, key);
        rd_sum_vector_add_node(rd_sum_vector, node, key);
        return true;
    } else
        return false;
}

void rd_sum_vector_add_keys(rd_sum_vector_type *rd_sum_vector,
                            const char *pattern) {
    stringlist_type *keylist =
        rd_sum_alloc_matching_general_var_list(rd_sum_vector->rd_sum, pattern);

    int num_keywords = stringlist_get_size(keylist);
    int i;
    for (i = 0; i < num_keywords; i++) {
        const char *key = stringlist_iget(keylist, i);
        const rd::smspec_node *node =
            rd_sum_get_general_var_node(rd_sum_vector->rd_sum, key);
        rd_sum_vector_add_node(rd_sum_vector, node, key);
    }
    stringlist_free(keylist);
}

int rd_sum_vector_get_size(const rd_sum_vector_type *rd_sum_vector) {
    return rd_sum_vector->node_index_list.size();
}

bool rd_sum_vector_iget_is_rate(const rd_sum_vector_type *rd_sum_vector,
                                int index) {
    return rd_sum_vector->is_rate_list[index];
}

bool rd_sum_vector_iget_valid(const rd_sum_vector_type *rd_sum_vector,
                              int index) {
    return (rd_sum_vector->node_index_list[index] >= 0);
}

int rd_sum_vector_iget_param_index(const rd_sum_vector_type *rd_sum_vector,
                                   int index) {
    return rd_sum_vector->node_index_list[index];
}

const char *rd_sum_vector_iget_key(const rd_sum_vector_type *rd_sum_vector,
                                   int index) {
    return rd_sum_vector->key_list[index].c_str();
}
