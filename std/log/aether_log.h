#ifndef AETHER_LOG_H
#define AETHER_LOG_H

#include <stdarg.h>
#include <stdio.h>

// Log levels
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4
} AetherLogLevel;

// Log configuration
typedef struct {
    AetherLogLevel min_level;
    FILE* output_file;
    int use_colors;
    int show_timestamps;
    int show_source_location;
    const char* format_string;  // e.g., "[{time}] {level}: {message}"
} AetherLogConfig;

// Initialize logging system
void aether_log_init(const char* filename, AetherLogLevel min_level);
void aether_log_init_with_config(AetherLogConfig* config);
void aether_log_shutdown();

// Core logging functions
void aether_log(AetherLogLevel level, const char* fmt, ...);
void aether_log_with_location(AetherLogLevel level, const char* file, int line, 
                              const char* func, const char* fmt, ...);

// Convenience macros
#define LOG_DEBUG(...)  aether_log(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)   aether_log(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARN(...)   aether_log(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERROR(...)  aether_log(LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_FATAL(...)  aether_log(LOG_LEVEL_FATAL, __VA_ARGS__)

// With source location
#define LOG_DEBUG_LOC(...)  aether_log_with_location(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO_LOC(...)   aether_log_with_location(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN_LOC(...)   aether_log_with_location(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR_LOC(...)  aether_log_with_location(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

// Configuration
void aether_log_set_level(AetherLogLevel level);
void aether_log_set_colors(int enabled);
void aether_log_set_timestamps(int enabled);
void aether_log_set_format(const char* format);

// Statistics
typedef struct {
    size_t debug_count;
    size_t info_count;
    size_t warn_count;
    size_t error_count;
    size_t fatal_count;
} AetherLogStats;

AetherLogStats aether_log_get_stats();
void aether_log_print_stats();

#endif // AETHER_LOG_H

