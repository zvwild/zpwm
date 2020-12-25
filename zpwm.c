#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error_codes.h"

#define MAX_PASSWORD_LENGTH 512

int main(int argc, const char *argv[])
{
        if (argc < 2) {
                fprintf(stderr, "Please provide an input file. (Syntax: zpwm file.zip)\n");
                return ERROR_NO_INPUT_FILE;
        }

        const char *file_name = argv[1];
        fprintf(stderr, "Operating on file: %s\n", file_name);

        if (access(file_name, R_OK | W_OK) != 0) {
                fprintf(stderr, "Could not access input file!\n");
                return ERROR_CANNOT_ACCESS_INPUT_FILE;
        }

        const unsigned long long BUFFER_SIZE = MAX_PASSWORD_LENGTH + 1;
        char *password = (char*)malloc(BUFFER_SIZE);

        if (!password) {
                fprintf(stderr, "Failed to allocate %llu bytes for the password!\n", BUFFER_SIZE);
                return ERROR_CANNOT_ALLOCATE_PASSWORD_BUFFER;
        }

        fprintf(stderr, "Please enter your password: ");
        scanf("%[^\n]", password);

        /* clear the password buffer */
        memset(password, 0, BUFFER_SIZE);
        free(password);
        return 0;
}

