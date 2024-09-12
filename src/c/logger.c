#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include <unistd.h>
#include <stdarg.h>
 #include <libgen.h>

#define MAX_BUFFER_LOG 1024

void log_message(char *buffer, size_t buffer_size , LogLevel level, const char *message, int log_color, int log_time) {
    const char *formatted_str;
    formatted_str = format_log(level, message, log_color, log_time);
    strcpy(buffer, formatted_str);
}


const char* format_prompt(LogLevel level, const char *prompt, const char* message) {
    return "";
}

const char* format_log(LogLevel level, const char* message, int log_color, int log_time) {
    const char* level_str;
    const char* color_code;
    static char formatted_msg[MAX_BUFFER_LOG];  // Buffer for formatted message
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_str[26];

    switch (level) {
        case LOG_INFO:
            level_str = "INFO";
            color_code = COLOR_BLUE;  // Blue
            break;
        case LOG_WARNING:
            level_str = "WARNING";
            color_code = COLOR_YELLOW;  // Yellow
            break;
        case LOG_ERROR:
            level_str = "ERROR";
            color_code = COLOR_RED;  // Red
            break;
        default:
            level_str = "UNKNOWN";
            color_code = COLOR_RESET;  // Default (white/gray)
            break;
    }

    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    // Format the message with the log level and color for fprint or for file log
    if (log_color){
        snprintf(formatted_msg, sizeof(formatted_msg), "[%s][%s] %s\n", time_str, level_str, message);
    }else{
        if(log_time){
            snprintf(formatted_msg, sizeof(formatted_msg), "%s[%s][%s]%s %s\n", color_code, time_str, level_str, COLOR_RESET, message);
        }else{
            snprintf(formatted_msg, sizeof(formatted_msg), "%s[%s]%s %s\n", color_code, level_str, COLOR_RESET, message);
        }
    }
    return formatted_msg;
}


// Function to log a message to a file
void log_file_message(const char *path, LogLevel level, const char *message) {
    char buffer[MAX_BUFFER_LOG];
    size_t buffer_size = MAX_BUFFER_LOG;

    char *path_copy = (char *)malloc(strlen(path) + 1);
    path_copy[sizeof(path_copy)-1] = '\0';
    char * dir = dirname(path_copy);
    if (ensure_log_directory(dir) != 0) {
        free(path_copy);
        return;  // If the directory can't be created, exit the function
    }
    free(path_copy);

    // Open the file in append mode
    FILE *file = fopen(path, "a");
    if (!file) {
        perror("Error opening log file");
        return;
    }
    log_message(buffer, buffer_size, level, message, 1, 1);

    // Append the message to the file
    fputs(buffer, file);
    fclose(file);
}


void log_print_message(LogLevel level, const char *message) {
    char buffer[MAX_BUFFER_LOG];
    log_message(buffer, sizeof(buffer), level, message, 1, 1);
    printf(buffer);
}

void log_print_prompt(LogLevel level, const char *prompt, const char *message){
    printf("*%s* %s\n", prompt, message);
}

int ensure_log_directory(char *dir) {
    // Check if the directory exists using access()
    if (access(dir, F_OK) == 0) {
        // Directory exists
        return 0;
    }

    // Directory doesn't exist, try to create it with mkdir()
    if (mkdir(dir, 0755) != 0) {
        // Failed to create the directory
        perror("mkdir");
        return -1;
    }

    return 0;
}


// Function to log a formatted message
void log_file_formatted_message(const char *path, LogLevel level, const char *format, ...) {
    char log_msg[MAX_BUFFER_LOG];
    va_list args;
    
    // Start extracting arguments
    va_start(args, format);
    
    // Format the log message
    vsnprintf(log_msg, sizeof(log_msg), format, args);
    
    // End extracting arguments
    va_end(args);
    
    // Call the log_file_message function with the formatted message
    log_file_message(path, level, log_msg);
}



void log_print_file_message(const char *path, LogLevel level, const char *message) {
    printf(message);
    log_file_message(path, level, message);
}

void log_server_message(const char *path, LogLevel level, const char* format, ...){
    char buffer[MAX_BUFFER_LOG];
    size_t buffer_size = MAX_BUFFER_LOG;
    char log_msg[MAX_BUFFER_LOG];
    va_list args;
    
    // Start extracting arguments
    va_start(args, format);
    
    // Format the log message
    vsnprintf(log_msg, sizeof(log_msg), format, args);
    
    // End extracting arguments
    va_end(args);
    
    // Call the log_file_message function with the formatted message
    log_file_message(path, level, log_msg);


    log_message(buffer, buffer_size, level, log_msg, 1, 0);
    log_print_message(level, log_msg);
}
