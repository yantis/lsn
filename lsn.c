/*
 * Copyright (C) 2023 Jonathan Yantis. All rights reserved.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <dirent.h>

/* Function declarations */
void set_description(const char *filename, const char *description);
void get_plain_filename(const char *colored_filename, char *plain_filename);
void url_encode(const char *str, char *encoded, size_t encoded_size);
void url_decode(const char *str, char *decoded, size_t decoded_size);
void display_files(int num_additional_args, char **additional_args);
void parse_arguments(int argc, char *argv[], bool *should_set_description, int *num_additional_args, char ***additional_args);
void show_help();
void set_description_if_needed(bool should_set_description, int argc, char *argv[], int num_additional_args, char **additional_args);
const char *get_description(const char *filename);
void print_description(const char *filename, int padding);

#define MAX_LINE_LENGTH 1024
#define DESCRIPTION_FILE ".descriptions"
#define BLUE_COLOR "\033[34m"
#define RESET_COLOR "\033[0m"

#ifdef DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

/* Function implementations */
void show_help() {
    printf("lsn v1.029 - List with Notes\n\n");
    printf("Display files and directories with descriptions\n\n");
    printf("Usage: lsn [OPTION]... [FILE]... [DESCRIPTION]...\n\n");
    printf("Options:\n");
    printf("--help           Display this help and exit\n");
    printf("  -e, --extended-description            Set description for a file\n\n");
    printf("Examples:\n");
    printf("  lsn                List files and directories with descriptions\n");
    printf("  lsn -e file.txt 'my description'   Set description for file.txt\n");
    printf("  lsn [Any of the ls arguments beloww]	Will pass arguments to ls\n\n");
    printf("Copyright (C) 2023 Jonathan Yantis. All rights reserved.\n\n");

    printf("ls command help:\n");
    system("ls --help");
}

