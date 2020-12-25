#include <stdio.h>
#include <unistd.h>

#include "error_codes.h"

int main(int argc, const char *argv[])
{
        if (argc < 2) {
                fprintf(stderr, "Please provide an input file. (Syntax: zpwm file.zip)\n");
                return ERROR_NO_INPUT_FILE;
        }

        const char *file_name = argv[1];
        fprintf(stderr, "Operating on file: %s\n", file_name);

        if (!access(file_name, R_OK | W_OK)) {
                fprintf(stderr, "Could not access input file!\n");
                return ERROR_CANNOT_ACCESS_INPUT_FILE;
        }

        return 0;
}

