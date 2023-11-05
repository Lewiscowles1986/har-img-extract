#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include "json.h"
#include <b64/cdecode.h>

int create_directory(const char *path) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    // Copy the path so that we can modify it
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if(tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    // Iterate the string
    for(p = tmp + 1; *p; p++) {
        if(*p == '/') {
            *p = 0; // Temporarily truncate
            if (create_directory(tmp) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/'; // Restore slash
        }
    }

#ifdef _WIN32
    return _mkdir(tmp);
#else
    return mkdir(tmp, 0755);
#endif
}

json_value_t* get_object_key_impl(json_value_t* value, char* key);
json_value_t* get_object_key(json_value_t* value, const char* key);

typedef struct mimetype
{
	const char* name;
	const char* extension;
} mimetype_t;

typedef struct path_parts
{
	char* path;
	char* file;
} path_parts_t;

typedef struct parsed_data
{
	const char* data;
	long size;
} parsed_data_t;

mimetype_t mimetypes[] = {
	{"image/webp", ".webp"},
	{"image/jpeg", ".jpeg"},
	{"image/jpeg", ".jpg"},
	{"image/png", ".png"},
	{"image/svg+xml", ".svg"}
};

const char* get_extension_for_mimetype(const char* mimetype) {
    size_t num_mimetypes = sizeof(mimetypes) / sizeof(mimetypes[0]);
    
    for (size_t i = 0; i < num_mimetypes; ++i) {
        if (strcmp(mimetypes[i].name, mimetype) == 0) {
            return mimetypes[i].extension;
        }
    }
    
    return NULL; // MIME type not found
}

int dir_exists(const char* path)
{
	struct stat sb;
	return (stat(path, &sb) == 0) & (sb.st_mode & S_IFDIR);
}

char* read_file(const char* filename, size_t* len)
{
	// printf("Reading file %s...\n", filename);
	FILE* handle = fopen(filename, "rb");
	if (!handle)
	{
		printf("Cannot open file %s\n", filename);
		return NULL;
	}
	// printf("File %s opened.\n", filename);
	fseek(handle, 0l, SEEK_END);
	// printf("Read to end of file %s.\n", filename);
	*len = ftell(handle);
	// printf("Reading file size %s.\n", filename);
	rewind(handle);
	// printf("Rewinding file pointer %s.\n", filename);

	// Allocate memory for the file content plus a null terminator
    char* data = (char*)malloc(*len + 1); // Added +1 for null terminator
    if (data == NULL) {
        fclose(handle);
        puts("Cannot allocate memory");
        return NULL;
    }
	// puts("Allocated string of file contents.");

    // Read the file into memory and null-terminate the string
    size_t read_result = fread(data, 1, *len, handle);
    data[read_result] = '\0'; // Null-terminate the data

	// puts("Read File contents.");
	if (read_result != *len) {
        free(data);
        puts("Failed to read the full file");
        return NULL;
    }

	return data;
}

json_value_t* parse_file(const char* filename)
{
	size_t len;
	char* data = read_file(filename, &len);
	// puts("HAR File read");
	if (data == NULL) return NULL;
	json_value_t* json = json_parse(data, len);
	if (json == NULL)
	{
		printf("Cannot parse file %s\n", filename);
		return NULL;
	}
	return json;
}

json_value_t* get_object_key_impl(json_value_t* value, char* key) {
	// puts("called get_object_key_impl");

	char* next_key = strchr(key, '.');
	if (next_key) {
		*next_key = '\0'; // Split the key on the dot only if a dot was found
	}

	json_object_t* obj = json_value_as_object(value);
	// puts("\tread json value as object");
	if (obj == NULL) return NULL;

	for (json_object_element_t* e = obj->start; e; e = e->next) {
		if (strcmp(e->name->string, key) == 0) {
			if (next_key == NULL) {
				return e->value;
			} else {
				next_key[0] = '.'; // Restore the dot so the original string is not altered
				return get_object_key_impl(e->value, next_key + 1); // Recursive call to the same function
			}
		}
	}
	return NULL;
}

json_value_t* get_object_key(json_value_t* value, const char* key)
{
	char *mutKey = strdup(key);
	// puts("calling get_object_key_impl");
	json_value_t* r = get_object_key_impl(value, mutKey);
	// puts("all done get_object_key_impl");
	return r;
}

const char* path_get_last(const char* path)
{
	size_t len = strlen(path);
	for (int i = len - 1; i > -1; i--)
	{
		if (path[i] == '/') return path + i + 1;
	}
	return NULL;
}

int ends_with(const char* str, const char* suffix)
{
	size_t str_len = strlen(str);
	size_t suffix_len = strlen(suffix);
	return (str_len >= suffix_len) &&
		(!memcmp(str + str_len - suffix_len, suffix, suffix_len));
}

int extensions_ends_with(const char* string)
{
	for (int i = 0; i < sizeof(mimetypes); i++)
	{
		if (ends_with(string, mimetypes[i].extension))
		{
			return 1;
		}
	}
	return 0;
}

const char* get_string_from_json(json_value_t* entry, const char* key_path) {
    if (entry == NULL || key_path == NULL) {
        // puts("Invalid argument: NULL pointer provided.");
        return NULL;
    }

    json_value_t* value = get_object_key(entry, key_path);
    if (value == NULL) {
        // printf("Invalid JSON: expected key '%s'.\n", key_path);
        return NULL;
    }

    json_string_t* string = json_value_as_string(value);
    if (string == NULL || string->string == NULL) {
        // printf("Invalid JSON: expected key '%s' to be a string.\n", key_path);
        return NULL;
    }

    return string->string;
}

#define NOT_A_NUMBER LONG_MIN // Make sure to define this appropriately

long get_long_from_json(json_value_t* entry, const char* key_path) {
    if (entry == NULL || key_path == NULL) {
        // fprintf(stderr, "Invalid argument: NULL pointer provided.\n");
        return LONG_MIN;
    }

    json_value_t* value = get_object_key(entry, key_path);
    if (value == NULL) {
        // fprintf(stderr, "Invalid JSON: expected key '%s'.\n", key_path);
        return LONG_MIN;
    }

    json_number_t* json_num = json_value_as_number(value);
    if (json_num == NULL || json_num->number == NULL) {
        // fprintf(stderr, "Invalid JSON: expected key '%s' to be a string representing a number.\n", key_path);
        return LONG_MIN;
    }

    char* endptr;
    errno = 0; // To detect any errors during conversion
    long num = strtol(json_num->number, &endptr, 10);

    if (errno == ERANGE && (num == LONG_MAX || num == LONG_MIN)) {
        perror("Error");
        return LONG_MIN; // Overflow or underflow occurred
    }

    if (endptr == json_num->number) {
        fprintf(stderr, "No digits were found in the string.\n");
        return LONG_MIN; // No number found
    }

    if (*endptr != '\0') {
        fprintf(stderr, "Further characters after number: %s\n", endptr);
        return LONG_MIN; // The string contains more characters after the number
    }

    // If we reach here, strtol() successfully parsed the number
    return num;
}

parsed_data_t* decode_base64(const char* encoded) {
    base64_decodestate state;
    base64_init_decodestate(&state);
	parsed_data_t* result = (parsed_data_t*)malloc(sizeof(parsed_data_t));

    // Compute the maximum possible size of the decoded data
    size_t encoded_len = strlen(encoded);
    size_t decoded_len = encoded_len * 3 / 4; // Approximation, actual may be less

    char* decoded_out = (char*)malloc(decoded_len + 1); // +1 for the null terminator
    if (!decoded_out) {
        // Handle allocation failure...
        return NULL;
    }

    // Decode the entire block in one go
    int count = base64_decode_block(encoded, encoded_len, decoded_out, &state);

    // Optionally resize the buffer down to the actual size needed
    char* resized_out = realloc(decoded_out, count);
    if (!resized_out) {
        // Handle potential realloc failure...
        free(decoded_out);
        return NULL;
    }

	result->data = decoded_out;
	result->size = count;
    return result; // Return the resized buffer with the decoded data
}

parsed_data_t* convert_to_raw_bytes(const char* input, const char* encoding) {
    if (encoding == NULL || strcmp(encoding, "literal") == 0) {
		parsed_data_t* result = (parsed_data_t*)malloc(sizeof(parsed_data_t));
		result->data = input;
		result->size = strlen(input);
        return result;
    } else if (strcmp(encoding, "base64") == 0) {
		size_t num_chars = strlen(input);  // Number of chars without null terminator
	    // Allocate memory for the destination buffer (no space for '\0' needed)
    	char* destination = malloc(num_chars);
		memcpy(destination, input, num_chars);
		// printf("size of base64 string %zu\n", num_chars);
        return decode_base64(destination);
    } else {
        // Handle other encodings if necessary
        fprintf(stderr, "Unsupported encoding: %s\n", encoding);
        return NULL;
    }
}

int write_bytes_to_file(const char* filepath, const char* data, size_t size) {
    FILE *file = fopen(filepath, "wb"); // Open the file for writing in binary mode
    if (!file) {
        perror("Error opening file for writing");
        return -1;
    }

    size_t written = fwrite(data, 1, size, file); // Write data to file
    if (written < size) {
        perror("Error writing to file");
        fclose(file);
        return -1;
    }

    fclose(file); // Close the file
    return 0; // Success
}

char* get_filename_without_extension(const char* url) {
    // Find the last '/' character.
    const char* last_slash = strrchr(url, '/');
    if (last_slash == NULL) {
        last_slash = url; // No slash found, assume the whole thing is a file name.
    } else {
        last_slash++; // Move past the last '/' character.
    }

    // Find the last '.' character.
    const char* last_dot = strrchr(last_slash, '.');
    size_t filename_length;
    if (last_dot != NULL) {
        filename_length = last_dot - last_slash;
    } else {
        // No dot found after the last slash, so the rest of the string is the file name.
        filename_length = strlen(last_slash);
    }

    // Copy the file name to a new buffer.
    char* filename = malloc(filename_length + 1); // +1 for the null terminator.
    if (filename == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL; // Failed to allocate memory.
    }

    strncpy(filename, last_slash, filename_length);
    filename[filename_length] = '\0'; // Ensure null termination.

    return filename;
}

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		puts(
			"extract_har: A simple script to extract all picture files from * .har file.\n"
			"Usage : extract_har <input_file> [<output_dir>]"
		);
		return 1;
	}
	// printf("Reading file %s...\n", argv[1]);
	json_value_t* harJson = parse_file(argv[1]);
	// printf("Parsed file %s\n", argv[1]);
	if (harJson == NULL)
	{
		return 1;
	}
	char outputDir[256];
	strcpy(outputDir, argv[2]);
	// printf("Checking if output dir %s exists.\n", outputDir);
	if (!dir_exists(outputDir))
	{
		// printf("Creating output dir %s...\n", outputDir);
		create_directory(outputDir);
	}
	// printf("output dir %s exists.\n", outputDir);
	// puts("Reading log entries");
	json_value_t* harJsonEntriesValue = get_object_key(harJson, "log.entries");
	if (harJsonEntriesValue == NULL)
	{
		puts("Invalid JSON: expected key 'log.entries'");
		return 1;
	}
	int count_total = 0, count_extracted = 0;
	// puts("iterating over elements");
	for (json_array_element_t* entry = json_value_as_array(harJsonEntriesValue)->start; entry; entry = entry->next)
	{
		const char* file_url = get_string_from_json(entry->value, "request.url");
		if (file_url == NULL)
		{
			puts("Invalid JSON: expected key 'log.entries.[idx].request.url'\n");
			continue;
		}

		// printf("Processing url '%s'\n", file_url);

		const char* response_mime_type = get_string_from_json(entry->value, "response.content.mimeType");
		if (response_mime_type == NULL)
		{
			// puts("Invalid JSON: expected key 'log.entries.[idx].response.content.mimeType'\n");
			continue;
		}
		const char* mt = get_extension_for_mimetype(response_mime_type);
		if (mt == NULL) {
			// puts("Skipping mime-type we don't deal with...");
			continue;
		} else {
			// printf("mime type is %s\n", response_mime_type);
		}

		

		// printf("file extension should be %s\n", mt);
		char filename[256];
		sprintf(filename, "%s/%s%s", outputDir, get_filename_without_extension(file_url), mt);
		// printf("file path will be %s\n", filename);

		const char* response_encoding = get_string_from_json(entry->value, "response.content.encoding");
		if (response_encoding == NULL)
		{
			response_encoding = "literal";
		}
		// printf("encoding is %s\n", response_encoding);
		const char* response_text = get_string_from_json(entry->value, "response.content.text");
		if (response_text == NULL)
		{
			// puts("Invalid JSON: expected key 'log.entries.[idx].response.content.text'\n");
			continue;
		}
		// json_value_as_number
		// "encoding": "base64",
		long length = get_long_from_json(entry->value, "response.content.size");
		if (length == LONG_MIN) {
			// puts("length invalid. Skipping...");
			continue;
		}
		long body_size = get_long_from_json(entry->value, "response.bodySize");
		if (body_size == LONG_MIN) {
			// puts("bodySize invalid. Skipping...");
			continue;
		} else if (body_size == 0) {
			// puts("bodySize zero. No data for file in HAR. Skipping...");
			continue;
		}
		// printf("size is expected to be %d\n", length);
		// printf("bodySize is expected to be %d\n", body_size);
		parsed_data_t* raw_data = convert_to_raw_bytes(response_text, response_encoding);
		if (raw_data == NULL) {
			// puts("Unable to read data...");
		}
		// printf("raw_bytes size is %d\n", raw_data->size);
		// printf("writing to file %s\n", filename);
		write_bytes_to_file(filename, raw_data->data, raw_data->size);
		count_total++;
	}
	return (count_total == 0) ? 1 : 0;
}
