#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE_LENGTH 80
#define MAX_MACRO_LENGTH 100
#define MAX_MACRO_NAME_LENGTH 30

char* macro_table[MAX_MACRO_LENGTH];
char* input_file_name;

int is_macro(char* label) {
    for (int i = 0; i < MAX_MACRO_LENGTH; i++) {
        if (macro_table[i] != NULL && strcmp(macro_table[i], label) == 0) {
            return 1;
        }
    }
    return 0;
}


/* function to replace macros in a line */
void replace_macro(char *line, char **macro_table) {
    char *token, *macro, *value, *rest_of_line;
    int i;

    token = strtok(line, " \t");  // get first token in line

    while (token != NULL) {
        if (token[0] == '#') {  // check if token is a macro
            macro = strtok(token + 1, " \t");  // extract macro name
            value = NULL;

            // search for macro in table
            for (i = 0; macro_table[i] != NULL; i += 2) {
                if (strcmp(macro, macro_table[i]) == 0) {
                    value = macro_table[i + 1];
                    break;
                }
            }

            if (value != NULL) {
                // replace macro with its value
                rest_of_line = strstr(line, macro) + strlen(macro) + 1;
                sprintf(token, "%s %s", value, rest_of_line);
            }
        }

        token = strtok(NULL, " \t");  // get next token in line
    }
}

/* function to save assembled file to disk */
void save_file(char *filename, int *code_image, int code_size, int *data_image, int data_size) {
    int i;
    char ob_filename[MAX_MACRO_LEN], ext_filename[MAX_MACRO_LEN], ent_filename[MAX_MACRO_LEN];
    FILE *ob_file, *ext_file, *ent_file;

    // create output file names
    strcpy(ob_filename, filename);
    strcpy(ext_filename, filename);
    strcpy(ent_filename, filename);
    strcat(ob_filename, ".ob");
    strcat(ext_filename, ".ext");
    strcat(ent_filename, ".ent");

    // open object file for writing
    ob_file = fopen(ob_filename, "w");
    if (ob_file == NULL) {
        printf("Error: could not create object file\n");
        return;
    }

    // write IC and DC to object file
    fprintf(ob_file, "%d %d\n", code_size, data_size);

    // write code image to object file
    for (i = 0; i < code_size; i++) {
        fprintf(ob_file, "%04d %03X\n", 100 + i, code_image[i]);
    }

    // write data image to object file
    for (i = 0; i < data_size; i++) {
        fprintf(ob_file, "%04d %03X\n", 100 + code_size + i, data_image[i]);
    }

    fclose(ob_file);

    // open externals file for writing
    ext_file = fopen(ext_filename, "w");
    if (ext_file == NULL) {
        printf("Error: could not create externals file\n");
        return;
    }

    // TODO: write external labels and memory addresses to file

    fclose(ext_file);

    // open entries file for writing
    ent_file = fopen(ent_filename, "w");
    if (ent_file == NULL) {
        printf("Error: could not create entries file\n");
        return;
    }

    // TODO: write internal labels and memory addresses to file

    fclose(ent_file);
}

void pre_assembler() {
    FILE* input_file = fopen(input_file_name, "r");
    if (input_file == NULL) {
        printf("Failed to open input file.\n");
        return;
    }

    char line[MAX_LINE_LENGTH];
    int isMCR = 0;
    int macro_index = 0;

    while (fgets(line, MAX_LINE_LENGTH, input_file)) {
        char* label = strtok(line, " \t\n");
        if (label == NULL) {
            continue;
        }

        if (is_macro(label)) {
            replace_macro(line);
            continue;
        }

        if (strcmp(label, "mcr") == 0) {
            isMCR = 1;
            char* macro_name = strtok(NULL, " \t\n");
            macro_table[macro_index++] = strdup(macro_name);
            continue;
        }

        if (isMCR) {
            if (strcmp(label, "endmcr") == 0) {
                isMCR = 0;
                continue;
            }
            macro_table[macro_index++] = strdup(line);
            continue;
        }

        // Perform any necessary processing on the line here
    }

    fclose(input_file);
    save_file();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input file>\n", argv[0]);
        return 0;
    }

    input_file_name = argv[1];
    pre_assembler();

    return 0;
}