#ifndef AETHER_FS_H
#define AETHER_FS_H

#include "../string/aether_string.h"

// File operations
typedef struct {
    void* handle;
    int is_open;
    const char* path;
} AetherFile;

AetherFile* aether_file_open(const char* path, const char* mode);
AetherString* aether_file_read_all(AetherFile* file);
int aether_file_write(AetherFile* file, const char* data, size_t length);
int aether_file_close(AetherFile* file);
int aether_file_exists(const char* path);
int aether_file_delete(const char* path);
size_t aether_file_size(const char* path);

// Directory operations
int aether_dir_exists(const char* path);
int aether_dir_create(const char* path);
int aether_dir_delete(const char* path);

// Path operations
AetherString* aether_path_join(const char* path1, const char* path2);
AetherString* aether_path_dirname(const char* path);
AetherString* aether_path_basename(const char* path);
AetherString* aether_path_extension(const char* path);
int aether_path_is_absolute(const char* path);

// Directory listing
typedef struct {
    char** entries;
    int count;
} AetherDirList;

AetherDirList* aether_dir_list(const char* path);
void aether_dir_list_free(AetherDirList* list);

#endif // AETHER_FS_H

