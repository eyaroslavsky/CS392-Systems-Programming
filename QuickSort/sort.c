/*******************************************************************************
 * Name        : sort.c
 * Author      : Edward Yaroslavsky
 * Date        : 2/21/20
 * Description : Uses quicksort to sort a file of either ints, doubles, or
 *               strings.
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quicksort.h"

#define MAX_STRLEN     64 // Not including '\0'
#define MAX_ELEMENTS 1024

typedef enum {
    STRING,
    INT,
    DOUBLE
} elem_t;

void print_usage() {
	printf("Usage: ./sort [-i|-d] [filename]\n"
    "	-i: Specifies the file contains ints.\n"
    "	-d: Specifies the file contains doubles.\n"
    "	filename: The file to sort.\n"
    "	No flags defaults to sorting strings.\n");
}

void display_strings(char **array, const int length)
{
    for (int i = 0; i < length; i++)
    {
        printf("%s\n", *(array + i));
    }
}

void display_ints(int *array, const int length)
{
    for (int i = 0; i < length; i++)
    {
        printf("%d\n", *(array + i));
    }
}

void display_doubles(double *array, const int length)
{
    for (int i = 0; i < length; i++)
    {
        printf("%f\n", *(array + i));
    }
}

/**
 * Basic structure of sort.c:
 *
 * Parses args with getopt.
 * Opens input file for reading.
 * Allocates space in a char** for at least MAX_ELEMENTS strings to be stored,
 * where MAX_ELEMENTS is 1024.
 * Reads in the file
 * - For each line, allocates space in each index of the char** to store the
 *   line.
 * Closes the file, after reading in all the lines.
 * Calls quicksort based on type (int, double, string) supplied on the command
 * line.
 * Frees all data.
 * Ensures there are no memory leaks with valgrind. 
 */
int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
    	print_usage();
    	return EXIT_FAILURE;
    }

    int opt;
    int val = 0;

    while ((opt = getopt(argc, argv, ":if:id")) != -1) {
    	switch (opt) {
    		case 'i':
    			val = 1;
    			break;
    		case 'd':
    			val = 2;
    			break;
    		case '?':
    			printf("Error: Unknown option '%s' received.\n", argv[1]);
    			print_usage();
    			return EXIT_FAILURE;
    	}
    }

    char buf[MAX_ELEMENTS];

    FILE *fp;
    int index = 0;
    int len = 0;

    if (argc == 2) {
    	fp = fopen(argv[1], "r");
    	if (fp == NULL) {
	    	fprintf(stderr, "Error: Cannot open file '%s'. No such file or directory.\n", argv[1]);
	    	return EXIT_FAILURE;
    	}

        char **temp = (char **)malloc(MAX_ELEMENTS * sizeof(char*)); 
        while (fgets(buf, MAX_STRLEN + 2, fp)) {
            char *eoln = strchr(buf, '\n');
            if (eoln == NULL) {
                buf[MAX_STRLEN] = '\0';
            }
            else {
                *eoln = '\0';
            }

            temp[index] = (char *)malloc(MAX_STRLEN * sizeof(char));
            strcpy(*(temp + index), buf);
            index++;
            len++;
        }

        fclose(fp);
        quicksort(temp, len, sizeof(char *), str_cmp);
        display_strings(temp, len);
        for (int i = 0; i <= len; i++) {
            free(temp[i]);
        }
        free(temp);
    }
    else if (argc == 3) {
    	fp = fopen(argv[2], "r");
    	if (fp == NULL) {
	    	fprintf(stderr, "Error: Cannot open file '%s'. No such file or directory.\n", argv[2]);
	    	return EXIT_FAILURE;
	    }

        if (val == 1) {
            int *temp = malloc(MAX_ELEMENTS * sizeof(int));
            while (fgets(buf, MAX_STRLEN + 2, fp)) {
                char *eoln = strchr(buf, '\n');
                if (eoln == NULL) {
                    buf[MAX_STRLEN] = '\0';
                }
                else {
                    *eoln = '\0';
                }

                int num = atoi(buf);
                *(temp + index) = num;
                index++;
                len++;
            }

            fclose(fp);
            quicksort(temp, len, sizeof(int), int_cmp);
            display_ints(temp, len);
            free(temp);
        }
        else {
            double *temp = malloc(MAX_ELEMENTS * sizeof(double));
            while (fgets(buf, MAX_STRLEN + 2, fp)) {
                char *eoln = strchr(buf, '\n');
                if (eoln == NULL) {
                    buf[MAX_STRLEN] = '\0';
                }
                else {
                    *eoln = '\0';
                }

                double num = atof(buf);
                *(temp + index) = num;
                index++;
                len++;
            }

            fclose(fp);
            quicksort(temp, len, sizeof(double), dbl_cmp);
            display_doubles(temp, len);
            free(temp);
        }
    }

    return EXIT_SUCCESS;
}
