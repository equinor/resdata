#include <stdio.h>

#include <vector>
#include <algorithm>

template <class T> int vector_util_index(const std::vector<T> &vec, T value) {

    int index;
    auto iter = find(vec.begin(), vec.end(), value);
    if (iter == vec.end())
        index = -1;
    else
        index = iter - vec.begin();
    return index;
}

template <class T>
void vector_util_fprintf(const std::vector<T> &vec, FILE *stream,
                         const char *name, const char *fmt) {
    size_t i;
    if (name != NULL)
        fprintf(stream, "%s = [", name);
    else
        fprintf(stream, "[");

    for (i = 0; i < vec.size(); i++) {
        fprintf(stream, fmt, vec[i]);
        if (i < (vec.size() - 1))
            fprintf(stream, ", ");
    }

    fprintf(stream, "]\n");
}
