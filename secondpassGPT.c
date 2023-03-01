void second_pass(FILE* fp, LabelTable* label_table) {
    char line[MAX_LINE_LENGTH];
    int ic = 0;
    int line_number = 1;

    while (fgets(line, MAX_LINE_LENGTH, fp) != NULL) {
        char label[MAX_LABEL_LENGTH];
        char op_name[MAX_OP_NAME_LENGTH];
        char operand[MAX_OPERAND_LENGTH];

        if (parse_line(line, label, op_name, operand) == 0) {
            continue;  // empty or comment line
        }

        // skip label
        if (is_label(label)) {
            continue;
        }

        // skip directives that were handled in the first pass
        if (is_data_directive(op_name) || is_string_directive(op_name) || is_extern_directive(op_name)) {
            continue;
        }

        // handle entry directive
        if (is_entry_directive(op_name)) {
            // mark corresponding labels as entry type
            char* label_list[MAX_LABELS_IN_LINE];
            int num_labels = get_label_list(operand, label_list);
            for (int i = 0; i < num_labels; i++) {
                Label* label = find_label(label_table, label_list[i]);
                if (label != NULL) {
                    label->type = ENTRY_LABEL;
                } else {
                    printf("Error at line %d: Undefined label '%s'\n", line_number, label_list[i]);
                }
            }
            continue;
        }

        // search opName in opTable
        Op* op = find_op(op_name);
        if (op == NULL) {
            printf("Error at line %d: Invalid operation '%s'\n", line_number, op_name);
            continue;
        }

        // analyze operands
        int num_words = 1 + count_operands(operand);
        char binary_code[MAX_CODE_LENGTH];
        build_word_binary_code(op, binary_code);

        // build binary code for each operand
        char* operand_list[MAX_OPERANDS_IN_LINE];
        int num_operands = get_operand_list(operand, operand_list);
        for (int i = 0; i < num_operands; i++) {
            char operand_binary_code[MAX_CODE_LENGTH];
            build_operand_binary_code(operand_list[i], label_table, ic + i + 1, operand_binary_code);
            strcat(binary_code, operand_binary_code);
        }

        // write binary code to output file
        write_to_ob_file(ic + CODE_OFFSET, binary_code, num_words);

        // increment IC
        ic += num_words;

        line_number++;
    }

    // write external and entry files
    write_external_file(label_table);
    write_entry_file(label_table);
}


// Note that this code assumes the existence of several helper functions, such as is_label, is_data_directive, is_string_directive, is_extern_directive, is_entry_directive, find_op, count_operands, get_operand_list, build_word_binary_code, build_operand_binary_code, write_to_ob_file, write_external_file, and write_entry_file. These functions would need to be implemented separately.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "constants.h"
#include "utils.h"
#include "data_structures.h"
#include "assembler.h"

int is_label(char *field) {
    if (field[strlen(field)-1] == ':') {
        return 1;
    }
    return 0;
}

int is_data_directive(char *field) {
    if (strcmp(field, ".data") == 0) {
        return 1;
    }
    return 0;
}

int is_string_directive(char *field) {
    if (strcmp(field, ".string") == 0) {
        return 1;
    }
    return 0;
}

int is_extern_directive(char *field) {
    if (strcmp(field, ".extern") == 0) {
        return 1;
    }
    return 0;
}

int is_entry_directive(char *field) {
    if (strcmp(field, ".entry") == 0) {
        return 1;
    }
    return 0;
}

OpCode* find_op(char *opName, OpCode *opTable) {
    int i;
    for (i = 0; i < NUM_OF_OPS; i++) {
        if (strcmp(opName, opTable[i].name) == 0) {
            return &opTable[i];
        }
    }
    return NULL;
}

int count_operands(char *operands) {
    int count = 0;
    char *token = strtok(operands, ",");
    while (token != NULL) {
        count++;
        token = strtok(NULL, ",");
    }
    return count;
}

char** get_operand_list(char *operands) {
    int i = 0;
    char **operandList = malloc(MAX_NUM_OF_OPERANDS * sizeof(char*));
    char *token = strtok(operands, ",");
    while (token != NULL) {
        operandList[i] = strdup(token);
        token = strtok(NULL, ",");
        i++;
    }
    operandList[i] = NULL;
    return operandList;
}

