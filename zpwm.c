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

struct string_node *string_node_new(char *value, size_t len);
void string_node_free_self_and_following(struct string_node *node);

void dump_list(struct string_node *list);
void print_unknown_command(void);

int main(int argc, const char *argv[])
{
        if (argc < 2) {
                fprintf(stderr, "Please provide an input file. (Syntax: zpwm file.zip)\n");
                return ERROR_NO_INPUT_FILE;
        }


        const char *file_name = argv[1];
        fprintf(stderr, "Operating on file: %s\n", file_name);

        size_t password_buffer_size;
        char *password;

prompt_password:
        password_buffer_size = MAX_PASSWORD_LENGTH + 1;
        password = (char*)malloc(password_buffer_size);

        if (!password) {
                fprintf(stderr, "Failed to allocate %zu bytes for the password!\n", password_buffer_size);
                return ERROR_CANNOT_ALLOCATE_PASSWORD_BUFFER;
        }

        fprintf(stderr, "Please enter your password: ");
        size_t characters_read = getline(&password, &password_buffer_size, stdin);
        password[characters_read - 1] = 0;

        if (password[0] == '\0') {
                fprintf(stderr, "Please provide a valid password!\n");
                free(password);
                goto prompt_password;
        }

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
                                fprintf(stderr, "Avaiable commands:\n\t- help\n\t- get name\n\t- set service entry1 entry2 entry3\n\t- exit (quit)\n\t- discard\n");
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

                                                size_t file_buffer_size = stat.size;
                                                char *file_buffer = (char*)malloc(file_buffer_size + 1);
                                                zip_fread(zip_file, file_buffer, file_buffer_size);
                                                file_buffer[file_buffer_size] = '\0';

                                                fprintf(stderr, "Section %s:\n\t%s\n", section, file_buffer);

                                                free(file_buffer);
                                                zip_fclose(zip_file);
                                        }
                                }
                        } else if (strcmp("set", cmd) == 0) {
                                if (list_len == 2) {
                                        fprintf(stderr, "You need to provide at least a section and one entry. Example: set github awesomedude@awesomemail.com crazydev123\n");
                                } else {
                                        struct string_node *section_node = list_start->next;
                                        char *section = section_node->value;

                                        struct string_node *entry_node = section_node->next;

                                        size_t current_len = entry_node->len;
                                        char *entries_buffer = (char*)malloc(current_len);
                                        memcpy(entries_buffer, entry_node->value, current_len);

                                        entry_node = entry_node->next;
                                        while (entry_node) {
                                                entries_buffer = realloc(entries_buffer, current_len + entry_node->len + 1);
                                                entries_buffer[current_len] = ' ';
                                                memcpy(entries_buffer + current_len + 1, entry_node->value, entry_node->len);

                                                current_len += entry_node->len + 1;
                                                entry_node = entry_node->next;
                                        }

                                        entries_buffer = realloc(entries_buffer, current_len + 1);
                                        entries_buffer[current_len] = '\0';

                                        zip_source_t *src = zip_source_buffer(archive, entries_buffer, current_len, 0);
                                        zip_int64_t index = zip_file_add(archive, section, src, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
                                        zip_file_set_encryption(archive, index, ZIP_EM_AES_256, password);

                                        fprintf(stderr, "Section %s updated.\n", section);
                                }
                        } else if (strcmp("exit", cmd) == 0 || strcmp("quit", cmd) == 0) {
                                fprintf(stderr, "Bye!\n");
                                break;
                        } else if (strcmp("discard", cmd) == 0) {
                                zip_unchange_all(archive);
                                fprintf(stderr, "Discarded changes!\n");
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
        memset(password, 0, password_buffer_size);
        free(password);
        return 0;
}

void print_unknown_command(void)
{

        fprintf(stderr, "Unkown command. Try \"help\".\n");
}

void dump_list(struct string_node *list)
{
        while (list) {
                fprintf(stderr, "%s (%zu)\n", list->value, list->len);
                list = list->next;
        }
}

void string_node_free_self_and_following(struct string_node *node)
{
        while (node) {
                struct string_node *current = node;
                node = node->next;

                free(current->value);
                free(current);
        }
}

struct string_node *string_node_new(char *value, size_t len)
{
        struct string_node *result = (struct string_node*)malloc(sizeof(*result));
        result->value = value;
        result->len = len;
        result->next = NULL;
        return result;
}
