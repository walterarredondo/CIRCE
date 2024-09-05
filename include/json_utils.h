#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <jansson.h>

// Function to validate a JSON string
int validate_json(const char *json_str, char *error_msg, size_t error_msg_size);

// Function to create a JSON object with a name and age
//json_t* create_json_object(const char *name, int age);

// Function to extract a value from a JSON object
//int extract_json_values(const char *json_str, char *name, size_t name_size, int *age);

// Function to check if the "TYPE" field exists in the JSON and return its value
int json_type_field(const char *json_str, char *type_value, size_t value_size);

// Function to check if JSON contains a field and return its value
int json_field_matches(const char *json_str, const char *field, char *value, size_t value_size);

// Function to get the size of a string field in JSON and ensure it is within the expected max size
int json_string_field_size(const char *json_str, const char *field, size_t *size, size_t max_size);



#endif // JSON_UTILS_H