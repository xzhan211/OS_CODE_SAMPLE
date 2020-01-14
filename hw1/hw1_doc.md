## HW1
[Shell: Process control in Foreground and Background](https://oscourse.github.io/hw1.html)

####0. multiple processes
Shell is main process, other command runs in shell is another process.

```c
if((pid = fork()) < 0){
	printf("fork error!\n");
	exit(1);
}else if(pid == 0){
	//in child process	
	//exec() command here
}else{
	//in parent process
	//can wait() child here
}
```


####1. Execute commands with multiple arguments.
Use execv(), put all the arguments into arr[], the last one is always "NULL".

```c
char* arr[] = {"ls", "-l", "-R", "-a", NULL};
execv("/bin/ls", arr);
```

####2. Execute commands in either foreground or background mode.
If there is no wait() in parent process, child process can be considered to run in background. In this case, user sees stuff in parent process first. 

If wait() is used in parent process, child process will run first. In this case, user sees stuff in child process first. Let's say "child process is running in foreground".

```c
//forceground case
if(pid == 0){
	...
}else{
	...
	wait(); //child process runs in forceground.
}

//background case
if(pid == 0){
	...
}else{
	...
	//no wait() here, child process runs in background.
}

```

####3. Maintain multiple processes running in background mode simultaneously.
Without wait() in parent process, multiple processes can run in background simultaneously. 

But, once the child process exit() without a wait() from parent process, the child process will become a zombie.

We can use signal mechanism to "kill" zombie process.

```c
/* type definition of "pointer to a function that takes integer argument and returns void */
typedef void Sigfunc(int);

// define signal handler wrapper which contains sigaction()
Sigfunc *install_signal_handler(int signo, Sigfunc *handler){
    struct sigaction act, oact;
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);

    //there is a confliction between sigaction() and getline() if we set act.sa_flags = 0.
    //https://stackoverflow.com/questions/19140892/strange-sigaction-and-getline-interaction
    //act.sa_flags = 0;
    act.sa_flags = SA_RESTART;

    if( sigaction(signo, &act, &oact) < 0 )
        return (SIG_ERR);

    return(oact.sa_handler);
}


//When a child process exits, SIGCHLD will be sent to parent process.
//We set a signal handler to catch SIGCHLD signal which can trigger this int_handler function.
void int_handler(int sig){
    wait(NULL);
}

//last step, register this signal handler at the beginning of main()
int main(){
    install_signal_handler(SIGCHLD, int_handler);
    ...
}
```

####4. List all currently running background jobs using "listjobs" command.
Here I just use global array to record background jobs. Once we get the pid about background job, kill() can be used to check whether the checked process is existing.

If sig is 0, then no signal is sent, but existence and permission checks are still performed; this can be used to check for the existence of a process ID or process group ID that the caller is permitted to signal.
 

```c
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
```

####5. Bring a background process to foreground using the fg command with process ID as argument.

Shell is parent process. Other command processes are child processes. Using wait() in parent process can bring background child process to foreground.

It is good to remember: forceground is the things user can see directly, background is the things user cannot see now.

In our case, we can simply consider PROMPT “sh550> ” is always in forceground. In another word, once user can see the prompt, yes, it is in forceground.

```c
// in parent process
if(strcmp(tokens[0], "fg") == 0)
    waitpid(atoi(tokens[1]), status, 0);
```

####6. The exit command should terminate your shell without orphan and zombie processes.
See code from No.3 issue.


## Addition

We can learn other good code from hw1.

####A. how to malloc one/two dimensions char array with assert.

About assert, this macro can help programmers find bugs in their programs, or handle exceptional cases via a crash that will produce limited debugging output.

If expression is false (i.e., compares equal to zero), assert() prints an error message to standard error and terminates the program by calling abort(3).  

The error message includes the name of the file and function containing the assert() call, the source code line number of the call, and the text of the argument; something like:

*prog: some_file.c:16: some_func: Assertion `val == 0' failed.*

```c
// allocate space for the whole line
assert( (line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);

// allocate space for individual tokens
assert( (tokens = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);
```

####B. analyse tokens from string / dynamic reallocate more space

```c
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
```
####C. use stdin 

```c
FILE *fp; // file struct for stdin
char *line;

// open stdin as a file pointer
assert( (fp = fdopen(STDIN_FILENO, "r")) != NULL);

void read_command(){
    // getline will reallocate if input exceeds max length
    assert( getline(&line, &MAX_LINE_LEN, fp) > -1);
    // return key is also a char "\n"
    if(strlen(line) == 1){
        strcpy(line, "noCommand\n");
    }
    printf("Shell read this line: %s\n", line);
    tokenize(line);
}
```
###D. loop input usable data

```c
int run_command() {
    if (strcmp( tokens[0], EXIT_STR ) == 0)
        return EXIT_CMD;
    return UNKNOWN_CMD;
}

int main(){
    do{
        ...
    }while(run_command() != EXIT_CMD);
}
```

###E. implement "cd" function

```c
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
}
```

###F. implement all shell command

```c
if(pid == 0){
    char* para[tokensSize +1];
    para[tokensSize] = NULL;
    
    for(int i=0; i<tokensSize; i++)
        para[i] = tokens[i];

    char * s1 = "/bin/";
    char * s2 = tokens[0];
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    execv(result, para);
    exit(0);
}
```