/* Parses command-line arguments */
void parse_arguments(int argc, char *argv[], bool *should_set_description, int *num_additional_args, char ***additional_args) {
    int c;
    int add_arg_index = 0;
    opterr = 0; // Disable error messages from getopt_long
    char **temp_additional_args = (char **)malloc((argc * 3) * sizeof(char *));

    while (1) {
        static struct option long_options[] = {
            {"help", no_argument, 0, 0},
            {"extended-description", no_argument, 0, 'e'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long(argc, argv, "e", long_options, &option_index);
        if (c == -1) {
            break;
        }
        switch (c) {
            case 0:
                if (option_index == 0) { // Check if the option_index corresponds to the "help" option
                    show_help();
                    exit(0);
                }
                break;
            case 'e':
                *should_set_description = true;
                break;
            case '?':
                break;
        }

        if (c == '?' || c == -1) {
            DEBUG_PRINT("DEBUG: optind = %d\n", optind);
            DEBUG_PRINT("DEBUG: argv[optind - 1] = %s\n", argv[optind - 1]);
            DEBUG_PRINT("DEBUG: optopt = %d\n", optopt);

            if (optopt != 0) {
                char unknown_opt[3] = "-";
                unknown_opt[1] = optopt;
                unknown_opt[2] = '\0';
                temp_additional_args[add_arg_index] = (char *)malloc(3 * sizeof(char));
                strcpy(temp_additional_args[add_arg_index], unknown_opt);
                add_arg_index++;
            } else if (argv[optind - 1][0] == '-') {
                temp_additional_args[add_arg_index] = strdup(argv[optind - 1]);
                add_arg_index++;
            }
            DEBUG_PRINT("DEBUG: (c == '?' || c == -1)\n");
        } else {
            break;
        }

    }


    while (optind < argc) {
        temp_additional_args[add_arg_index++] = argv[optind++];
    }

    *num_additional_args = add_arg_index;
    temp_additional_args[add_arg_index] = NULL;
    *additional_args = temp_additional_args;

    DEBUG_PRINT("DEBUG: should_set_description = %d\n", *should_set_description);
    DEBUG_PRINT("DEBUG: num_additional_args = %d\n", *num_additional_args);
    for (int i = 0; i < *num_additional_args; i++) {
        DEBUG_PRINT("DEBUG: additional_args[%d] = %s\n", i, (*additional_args)[i]);
    }
}




/* Displays the output of ls -l command along with the file descriptions. */
void display_files(int num_additional_args, char **additional_args) {
    static char line[MAX_LINE_LENGTH];
    char *filename;
    size_t max_filename_length = 0;

    char ls_cmd[MAX_LINE_LENGTH] = "ls --color=always -l";
    for (int i = 0; i < num_additional_args; i++) {
        strcat(ls_cmd, " ");
        strcat(ls_cmd, additional_args[i]);
    }
    FILE *ls_output = popen(ls_cmd, "r");
    if (ls_output == NULL) {
        fprintf(stderr, "Failed to run ls command\n");
        return;
    }

    // Find the maximum filename length
    while (fgets(line, sizeof(line), ls_output)) {
        filename = strrchr(line, ' ');
        if (filename) {
            *filename = '\0';
            filename++;
            filename[strcspn(filename, "\n")] = '\0';

            char plain_filename[MAX_LINE_LENGTH] = {0};
            get_plain_filename(filename, plain_filename);

            size_t filename_length = strlen(plain_filename);
            if (filename_length > max_filename_length) {
                max_filename_length = filename_length;
            }
        }
    }
    pclose(ls_output);

    strcpy(ls_cmd, "ls --color=always -l");
    for (int i = 0; i < num_additional_args; i++) {
        strcat(ls_cmd, " ");
        strcat(ls_cmd, additional_args[i]);
    }

    ls_output = popen(ls_cmd, "r");
    if (ls_output == NULL) {
        fprintf(stderr, "Failed to run ls command\n");
        return;
    }

    DEBUG_PRINT("DEBUG: ls_cmd = %s\n", ls_cmd);

    // Print the filenames and descriptions with padding
    while (fgets(line, sizeof(line), ls_output)) {
        filename = strrchr(line, ' ');
        if (filename) {
            *filename = '\0';
            filename++;
            filename[strcspn(filename, "\n")] = '\0';

            char plain_filename[MAX_LINE_LENGTH] = {0};
            get_plain_filename(filename, plain_filename);
            printf("%s %s", line, filename);
            int padding = (int)max_filename_length - (int)strlen(plain_filename) + 2;
            print_description(plain_filename, padding);
            printf("\n");
        } else {
            printf("%s", line);
        }
    }
    pclose(ls_output);
}

/* Extracts the plain file name from the colored file name */
void get_plain_filename(const char *colored_filename, char *plain_filename) {
    if (!colored_filename || !plain_filename) {
        return;
    }

    int i = 0;
    int j = 0;
    bool inside_escape = false;

    while (colored_filename[i] != '\0') {
        if (colored_filename[i] == '\033') {
            inside_escape = true;
        }

        if (!inside_escape) {
            plain_filename[j++] = colored_filename[i];
        }

        if (inside_escape && colored_filename[i] == 'm') {
            inside_escape = false;
        }

        i++;
    }

    plain_filename[j] = '\0';
}

/* Encodes a string into URL format. */
void url_encode(const char *str, char *encoded, size_t encoded_size) {
    char *p_encoded = encoded;

    while (*str && (size_t)(p_encoded - encoded) < encoded_size - 4) {
        if (isalnum(*str) || *str == '-' || *str == '_' || *str == '.') {
            *p_encoded++ = *str;
        } else {
            sprintf(p_encoded, "%%%02X", *str);
            p_encoded += 3;
        }
        str++;
    }

    *p_encoded = '\0';
}

/* Retrieves the description of a file from the .descriptions file. */
const char *get_description(const char *filename) {
    static char line[MAX_LINE_LENGTH];
    static char decoded_line[MAX_LINE_LENGTH];
    static char decoded_description[MAX_LINE_LENGTH];
    char *description = NULL;
    FILE *fp = fopen(DESCRIPTION_FILE, "r");
    if (fp == NULL) {
        return "";
    }

    while (fgets(line, sizeof(line), fp)) {
        char *separator = strchr(line, '|');
        if (separator) {
            *separator = '\0';
            description = separator + 1;
            description[strcspn(description, "\n")] = '\0';

            url_decode(line, decoded_line, sizeof(decoded_line));
            if (strcmp(decoded_line, filename) == 0) {
                fclose(fp);
                url_decode(description, decoded_description, sizeof(decoded_description));
                return decoded_description;
            }
        }
    }

    fclose(fp);
    return "";
}


/* Prints the description of a file with padding. */
void print_description(const char *filename, int padding) {
    if (!filename) {
        return;
    }
    const char *description = get_description(filename);
    if (strlen(description) > 0) {
        printf("%*s" BLUE_COLOR "%s" RESET_COLOR, padding, "", description);
    }
}

/* Sets the description for a file by updating the .descriptions file. */
void set_description_if_needed(bool should_set_description, int argc, char *argv[], int num_additional_args, char **additional_args) {
    if (should_set_description) {
        DEBUG_PRINT("DEBUG: should_set_description = %d\n", should_set_description);
        DEBUG_PRINT("DEBUG: argc = %d\n", argc);

        for (int i = 0; i < argc; i++) {
            DEBUG_PRINT("DEBUG: argv[%d] = %s\n", i, argv[i]);
        }

        if (argc == 4) { // Check if there are exactly two additional arguments for filename and description
            char *filename = argv[2];
            char *description = argv[3];

            DEBUG_PRINT("DEBUG: filename = %s\n", filename);
            DEBUG_PRINT("DEBUG: description = %s\n", description);

            char encoded_description[MAX_LINE_LENGTH];
            url_encode(description, encoded_description, sizeof(encoded_description));
            set_description(filename, encoded_description);
        } else {
            fprintf(stderr, "Missing filename and/or description for setting description\n");
            exit(1);
        }
    } else {
        display_files(num_additional_args, additional_args);
    }
}

/* Decodes a string into URL format. */
void url_decode(const char *str, char *decoded, size_t decoded_size) {
    char *p_decoded = decoded;

    while (*str && (size_t)(p_decoded - decoded) < decoded_size - 1) {
        if (*str == '%') {
            unsigned int code;
            sscanf(str + 1, "%02x", &code);
            *p_decoded++ = (char)code;
            str += 3;
        } else {
            *p_decoded++ = *str;
            str++;
        }
    }

    *p_decoded = '\0';
}

void set_description(const char *filename, const char *description) {
    if (!filename || !description) {
        return;
    }
    char line[MAX_LINE_LENGTH];
    char temp_descriptions_filename[] = ".descriptions.temp";
    FILE *descriptions_file = fopen(DESCRIPTION_FILE, "a+");

    if (descriptions_file == NULL) {
        perror("Error opening description file");
        return;
    }

    FILE *temp_descriptions_file = fopen(temp_descriptions_filename, "w");

    if (temp_descriptions_file == NULL) {
        perror("Error opening temporary description file");
        fclose(descriptions_file);
        return;
    }

    int description_set = 0;

    while (fgets(line, sizeof(line), descriptions_file)) {
        char *file_entry = strtok(line, "|");
        char *desc_entry = strtok(NULL, "\n");

        if (strcmp(file_entry, filename) == 0) {
            fprintf(temp_descriptions_file, "%s|%s\n", filename, description);
            description_set = 1;
        } else {
            fprintf(temp_descriptions_file, "%s|%s\n", file_entry, desc_entry);
        }
    }

    if (!description_set) {
        fprintf(temp_descriptions_file, "%s|%s\n", filename, description);
    }

    fclose(descriptions_file);
    fclose(temp_descriptions_file);
    remove(DESCRIPTION_FILE);
    rename(temp_descriptions_filename, DESCRIPTION_FILE);
}

/* Main function */
int main(int argc, char *argv[]) {
    bool should_set_description = false;
    int num_additional_args = 0;
    char **additional_args = NULL;

    parse_arguments(argc, argv, &should_set_description, &num_additional_args, &additional_args);
    set_description_if_needed(should_set_description, argc, argv, num_additional_args, additional_args);
    free(additional_args);

    return 0;
}


