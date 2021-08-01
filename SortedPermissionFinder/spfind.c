/**
Edward Yaroslavsky and Nicholas Szegheo
I pledge my honor that I have abided by the Stevens Honor System.
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <dirent.h>

void display_usage(char *argv) {
	printf("Usage: %s -d <directory> -p <permissions string> [-h]\n", argv);
}

int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 6) {
		display_usage(argv[0]);
		return EXIT_FAILURE;
	}

	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0) {
			display_usage(argv[0]);
			return EXIT_SUCCESS;
		}
	}

	int pfind_to_sort[2], sort_to_parent[2];
	if (pipe(pfind_to_sort) < 0) {
		fprintf(stderr, "Error: Cannot create pipe from pfind to sort. %s.\n", strerror(errno));
        return EXIT_FAILURE;
	}
	if (pipe(sort_to_parent) < 0) {
		fprintf(stderr, "Error: Cannot create pipe from sort to spfind. %s.\n", strerror(errno));
        return EXIT_FAILURE;
	}

	pid_t pid[2];
	if ((pid[0] = fork()) == 0) {
		close(sort_to_parent[0]);
		close(sort_to_parent[1]);

		if (dup2(pfind_to_sort[1], STDOUT_FILENO) < 0) {
			fprintf(stderr, "Error: Failed to dup2 1.\n");
			close(pfind_to_sort[1]);
			close(pfind_to_sort[0]);
			return EXIT_FAILURE;
		}

		execv("pfind", argv);

		close(pfind_to_sort[1]);
		close(pfind_to_sort[0]);
	}

	if ((pid[1] = fork()) == 0) {
		close(pfind_to_sort[1]);
		close(sort_to_parent[0]);
		if (dup2(pfind_to_sort[0], STDIN_FILENO) < 0) {
			fprintf(stderr, "Error: Failed to dup2 2.\n");
			close(sort_to_parent[1]);
			close(pfind_to_sort[0]);
			return EXIT_FAILURE;
		}
		if (dup2(sort_to_parent[1], STDOUT_FILENO) < 0) {
			fprintf(stderr, "Error: Failed to dup2 3.\n");
			close(sort_to_parent[1]);
			close(pfind_to_sort[0]);
			return EXIT_FAILURE;
		}

		execlp("sort", "sort", NULL);

		close(pfind_to_sort[0]);
		close(sort_to_parent[1]);
	}

	close(sort_to_parent[1]);
	if (dup2(sort_to_parent[0], STDIN_FILENO) < 0) {
		fprintf(stderr, "Error: Failed to dup2 4.\n");
		close(pfind_to_sort[0]);
		close(pfind_to_sort[1]);
		close(sort_to_parent[0]);
		return EXIT_FAILURE;
	}

	close(pfind_to_sort[0]);
	close(pfind_to_sort[1]);
	close(sort_to_parent[0]);

	char buffer[1024];
	ssize_t count = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
	if (count == -1) {
		perror("read()");
		exit(EXIT_FAILURE);
	}
	buffer[count] = '\0';

	int lines = 0;
	for (int i = 0; i < count; i++) {
		if (buffer[i] == '\n') {
			lines++;
		}
	}

	int status;
    if(waitpid(pid[0], &status, WUNTRACED | WCONTINUED) == -1){
        fprintf(stderr, "Error: waitpid() failed for pfind.\n");
        return EXIT_FAILURE;
    }
    int status2;
    if(waitpid(pid[1], &status2, WUNTRACED | WCONTINUED) == -1){
        fprintf(stderr, "Error: waitpid() failed for sort.\n");
        return EXIT_FAILURE;
    }

	printf("%s", buffer);

	if (WEXITSTATUS(status) == 0) {
		printf("Total matches: %d\n", lines);
	}	

	return EXIT_SUCCESS;
}
