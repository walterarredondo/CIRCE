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
#define MAX_BUFFER_PROMPT 256

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
        case LOG_SUCCESS:
            level_str = "SUCCESS";
            color_code = COLOR_GREEN;  // Blue
            break;
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
        if(log_time){
            snprintf(formatted_msg, sizeof(formatted_msg), "%s[%s][%s]%s %s\n", color_code, time_str, level_str, COLOR_RESET, message);
        }else{
            snprintf(formatted_msg, sizeof(formatted_msg), "%s[%s]%s %s\n", color_code, level_str, COLOR_RESET, message);
        }
    }else{
        snprintf(formatted_msg, sizeof(formatted_msg), "[%s][%s] %s\n", time_str, level_str, message);
    }
    return formatted_msg;
}

const char* format_user_message(const char* username, const char* message) {
    static char formatted_msg[MAX_BUFFER_LOG];  // Buffer for formatted message
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char time_str[26];

    // Set the color for the username
    const char* username_color = COLOR_BLUE;  // Default color for username
    const char* reset_color = COLOR_RESET;  // Reset after coloring

    // Get the current time in the format HH:MM:SS
    strftime(time_str, 26, "%H:%M", tm_info);

    // Format the message
    snprintf(formatted_msg, sizeof(formatted_msg), "[%s] %s%s%s : %s\n", time_str, username_color, username, reset_color, message);
    return formatted_msg;
}



// Function to log a message to a file
void log_file_message(const char *path, LogLevel level, const char *message) {
    char buffer[MAX_BUFFER_LOG];
    size_t buffer_size = MAX_BUFFER_LOG;

    char *path_copy = (char *)malloc(strlen(path) + 1);
    strcpy(path_copy, path);
    path_copy[strlen(path)] = '\0';
    char * dir = dirname(path_copy);
    if (!ensure_log_directory(dir) != 0) {
        free(path_copy);
        return;  // If the directory can't be created, exit the function
    }

    // Open the file in append mode
    FILE *file = fopen(path, "a");
    if (!file) {
        perror("Error opening log file");
        return;
    }
    log_message(buffer, buffer_size, level, message, 0, 1);

    // Append the message to the file
    free(path_copy);
    fputs(buffer, file);
    fclose(file);
}


void log_print_message(LogLevel level, const char *message) {
    char buffer[MAX_BUFFER_LOG] = "";
    log_message(buffer, sizeof(buffer), level, message, 1, 1);
    printf("%s",buffer);
}

void log_print_prompt(LogLevel level, const char *format, ...){
    char prompt[MAX_BUFFER_PROMPT];
    va_list args;
    va_start(args, format);
    vsnprintf(prompt, sizeof(prompt), format, args);
    va_end(args);

    printf("> %s%s%s\n> ", COLOR_MAGENTA, prompt, COLOR_RESET);
}

void print_msg(char *user, char* msg){
    const char *str = format_user_message(user, msg);
    printf("%s> ", str);
}

void print_my_msg(char* msg){
    print_msg("Me", msg);
}

int ensure_log_directory(char *dir) {
    struct stat statbuf;

    // Check if the path exists
    if (stat(dir, &statbuf) == 0) {
        // Check if the path is a directory
        if (S_ISDIR(statbuf.st_mode)) {
            // Path is a directory
            return 1;
        } else {
            // Path exists but is not a directory
            fprintf(stderr, "Path exists but is not a directory: %s\n", dir);
            return 0;
        }
    }

    // Path doesn't exist, try to create it with mkdir()
    if (mkdir(dir, 0755) != 0) {
        // Failed to create the directory
        perror("mkdir");
        return 0;
    }

    return 1;
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
    printf("%s\n",message);
    log_file_message(path, level, message);
}

void log_server_message(const char *path, LogLevel level, const char* format, ...){
    char buffer[MAX_BUFFER_LOG];
    size_t buffer_size = MAX_BUFFER_LOG;
    char log_msg[MAX_BUFFER_LOG];
 
    va_list args;
    va_start(args, format);
    vsnprintf(log_msg, sizeof(log_msg), format, args);
    va_end(args);
    
    // Write in the log file 
    log_file_message(path, level, log_msg);

    //print in the console the formatted message
    log_message(buffer, buffer_size, level, log_msg, 1, 0);
    log_print_message(level, log_msg);
}
