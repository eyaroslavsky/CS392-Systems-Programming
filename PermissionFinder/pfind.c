/*******************************************************************************
 * Name        : pfind.c Revision
 * Author      : Edward Yaroslavsky
 * Date        : 3/24/20
 * Description : Permission Finder
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
 ******************************************************************************/

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/stat.h>

void display_usage(char *argv) {
	printf("Usage: %s -d <directory> -p <permissions string> [-h]\n", argv);
}

int to_binary(char *permissions) {
    char binaryFormat[strlen(permissions)];
    for (int i = 0; i < strlen(permissions); i++) {
    	*(binaryFormat + i) = (*(permissions + i) == '-' ? '0' : '1');
    }
    *(binaryFormat + strlen(permissions)) = '\0';

    return atoi(binaryFormat);
}

int get_permissions(struct stat *sb) {
	int perms[] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
	char *str = (char *)malloc(9 * sizeof(char) + 1);
    int permission_valid;

    for (int i = 0; i < 9; i += 3) {
        permission_valid = sb->st_mode & perms[i];
        if (permission_valid) {
            *(str + i) = 'r';
        }
        else {
            *(str + i) = '-';
        }
        permission_valid = sb->st_mode & perms[i+1];
        if (permission_valid) {
            *(str + i + 1) = 'w';
        }
        else {
            *(str + i + 1) = '-';
        }
        permission_valid = sb->st_mode & perms[i+2];
        if (permission_valid) {
            *(str + i + 2) = 'x';
        }
        else {
            *(str + i + 2) = '-';
        }
    }
    *(str + 9) = '\0';

    int binary = to_binary(str);
    free(str);

	return binary;
}

void navigate_dir(char* name, int permissions) {
	DIR *dir = opendir(name);
    char path[PATH_MAX];
    struct dirent *de;
	struct stat sb;
	char full_filename[PATH_MAX];
	size_t pathlen = 0;
	realpath(name, path);

	full_filename[0] = '\0';
	if (strcmp(path, "/")) {
		strncpy(full_filename, path, PATH_MAX);
	}
	pathlen = strlen(full_filename) + 1;
	full_filename[pathlen - 1] = '/';
	full_filename[pathlen] = '\0';

    if (dir == NULL) {
        fprintf(stderr, "Error: Cannot open directory '%s'. Permission denied.\n", name);
    }
    else {
        while ((de = readdir(dir)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
                continue;
            }
            strncpy(full_filename + pathlen, de->d_name, PATH_MAX - pathlen);
            if (lstat(full_filename, &sb) < 0) {
                fprintf(stderr, "Error: Cannot stat file '%s'.\n", full_filename);
                continue;
            }
            if (de->d_type == DT_DIR) {
                if (get_permissions(&sb) == permissions) {
                    printf("%s\n", full_filename);
                }
                navigate_dir(full_filename, permissions);
            }
            else if (get_permissions(&sb) == permissions) {
                printf("%s\n", full_filename);
            }
        }
    }

	closedir(dir);
}

int main(int argc, char *argv[]) {
    int opt;
    bool d_flag = false;
    bool p_flag = false;

    if (argc < 2 || argc > 6) {
    	display_usage(argv[0]);
    	return EXIT_FAILURE;
    }

    int optindex = 1;
    while ((opt = getopt(argc, argv, ":hd:p:")) != -1) {
    	switch (opt) {
    		case 'd':
    			d_flag = true;
    			optindex+=2;
    			break;
    		case 'p':
    			p_flag = true;
    			break;
    		case 'h':
    			display_usage(argv[0]);
    			return EXIT_SUCCESS;
    		case '?':
    			fprintf(stderr, "Error: Unknown option '%s' received.\n", argv[optindex]);
    			return EXIT_FAILURE;
    	}
    }

    if (!d_flag) {
    	printf("Error: Required argument -d <directory> not found.\n");
    	return EXIT_FAILURE;
    }

    if (!p_flag) {
    	printf("Error: Required argument -p <permissions string> not found.\n");
    	return EXIT_FAILURE;
    }

    char path[PATH_MAX];
	if (realpath(argv[2], path) == NULL) {
		fprintf(stderr, "Error: Cannot stat '%s'. No such file or directory.\n", argv[2]);
		return EXIT_FAILURE;
	}

	DIR *dir;
	if ((dir = opendir(path)) == NULL) {
		fprintf(stderr, "Error: Cannot open directory '%s'. Permission denied.\n", argv[2]);
		return EXIT_FAILURE;
	}
	closedir(dir);

    bool format = false;
    if (strlen(argv[4]) == 9) {
    	format = (argv[4][0] == 'r' || argv[4][0] == '-') &&
    		(argv[4][1] == 'w' || argv[4][1] == '-') &&
    		(argv[4][2] == 'x' || argv[4][2] == '-') &&
    		(argv[4][3] == 'r' || argv[4][3] == '-') &&
    		(argv[4][4] == 'w' || argv[4][4] == '-') &&
    		(argv[4][5] == 'x' || argv[4][5] == '-') &&
    		(argv[4][6] == 'r' || argv[4][6] == '-') &&
    		(argv[4][7] == 'w' || argv[4][7] == '-') &&
    		(argv[4][8] == 'x' || argv[4][8] == '-');
    }

    if (!format) {
    	fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", argv[4]);
    	return EXIT_FAILURE;
    }
    
    int perms = to_binary(argv[4]);
    navigate_dir(argv[2], perms);
    
	return EXIT_SUCCESS;
}