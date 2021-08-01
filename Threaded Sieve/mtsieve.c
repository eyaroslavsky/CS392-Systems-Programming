/*Edward Yaroslavsky and Jared Follet
I pledge my honor that I have abided by the Stevens Honor System.
*/

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <getopt.h>
#include <ctype.h>
#include <limits.h>
#include <sys/sysinfo.h>

typedef struct arg_struct {
    int start;
    int end;
} thread_args;

int total_count = 0;
pthread_mutex_t lock;

bool contains3(int prime) {
	int count = 0;
	int temp = prime;
	while (temp != 0) {
		temp /= 10;
		count++;
	}
	char buf[count+2];
	sprintf(buf, "%d", prime);
	buf[count+1] = '\0';

	int num3s = 0;
	for (int i = 0; i < count; i++) {
		if (buf[i] == '3') {
			num3s++;
		}
	}

	if (num3s >= 2) {
		return true;
	}
	return false;
}

void *sieve(void *ptr) {
	thread_args *thread = (thread_args *)ptr; 
	int start = thread->start;
	int end = thread->end;

	bool *low_primes = (bool *)malloc(end * sizeof(bool));
	for (int i = 2; i < end; i++) {
	    low_primes[i] = true;
	}

	for (int x = 2; x <= sqrt(end); x++) {
	    if (low_primes[x]) {
	       	for (int y = x*x; y <= end; y+=x) {
	        	low_primes[y] = false;
	        }
	    }
	}

	bool *high_primes = (bool *)malloc((end - start + 1) * sizeof(bool));
	for (int i = 0; i < end - start + 1; i++) {
		high_primes[i] = true;
	}

	for (int p = 2; p < end; p++) {
		if (low_primes[p]) {
			int i = ceil((double)(start)/p) * p - start;
			if (start <= p) {
				i = i + p;
			}
			for (; i < end - start + 1; i+=p) {
				high_primes[i] = false;
			}
		}
	}

	free(low_primes);

	int retval;
	for (int i = 0; i < end - start + 1; i++) {
		if (high_primes[i] && contains3(start + i)) {
		    if ((retval = pthread_mutex_lock(&lock)) != 0) {
		        fprintf(stderr, "Warning: Cannot lock mutex. %s.\n", strerror(retval));
		    }
			total_count++;
			if ((retval = pthread_mutex_unlock(&lock)) != 0) {
		        fprintf(stderr, "Warning: Cannot unlock mutex. %s.\n", strerror(retval));
		    }
		}
	}

	free(high_primes);
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 7) {
		printf("Usage: %s -s <starting value> -e <ending value> -t <num threads>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int opt;
	bool sflag = false, eflag = false, tflag = false;
	int sNum=0;
	int eNum=0;
	int tNum=0;
	char *end;
	int num_procs = get_nprocs();

	opterr = 0;
	while ((opt = getopt(argc, argv, "s:e:t:")) != -1) {
		switch (opt) {
			case 's':
				sflag = true;
				for (int i = 0; i < strlen(optarg); i++) {
					if (!isdigit(optarg[i])) {
						fprintf(stderr, "Error: Invalid input '%s' received for parameter '-%c'.\n", optarg, opt);
						return EXIT_FAILURE;
					}
				}					
				if (strtol(optarg, &end, 10) > INT_MAX) {
					fprintf(stderr, "Error: Integer overflow for parameter '-%c'.\n", opt);
					return EXIT_FAILURE;
				}			
				sNum = atoi(optarg);
				opterr = 0;
				break;
			case 'e':
				eflag = true;
				for (int i = 0; i < strlen(optarg); i++) {
					if (!isdigit(optarg[i])) {
						fprintf(stderr, "Error: Invalid input '%s' received for parameter '-%c'.\n", optarg, opt);
						return EXIT_FAILURE;
					}
				}
				if (strtol(optarg, &end, 10) > INT_MAX) {
					fprintf(stderr, "Error: Integer overflow for parameter '-%c'.\n", opt);
					return EXIT_FAILURE;
				}
				eNum = atoi(optarg);
				break;
			case 't':
				tflag = true;
				for (int i = 0; i < strlen(optarg); i++) {
					if (!isdigit(optarg[i])) {
						fprintf(stderr, "Error: Invalid input '%s' received for parameter '-%c'.\n", optarg, opt);
						return EXIT_FAILURE;
					}
				}
				if (strtol(optarg, &end, 10) > INT_MAX) {
					fprintf(stderr, "Error: Integer overflow for parameter '-%c'.\n", opt);
					return EXIT_FAILURE;
				}
				tNum = atoi(optarg);
				break;
			case '?':
				if (optopt == 'e' || optopt == 's' || optopt == 't') {
					fprintf(stderr, "Error: Option -%c requires an argument.\n", optopt);
				}
				else if (isprint(optopt)) {
					fprintf(stderr, "Error: Unknown option '-%c'.\n", optopt);
				}
				else {
					fprintf(stderr, "Error: Unknown option character '\\x%x'.\n", optopt);
				}
				return EXIT_FAILURE;
		}
	}

	if (argv[optind] != NULL) {
		fprintf(stderr, "Error: Non-option argument '%s' supplied.\n", argv[optind]);
		return EXIT_FAILURE;
	}

	else if (!sflag) {
		fprintf(stderr, "Error: Required argument <starting value> is missing.\n");
		return EXIT_FAILURE;
	}

	else if (sNum < 2) {
		fprintf(stderr, "Error: Starting value must be >= 2.\n");
		return EXIT_FAILURE;
	}

	else if (!eflag) {
		fprintf(stderr, "Error: Required argument <ending value> is missing.\n");
		return EXIT_FAILURE;
	}

	else if (eNum < 2) {
		fprintf(stderr, "Error: Ending value must be >= 2.\n");
		return EXIT_FAILURE;
	}

	else if (eNum < sNum) {
		fprintf(stderr, "Error: Ending value must be >= starting value.\n");
		return EXIT_FAILURE;
	}

	else if (!tflag) {
		fprintf(stderr, "Error: Required argument <num threads> is missing.\n");
		return EXIT_FAILURE;
	}

	else if (tNum < 0) {
		fprintf(stderr, "Error: Number of threads cannot be less than 1.\n");
		return EXIT_FAILURE;
	}

	else if (tNum > 2 * num_procs) {
		fprintf(stderr, "Error: Number of threads cannot exceed twice the number of processors(%d).\n", num_procs);
		return EXIT_FAILURE;
	}

	int nums = eNum - sNum + 1;
	if (tNum > nums) {
		tNum = nums;
	}

	int thread_count = nums / tNum;
	int remainder = nums - (thread_count * tNum);
	int ranges[tNum];
	for (int i = 0; i < tNum; i++) {
		ranges[i] = thread_count;
	}
	int index = 0;
	while (remainder != 0) {
		ranges[index]++;
		index++;
		remainder--;

		if (index == tNum) {
			index = 0;
		}
	}

	printf("Finding all prime numbers between %d and %d.\n", sNum, eNum);
	if (tNum == 1) {
		printf("%d segment:\n", tNum);
	}
	else {
		printf("%d segments:\n", tNum);
	}

	int retval;
	if ((retval = pthread_mutex_init(&lock, NULL)) != 0) {
        fprintf(stderr, "Error: Cannot create mutex. %s.\n", strerror(retval));
        return EXIT_FAILURE;
    }

	pthread_t threads[tNum];
    thread_args targs[tNum];

    int counter = sNum;
    for (int i = 0; i < tNum; i++) {
	    targs[i].start = counter;
	    targs[i].end = counter + ranges[i] - 1;
	    counter = targs[i].end + 1;

	    printf("   [%d, %d]\n", targs[i].start, targs[i].end);

	    if ((retval = pthread_create(&threads[i], NULL, sieve, (void *)(&targs[i]))) != 0) {
            fprintf(stderr, "Error: Cannot create thread %d. %s.\n", i+1, strerror(retval));
            break;
        }
    }

    for (int i = 0; i < tNum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Warning: Thread %d did not join properly.\n", i + 1);
        }
    }

    if ((retval = pthread_mutex_destroy(&lock)) != 0) {
        fprintf(stderr, "Error: Cannot destroy mutex. %s.\n", strerror(retval));
        return EXIT_FAILURE;
    }

    printf("Total primes between %d and %d with two or more '3' digits: %d\n", sNum, eNum, total_count);

	return EXIT_SUCCESS;
}