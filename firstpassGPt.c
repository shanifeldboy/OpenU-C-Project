void first_pass(FILE *file) {
    int IC = 0, DC = 0;
    char line[MAX_LINE_LENGTH];

    while (fgets(line, MAX_LINE_LENGTH, file)) {
        if (line[0] == '\n' || line[0] == ';') { // Ignore empty lines and comments
            continue;
        }

        char *label = NULL, *opName = NULL, *operands = NULL;
        parse_line(line, &label, &opName, &operands);

        // Check for labels
        int isLabel = 0;
        if (label != NULL) {
            insert_label(label, IC, CODE_LABEL);
            isLabel = 1;
        }

        // Check directives
        if (strcmp(opName, ".data") == 0 || strcmp(opName, ".string") == 0) {
            if (isLabel) {
                insert_label(label, DC, DATA_LABEL);
            }
            else {
                DC++;
            }
        }
        else if (strcmp(opName, ".entry") == 0) {
            // Do nothing for first pass
        }
        else if (strcmp(opName, ".extern") == 0) {
            // Insert operands as external labels
            char *operand = strtok(operands, ",");
            while (operand != NULL) {
                insert_label(operand, 0, EXTERN_LABEL);
                operand = strtok(NULL, ",");
            }
        }
        else { // Instruction
            // Check opName in opTable
            op_info_t opInfo = get_op_info(opName);
            if (opInfo.opCode == -1) {
                printf("Error: invalid operation name at line %d\n", line_number);
                continue;
            }

            // Analyze operands
            int L = analyze_operands(opName, operands);
            if (L == -1) {
                continue;
            }

            // Build binary code of first word
            int A = 0, R = 0, E = 0;
            build_first_word(opInfo, A, R, E, L);
            IC += L;
        }

        free(label);
        free(opName);
        free(operands);
    }

    update_labels_address(IC);
    second_pass(file, IC, DC);
}

void second_pass(FILE *file, int IC, int DC) {
    char line[MAX_LINE_LENGTH];
    int lineNumber = 0;

    rewind(file);

    while (fgets(line, MAX_LINE_LENGTH, file)) {
        if (line[0] == '\n' || line[0] == ';') { // Ignore empty lines and comments
            continue;
        }

        lineNumber++;

        char *label = NULL, *opName = NULL, *operands = NULL;
        parse_line(line, &label, &opName, &operands);

        if (label != NULL) {
            free(label);
        }

        if (opName != NULL) {
            if (strcmp(opName, ".data") == 0) {
                handle_data(lineNumber, operands);
            }
            else if (strcmp(opName, ".string") == 0) {
                handle_string(lineNumber, operands);
            }
            else if (strcmp(opName, ".entry") == 0) {
                handle_entry(lineNumber, operands);
            }
            else if (strcmp(opName, ".extern") == 0) {
                // Do nothing for second pass
            }
            else { // Instruction
                handle_instruction(lineNumber, opName, operands);
            }

            free(opName);
        }
    }

    write_output_files(IC, DC);
}