Word build_word_binary_code(OpCode *op, char *operands, LabelTable *lt) {
    int i;
    char **operandList = get_operand_list(operands);
    Word w;
    w.binaryCode = 0;
    w.era = A;

    w.binaryCode |= (op->code) << (WORD_SIZE - 6); // Set opcode
    w.binaryCode |= (op->funct) << (WORD_SIZE - 10); // Set funct

    int operandCodeStartBit = WORD_SIZE - 14;
    for (i = 0; i < op->numOfOperands; i++) {
        Operand *opType = &op->operands[i];
        char *opValue = operandList[i];
        if (opType->addressing == IMMEDIATE) {
            w.binaryCode |= (IMMEDIATE << operandCodeStartBit);
            w.binaryCode |= (atoi(opValue) & 0x7FFF);
        } else if (opType->addressing == DIRECT) {
            if (is_label(opValue)) {
                char *labelName = remove_colon(opValue);
                Label *label = get_label(labelName, lt);
                free(labelName);
                if (label == NULL) {
                    printf("Error: undefined label '%s'\n", opValue);
                    exit(EXIT_FAILURE);
                } else {
                    w.binaryCode |= (EXTERNAL << operandCodeStartBit);
                    write_external_file(label->name, lt->IC + i + 1);
                }
            } else {}
        }
    }
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "constants.h"
#include "utils.h"
#include "data_structures.h"
#include "assembler.h"

Word build_word_binary_code(OpCode *op, char *operands, LabelTable *lt) {
    int i, addressingType;
    char **operandList = get_operand_list(operands);
    Word word = 0;
    word |= op->opcode << OPCODE_SHIFT;
    word |= op->funct << FUNCT_SHIFT;
    for (i = 0; i < op->num_of_params; i++) {
        addressingType = op->params[i];
        if (addressingType == IMMEDIATE_ADDRESSING) {
            word |= build_operand_binary_code(operandList[i], lt) << IMMEDIATE_ADDRESSING_SHIFT;
        } else if (addressingType == DIRECT_ADDRESSING) {
            word |= build_operand_binary_code(operandList[i], lt) << DIRECT_ADDRESSING_SHIFT;
        } else if (addressingType == RELATIVE_ADDRESSING) {
            word |= build_operand_binary_code(operandList[i], lt) << RELATIVE_ADDRESSING_SHIFT;
        } else if (addressingType == REGISTER_ADDRESSING) {
            word |= atoi(&operandList[i][1]) << REGISTER_ADDRESSING_SHIFT;
        }
    }
    for (i = 0; i < op->num_of_params; i++) {
        free(operandList[i]);
    }
    free(operandList);
    return word;
}

Word build_operand_binary_code(char *operand, LabelTable *lt) {
    int i;
    Word code = 0;
    if (operand[0] == '#') { // immediate addressing
        code = atoi(&operand[1]);
    } else if (operand[0] == '%') { // relative addressing
        Label *label = find_label(lt, &operand[1]);
        code = label->address - (lt->IC + 1);
    } else { // direct addressing
        Label *label = find_label(lt, operand);
        if (label->type == EXTERN_TYPE) {
            code = 0; // external addressing
            write_external_file(label->name, lt->IC + START_ADDRESS);
        } else {
            code = label->address;
        }
    }
    return code;
}

void write_to_ob_file(char *filename, int *codeArr, int codeLength, int *dataArr, int dataLength) {
    int i;
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("Error opening output file: %s\n", filename);
        exit(1);
    }
    fprintf(fp, "%d %d\n", codeLength, dataLength);
    for (i = 0; i < codeLength; i++) {
        fprintf(fp, "%04d %05X\n", i + START_ADDRESS, codeArr[i]);
    }
    for (i = 0; i < dataLength; i++) {
        fprintf(fp, "%04d %05X\n", i + START_ADDRESS + codeLength, dataArr[i]);
    }
    fclose(fp);
}

void write_external_file(char *labelName, int address) {
    FILE *fp = fopen(EXTERNAL_FILENAME, "a");
    if (fp == NULL) {
        printf("Error opening external file: %s\n", EXTERNAL_FILENAME);
        exit(1);
    }
    fprintf(fp, "%s %04d\n", labelName, address);
    fclose(fp);
}


void write_entry_file(char *filename, LabelTable *lt) {
    FILE *fp;
    char entryFileName[MAX_FILENAME_LENGTH];

    // Open the entry file for writing
    sprintf(entryFileName, "%s.ent", filename);
    fp = fopen(entryFileName, "w");
    if (fp == NULL) {
        printf("Error: failed to open entry file for writing.\n");
        exit(1);
    }

    // Write each entry label to the file
    int i;
    for (i = 0; i < lt->count; i++) {
        if (lt->table[i].type == ENTRY_LABEL) {
            fprintf(fp, "%s\t%d\n", lt->table[i].name, lt->table[i].address);
        }
    }

    // Close the file
    fclose(fp);
}


//by cop
//byte build_word_binary_code(OpCode *op, char *operands, LabelTable *lt) {
void build_operand_binary_code(char *operand, LabelTable *label_table, int ic, char *binary_code) {
    int i;
    char *operand_list[MAX_OPERANDS_IN_LINE];
    int num_operands = get_operand_list(operand, operand_list);
    for (i = 0; i < num_operands; i++) {
        char *operand = operand_list[i];
        if (is_label(operand)) {
            char *label_name = remove_colon(operand);
            Label *label = get_label(label_name, label_table);
            free(label_name);
            if (label == NULL) {
                printf("Error: undefined label '%s'\n", operand);
                exit(EXIT_FAILURE);
            } else {
                if (label->type == EXTERN_LABEL) {
                    write_external_file(label->name, ic);
                }
                char *binary_address = decimal_to_binary(label->address);
                strcat(binary_code, binary_address);
                free(binary_address);
            }
        } else {
            char *binary_address = decimal_to_binary(atoi(operand));
            strcat(binary_code, binary_address);
            free(binary_address);
        }
    }
}
               
