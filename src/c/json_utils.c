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

// Retrieves the size of a string field in the JSON and checks if it's within the expected max size
int json_string_field_size(const char *json_str, const char *tag, char *field, size_t max_size) {
    json_error_t error;
    json_t *root;
    json_t *json_value;
    
    // Load the JSON string into a json_t object
    root = json_loads(json_str, 0, &error);
    if (!root) {
        printf("Error parsing JSON: %s\n", error.text);
        return 0;  // Return 0 on failure
    }
    
    // Find the field by tag
    json_value = json_object_get(root, tag);
    if (!json_value || !json_is_string(json_value)) {
        printf("Field '%s' not found or is not a string.\n", tag);
        json_decref(root);
        return 0;  // Return 0 if the field is not found or not a string
    }
    
    // Get the string value of the field
    const char *json_field_value = json_string_value(json_value);
    size_t field_size = strlen(json_field_value) + 1;  // +1 for null terminator

    // Verify the field size does not exceed max_size
    if (field_size > max_size) {
        printf("Field '%s' exceeds max allowed size of %zu.\n", tag, max_size);
        json_decref(root);
        return 0;  // Return 0 if the field size exceeds max_size
    }

    // Copy the field value into the provided char pointer
    strncpy(field, json_field_value, max_size);

    // Ensure null-termination
    field[max_size - 1] = '\0';

    // Clean up the json_t object
    json_decref(root);
    
    return 1;  // Return 1 on success
}

// Function to build or update a JSON string by adding a field with a string value
int json_add_string_field(char *json_str, size_t json_str_size, const char *field, const char *value) {
    json_t *root = NULL;
    json_error_t error;

    // If the input JSON string is empty, create a new JSON object
    if (strlen(json_str) == 0) {
        root = json_object();
        if (!root) {
            printf("failed to create json\n");
            return 0; // Failed to create a new JSON object
        }
    } else {
        // If the input JSON string is not empty, parse the existing JSON
        root = json_loads(json_str, 0, &error);
        if (!root) {
            printf("failed to parse json\n");
            return 0; // Failed to parse JSON string
        }
    }

    // Add or update the field with the specified string value
    if (json_object_set_new(root, field, json_string(value)) != 0) {
        printf("error adding/updating field:%s\t with value: %s\n",field,value);
        json_decref(root);
        return 0; // Failed to add or update the field
    }

    // Convert the JSON object back to a string
    char *new_json_str = json_dumps(root, JSON_COMPACT);
    if (!new_json_str) {
        printf("error converting json back to string\n");
        json_decref(root);
        return 0; // Failed to convert JSON object to string
    }

    // Ensure the new JSON string fits within the provided buffer size
    if (strlen(new_json_str) >= json_str_size) {
        printf("new json: %li >= json string size: %li\n", strlen(new_json_str), json_str_size);
        free(new_json_str);
        json_decref(root);
        return 0; // New JSON string exceeds buffer size
    }

    // Copy the new JSON string to the output buffer
    strncpy(json_str, new_json_str, json_str_size - 1);
    json_str[json_str_size - 1] = '\0'; // Ensure null-termination

    // Clean up
    free(new_json_str);
    json_decref(root);

    return 1; // Success
}


int build_json_response(char *json_str, size_t json_str_size, const char *fields_and_values[][2], size_t num_fields) {
    for (size_t i = 0; i < num_fields; ++i) {
        const char *field = fields_and_values[i][0];
        const char *value = fields_and_values[i][1];

        if (!json_add_string_field(json_str, json_str_size, field, value)) {
            printf("Failed to add field '%s' to JSON.\n", field);
            printf("error in field: %s \tvalue: %s\n",fields_and_values[i][0],fields_and_values[i][1]);
            return 0;  // Return 0 on failure
        }
    }
    return 1;  // Return 1 on success
}

int verify_json_fields(const char *json_str, const char *fields[], size_t num_fields) {
    json_error_t error;
    json_t *root;
    json_t *json_value;
    
    // Load the JSON string into a json_t object
    root = json_loads(json_str, 0, &error);
    if (!root) {
        printf("Error parsing JSON: %s\n", error.text);
        return 0;  // Return 0 on failure
    }
    
    for (size_t i = 0; i < num_fields; ++i) {
        const char *field = fields[i];
        json_t *json_type = json_object_get(root, field);
        if (!json_type || !json_is_string(json_type)) {
            json_decref(root);
            printf("Missing field: %s\n", field);
            return 0; // "TYPE" field not found or not a string
        }   
    }
    return 1;  // Return 1 if all fields are present
}