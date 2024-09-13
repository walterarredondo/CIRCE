#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <jansson.h>
#define MAX_BUFFER 2048
#define TYPE_MAX_LENGHT 32
#define USER_MAX_LENGHT 9
#define MAX_INPUT_MSG 255
#define MAX_BUFFER_LOG 1024
#define VALUE_MAX_LENGHT 32
#define USER_MAX_LENGHT 9

#define FIELD_OPERATION "operation"
#define FIELD_USERNAME "username"
#define FIELD_RESULT "result"
#define FIELD_EXTRA "extra"
#define VALUE_IDENTIFY "IDENTIFY"
#define RESULT_SUCCESS "SUCCESS"
#define RESULT_USER_ALREADY_EXISTS "USER_ALREADY_EXISTS"

// Function to validate a JSON string
int validate_json(const char *json_str, char *error_msg, size_t error_msg_size);

// Function to check if the "TYPE" field exists in the JSON and return its value
int json_type_field(const char *json_str, char *type_value, size_t value_size);

// Function to check if JSON contains a field and return its value
int json_field_matches(const char *json_str, const char *field, char *value, size_t value_size);

// Function to get the size of a string field in JSON and ensure it is within the expected max size
int json_string_field_size(const char *json_str, const char *tag, char *field, size_t max_size);

// Function to add a field with a string value to a JSON string
int json_add_string_field(char *json_str, size_t json_str_size, const char *field, const char *value);

// Function for building a json from a 2D array
int build_json_response(char *json_str, size_t json_str_size, const char *fields_and_values[][2], size_t num_fields);

// Function for verifying a json string contain all the given fields in a 1D array 
int verify_json_fields(const char *json_str, const char *fields[], size_t num_fields);

//function to extract field value
int json_extract_field_value(const char *json_string, const char *field, char *value_buffer, size_t max_size);

#endif // JSON_UTILS_H