#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <zip.h>

#include "error_codes.h"

#define MAX_PASSWORD_LENGTH 512
#define MAX_COMMAND_LENGTH 512

struct string_node {
        char *value;
        size_t len;

        struct string_node *next;
};

struct string_node *string_node_new(char *value, size_t len)
{
        struct string_node *result = (struct string_node*)malloc(sizeof(*result));
        result->value = value;
        result->len = len;
        result->next = NULL;
        return result;
}

void string_node_free_self_and_following(struct string_node *node) {
        while (node) {
                struct string_node *current = node;
                node = node->next;

                free(current->value);
                free(current);
        }
}

void dump_list(struct string_node *list)
{
        while (list) {
                fprintf(stderr, "%s (%zu)\n", list->value, list->len);
                list = list->next;
        }
}

void print_unknown_command(void)
{

        fprintf(stderr, "Unkown command. Try \"help\".\n");
}

int main(int argc, const char *argv[])
{
        if (argc < 2) {
                fprintf(stderr, "Please provide an input file. (Syntax: zpwm file.zip)\n");
                return ERROR_NO_INPUT_FILE;
        }

        const char *file_name = argv[1];
        fprintf(stderr, "Operating on file: %s\n", file_name);

        size_t buffer_size = MAX_PASSWORD_LENGTH + 1;
        char *password = (char*)malloc(buffer_size);

        if (!password) {
                fprintf(stderr, "Failed to allocate %zu bytes for the password!\n", buffer_size);
                return ERROR_CANNOT_ALLOCATE_PASSWORD_BUFFER;
        }

        fprintf(stderr, "Please enter your password: ");
        size_t characters_read = getline(&password, &buffer_size, stdin);
        password[characters_read - 1] = 0;

        int zip_error;
        zip_t *archive = zip_open(file_name, ZIP_CREATE, &zip_error);

        switch (zip_error) {
        case ZIP_ER_MEMORY:
                fprintf(stderr, "Could not allocate enough memory\n");
                goto cleanup_password;
        case ZIP_ER_NOZIP:
                fprintf(stderr, "Input file is not a zip!\n");
                goto cleanup_password;
        }

        size_t commad_line_size = MAX_COMMAND_LENGTH;
        while (true) {
                fprintf(stderr, "> ");
                char command_line[MAX_COMMAND_LENGTH];

                char *buffer = command_line;
                getline(&buffer, &commad_line_size, stdin);

                struct string_node *list_start = NULL;
                size_t list_len = 0;

                char *current_ptr = command_line;

                char *start = NULL;
                char previous = '\0';

                bool multi_word = false;

                while (true) {
                        char current = *current_ptr;

                        if (current == '\0') {
                                break;
                        } else if (current_ptr != command_line && previous != ' ' && (((multi_word && current == '"') || (!multi_word && current == ' ') || current == '\n')) && start != NULL) {
                                char *end = current_ptr;

                                size_t buffer_len = end - start;
                                char *text = (char*)malloc(buffer_len + 1);
                                memcpy(text, start, buffer_len);
                                text[buffer_len] = '\0';

                                struct string_node *new_node = string_node_new(text, buffer_len);

                                if (list_start == NULL) {
                                        list_start = new_node;
                                } else {
                                        struct string_node *last = list_start;
                                        while (last->next)
                                                last = last->next;

                                        last->next = new_node;
                                }

                                list_len += 1;
                                start = NULL;
                        } else if (current == '"') {
                                multi_word = true;
                        } else if (current != ' ' && start == NULL) {
                                start = current_ptr;
                        }

                        current_ptr += 1;
                        previous = current;
                }

                if (list_start != NULL) {
                        char *cmd = list_start->value;
                        if (strcmp("help", cmd) == 0) {
                                fprintf(stderr, "Avaiable commands:\n\t- help\n\t- get name\n\t- set service entry1 entry2 entry3\n");
                        } else if (strcmp("get", cmd) == 0) {
                                if (list_len == 1) {
                                        fprintf(stderr, "Please provide a section. Example: get github\n");
                                } else {
                                        char *section = list_start->next->value;

                                        zip_file_t *zip_file = zip_fopen_encrypted(archive, section, ZIP_FL_UNCHANGED, password);
                                        zip_error_t *error = zip_get_error(archive);

                                        int zip_error_code = zip_error_code_zip(error);

                                        if (zip_file == NULL) {
                                                if (zip_error_code == 27)
                                                        fprintf(stderr, "Wrong password!\n");
                                                else
                                                        fprintf(stderr, "Could not open file!\n");
                                        } else {
                                                zip_stat_t stat;
                                                zip_stat_index(archive, zip_name_locate(archive, section, ZIP_FL_ENC_GUESS), ZIP_FL_ENC_GUESS, &stat);

                                                size_t buffer_sizte = stat.size;
                                                char *buffer = (char*)malloc(buffer_size + 1);
                                                zip_fread(zip_file, buffer, buffer_size);
                                                buffer[buffer_size] = '\0';

                                                fprintf(stderr, "Section %s:\t%s\n", section, buffer);

                                                free(buffer);
                                                zip_fclose(zip_file);
                                        }
                                }
                        } else {
                                print_unknown_command();
                        }
                        string_node_free_self_and_following(list_start);
                } else {
                        print_unknown_command();
                }
        }

        zip_close(archive);

cleanup_password:
        /* clear the password buffer */
        memset(password, 0, buffer_size);
        free(password);
        return 0;
}

