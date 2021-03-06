#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

//limits
#define MAX_TOKENS 100
#define MAX_STRING_LEN 100
size_t MAX_LINE_LEN = 10000;
size_t MAX_BACKGROUND_PROS = 200;

// builtin commands
#define EXIT_STR "exit"
#define EXIT_CMD 0
#define UNKNOWN_CMD 99

FILE *fp; // file struct for stdin
char **tokens;
char *line;
int tokensSize;
int* pidArr;
char **commandWithPid;
int arrPos = 0;

/* type definition of "pointer to a function that takes integer argument and returns void */
typedef void Sigfunc(int);


Sigfunc *install_signal_handler(int signo, Sigfunc *handler){
    struct sigaction act, oact;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);

    //https://stackoverflow.com/questions/19140892/strange-sigaction-and-getline-interaction
    //act.sa_flags = 0;
    act.sa_flags = SA_RESTART;

    if( sigaction(signo, &act, &oact) < 0 )
        return (SIG_ERR);

    return(oact.sa_handler);
}

void int_handler(int sig){
    wait(NULL);
}




void initialize()
{

	// allocate space for the whole line
	assert( (line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);

	// allocate space for individual tokens
	assert( (tokens = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);

	// open stdin as a file pointer
	assert( (fp = fdopen(STDIN_FILENO, "r")) != NULL);

    // allocate space for commandWithPid array
	assert( (commandWithPid = malloc(sizeof(char*)*MAX_BACKGROUND_PROS)) != NULL);

    // allocate space for pid array
	assert( (pidArr = malloc(sizeof(int)*MAX_BACKGROUND_PROS)) != NULL);
}

void tokenize (char * string){
	int token_count = 0;
	int size = MAX_TOKENS;
	char *this_token;
	tokensSize = -1;
	while ( (this_token = strsep( &string, " \t\v\f\n\r")) != NULL) {
		tokensSize = token_count;
		if (*this_token == '\0') continue;

		tokens[token_count] = this_token;

		printf("Token %d: %s\n", token_count, tokens[token_count]);

		token_count++;

		// if there are more tokens than space ,reallocate more space
		if(token_count >= size){
			size*=2;
			assert ( (tokens = realloc(tokens, sizeof(char*) * size)) != NULL);
		}
	}
}

void read_command(){
	// getline will reallocate if input exceeds max length
	assert( getline(&line, &MAX_LINE_LEN, fp) > -1);
	if(strlen(line) == 1){
		strcpy(line, "noCommand\n");
	}
	printf("Shell read this line: %s\n", line);
	tokenize(line);
}

int run_command() {
	if (strcmp( tokens[0], EXIT_STR ) == 0)
		return EXIT_CMD;
	return UNKNOWN_CMD;
}




void listjobs(){
    char * status;
    for(int i=0; i<arrPos; i++){
        if(kill(pidArr[i], 0) == 0)
            status = "RUNNING";
        else
            status = "FINISHED";
        printf("%s Status: %s\n", commandWithPid[i], status);
    }
}



int main(){

    install_signal_handler(SIGCHLD, int_handler);
    initialize();

	do {
		printf("sh550> ");
		read_command();
		pid_t pid = -1;
		int * status;

		if(strcmp(tokens[0], "cd") == 0){
			char s[100] = {'\0'};
			if(tokens[1][0] == '/'){
				strcat(s, tokens[1]);
			}else{
				getcwd(s, 100);
				strcat(s, "/");
				strcat(s, tokens[1]);
			}
			chdir(s);
		}else if(strcmp(tokens[0], "listjobs") == 0){
            listjobs();
        }else if(strcmp(tokens[0], "fg") == 0){
            waitpid(atoi(tokens[1]), status, 0);
        }else{
            pid = fork();
            if(pid < 0){
			    printf("fork failed\n");
			    exit(1);
            }
        }

        if(pid >= 0){

            int flag = strcmp(tokens[tokensSize-1], "&");

            if(pid == 0){
                //printf("In child\n");
                int loopSize = 0;
                if(flag == 0){
                    loopSize = tokensSize-1;
                    sleep(30);
                }else
                    loopSize = tokensSize;
                char* para[loopSize+1];
                para[loopSize] = NULL;

                for(int i=0; i<loopSize; i++)
                    para[i] = tokens[i];

                char * s1 = "/bin/";
                char * s2 = tokens[0];
                char *result = malloc(strlen(s1)+strlen(s2)+1);
                strcpy(result, s1);
                strcat(result, s2);
                execv(result, para);
                exit(0);
            }else{
                //printf("In parent, flag == %d\n", flag);
                if(flag != 0){
                    waitpid(pid, status, 0);
                }else{
                    pidArr[arrPos] = pid;
                    int length = snprintf( NULL, 0, "%d", pid);
                    char* str = malloc( length + 1 );
                    snprintf( str, length + 1, "%d", pid);
                    char* temp;
                    assert( (temp = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);
                    strcpy(temp, line);
                    strcat(temp, " with PID ");
                    strcat(temp, str);
                    strcat(temp, "\0");
                    commandWithPid[arrPos] = temp;
                    free(str);
                    arrPos++;
                    if(arrPos == MAX_BACKGROUND_PROS)
                        arrPos = 0;
                }
                //printf("father last line!\n");
            }
        }
	} while( run_command() != EXIT_CMD);
	return 0;
}
