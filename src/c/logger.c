#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include <unistd.h>
#include <stdarg.h>

#define MAX_BUFFER_LOG 1024

void log_message(char *buffer, size_t buffer_size , LogLevel level, const char *message, int file_log) {
    const char *formatted_str;
    formatted_str = format_log(level, message, file_log) ;
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    snprintf(buffer, buffer_size, "[%s] %s\n", time_str, formatted_str);
}

const char* format_log(LogLevel level, const char* message, int file_log) {
    const char* level_str;
    const char* color_code;
    static char formatted_msg[1024];  // Buffer for formatted message

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


    // Format the message with the log level and color for fprint or for file log
    if (!file_log){
        snprintf(formatted_msg, sizeof(formatted_msg), "%s[%s] %s%s", color_code, level_str, COLOR_RESET, message);
        return formatted_msg;
    }
    snprintf(formatted_msg, sizeof(formatted_msg), "[%s] %s", level_str, message);
    return formatted_msg;
}


// Function to log a message to a file
void log_file_message(LogLevel level, const char *message) {
    char buffer[MAX_BUFFER_LOG];
    size_t buffer_size = MAX_BUFFER_LOG;

    // Ensure directory exists
    if (ensure_log_directory("./log") != 0) {
        return;  // If the directory can't be created, exit the function
    }

    // Open the file in append mode
    FILE *file = fopen("./log/client_logger", "a");
    if (!file) {
        perror("Error opening log file");
        return;
    }
    log_message(buffer, buffer_size, level, message, 1);

    // Append the message to the file
    fputs(buffer, file);
    fclose(file);
}


void log_print_message(LogLevel level, const char *message) {
    char buffer[MAX_BUFFER_LOG];
    log_message(buffer, sizeof(buffer), level, message, 0);
    printf(buffer);
}

int ensure_log_directory(const char *dir) {
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
void log_formatted_message(LogLevel level, const char *format, ...) {
    char log_msg[MAX_BUFFER_LOG];
    va_list args;
    
    // Start extracting arguments
    va_start(args, format);
    
    // Format the log message
    vsnprintf(log_msg, sizeof(log_msg), format, args);
    
    // End extracting arguments
    va_end(args);
    
    // Call the log_file_message function with the formatted message
    log_file_message(level, log_msg);
}


//int main() {
//    // Example usage
//    const char *json_str = "{\"key\":\"value\"}";
//    log_formatted_message(LOG_ERROR, "JSON sent to the server:\n%s\n", json_str);
//    return 0;
//}