#ifndef ERT_PATH_STACK_H
#define ERT_PATH_STACK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct path_stack_struct path_stack_type;

path_stack_type *path_stack_alloc();
void path_stack_pop(path_stack_type *path_stack);
void path_stack_push_cwd(path_stack_type *path_stack);
bool path_stack_push(path_stack_type *path_stack, const char *path);
void path_stack_free(path_stack_type *path_stack);
int path_stack_size(const path_stack_type *path_stack);

#ifdef __cplusplus
}
#endif

#endif
