typedef enum {
    LOG_USER,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

// Define ANSI color codes as macros
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"


void log_message(char *buffer, size_t buffer_size , LogLevel level, const char *message, int file_log, int log_time);

const char* format_prompt(LogLevel level, const char *prompt, const char* message);

const char* format_log(LogLevel level, const char* message, int log_color, int log_time);

void log_file_message(const char * path, LogLevel level, const char *message);

void log_print_message(LogLevel level, const char *message);

int ensure_log_directory(char *dir);

void log_print_prompt(LogLevel level, const char *prompt, const char *message);

void log_server_message(const char *path, LogLevel level, const char* format, ...);

void log_print_file_message(const char *path, LogLevel level, const char *message);
void log_file_formatted_message(const char *path, LogLevel log_level, const char *format, ...);
char* string_printf(const char *format, ...);