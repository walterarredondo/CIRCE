#include "json_utils.h"
#include <stdio.h>
#include <string.h>

// Validates a JSON string, returns 1 if valid, 0 if invalid
int validate_json(const char *json_str, char *error_msg, size_t error_msg_size) {
    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);
    
    if (!root) {
        snprintf(error_msg, error_msg_size, "Invalid JSON: %s", error.text);
        return 0;
    }

    json_decref(root);
    return 1;
}

// Checks if the JSON contains the "TYPE" field and returns its value if it exists
int json_type_field(const char *json_str, char *type_value, size_t value_size) {
    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);

    if (!root) {
        return 0; // Invalid JSON
    }

    json_t *json_type = json_object_get(root, "type");
    if (!json_type || !json_is_string(json_type)) {
        json_decref(root);
        return 0; // "TYPE" field not found or not a string
    }

    // Copy the value of the "TYPE" field to the output variable
    strncpy(type_value, json_string_value(json_type), value_size);
    json_decref(root);
    return 1; // "TYPE" field found and value extracted
}

// Check if JSON contains a field that matches a string and return its value
int json_field_matches(const char *json_str, const char *field, char *value, size_t value_size) {
    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);

    if (!root) {
        return 0; // Invalid JSON
    }

    json_t *json_field = json_object_get(root, field);
    if (!json_field) {
        json_decref(root);
        return 0; // Field not found
    }

    if (json_is_string(json_field)) {
        strncpy(value, json_string_value(json_field), value_size);
    } else if (json_is_integer(json_field)) {
        snprintf(value, value_size, "%lld", json_integer_value(json_field));
    } else if (json_is_real(json_field)) {
        snprintf(value, value_size, "%f", json_real_value(json_field));
    } else {
        json_decref(root);
        return 0; // Unsupported field type
    }

    json_decref(root);
    return 1; // Field found and value extracted
}

#include "json_utils.h"
#include <stdio.h>
#include <string.h>

// Retrieves the size of a string field in the JSON and checks if it's within the expected max size
int json_string_field_size(const char *json_str, const char *field, size_t *size, size_t max_size) {
    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);

    if (!root) {
        printf("invalid json\n");
        return 0; // Invalid JSON
    }

    json_t *json_field = json_object_get(root, field);
    if (!json_field || !json_is_string(json_field)) {
        json_decref(root);
        printf("field doesn't exists\n");
        return 0;
    }
    if (sizeof(json_field) > max_size) {
        json_decref(root);
        printf("size error: max size: %li actual size: %li\n",max_size,sizeof(json_field));
        return 0; // Field value exceeds max size
    }

    *size = sizeof(json_field);

    json_decref(root);
    return 1; // Successfully retrieved size
}
