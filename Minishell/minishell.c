/**
Edward Yaroslavsky and Jared Follet
I pledge my honor that I have abided by the Stevens Honor System.
*/

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <pwd.h>
#include <wait.h>
#include <setjmp.h>

#define BRIGHTBLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

sigjmp_buf jmpbuf;
pid_t check;

void catch_signal(int sig) {
	int stat;
	if (waitpid(check, &stat, WNOHANG)) {
		write(STDOUT_FILENO, "\n", 1);
	}
	siglongjmp(jmpbuf, 1);
}

int getCurrent() {
	char buf[PATH_MAX];
	if (getcwd(buf, PATH_MAX * sizeof(char)) == NULL) {
		fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
		return EXIT_FAILURE;
	}
	printf("[%s%s%s]$ ", BRIGHTBLUE, buf, DEFAULT);
	memset(buf, 0, sizeof(buf));
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
	if (argc != 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	struct sigaction action;
	action.sa_handler = catch_signal;    
	action.sa_flags = SA_RESTART; 

	if (sigaction(SIGINT, &action, NULL) == -1) {        
		fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));        
		return EXIT_FAILURE;    
	}

	sigsetjmp(jmpbuf, 1);
	//getCurrent();

	char input[1024];
	while (true) {
		getCurrent();
		fgets(input, 1023, stdin);
		input[strlen(input) - 1] = '\0';
		if (strcmp(input, "exit") == 0) {
			break;
		}		
		else if (strcmp(input, "cd") == 0 || strcmp(input, "cd ~") == 0 || strcmp(input, "cd ") == 0) {
			uid_t userID = getuid();
			struct passwd *pword;
			if ((pword = getpwuid(userID)) == NULL) {
				fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
				//return EXIT_FAILURE;
				//getCurrent();
				continue;
			}
			char homeDir[PATH_MAX];
			strcpy(homeDir, pword->pw_dir);
			if (chdir(homeDir) == -1) {
				fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", homeDir, strerror(errno));
				//return EXIT_FAILURE;
				//getCurrent();
				continue;
			}
			//getCurrent();
		}
		else if (strncmp(input, "cd ", 3) == 0) {
			uid_t userID = getuid();
			struct passwd *pword;
			if ((pword = getpwuid(userID)) == NULL) {
				fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
				continue;
			}
			char homeDir[PATH_MAX];
			strcpy(homeDir, pword->pw_dir);

			char *new_dir = input;
			new_dir += 3;
			if (strchr(new_dir, ' ') != NULL) {
				fprintf(stderr, "Error: Too many arguments to cd.\n");
				//return EXIT_FAILURE;
				//getCurrent();
				continue;
			}
			if (new_dir[0] == '~') {				
				new_dir += 2;
				char cat = '/';
				strncat(homeDir, &cat, 1);
				strcat(homeDir, new_dir);
				chdir(homeDir);
			}
			else if (chdir(new_dir) == -1) {
				fprintf(stderr, "Error: Cannot change directory to '%s'. %s.\n", new_dir, strerror(errno));
				//return EXIT_FAILURE;
				//getCurrent();
				continue;
			}
			//getCurrent();
		}
		else {
			pid_t pid;
			if ((pid = fork()) < 0) {
				fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
				//return EXIT_FAILURE;
				continue;
			}
			else if (pid == 0) {
				if (sigaction(SIGINT, &action, NULL) == -1) {        
					fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));        
					return EXIT_FAILURE;    
				}

				char arguments[1024][1024];
				int count = 1;
				int tempCount = 0;
				for (int i = 0; i < strlen(input); i++) {
					if (input[i] == ' ' || input[i] == '\0') {
						arguments[count-1][tempCount] = '\0';
						count++;	
						tempCount = 0;			
					}
					else {
						arguments[count-1][tempCount] = input[i];
						tempCount++;
					}
				}

				char **args;
				if ((args = (char **)malloc(count * sizeof(char *))) == NULL) {
					fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
					return EXIT_FAILURE;
				}
				for (int i = 0; i < count; i++) {
					args[i] = arguments[i];
				}		
				char *firstArg = args[0];
				firstArg[strlen(firstArg)] = '\0';		
				args[count] = NULL;

				if ((execvp(firstArg, args)) == -1) {
					fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
					return EXIT_FAILURE;
				}
				
				for (int i = 0; i < count; i++) {
					free(args[i]);
				}
				free(args);
			}
			else {
				check = pid;
				if (sigaction(SIGINT, &action, NULL) == -1) {        
					fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));        
					//return EXIT_FAILURE; 
					continue;   
				}
				int status;
				do {
					pid_t w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
					if (w == -1) {
						fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
						//return EXIT_FAILURE;
						continue;
					}
				} while (!WIFEXITED(status) && !WIFSIGNALED(status));
			}
			//getCurrent();	
		}
		memset(input, 0, sizeof(input));
	}

	return EXIT_SUCCESS;
}

