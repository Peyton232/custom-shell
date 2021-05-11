#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <pwd.h>
#include <fcntl.h>
#include <stdio.h>

#define MAXBYTES 512
//#define stdout  (&_iob[STDOUT_FILENO])
//extern struct _IO_FILE *stdout;			//what the fuck just work

//prototypes
void LineParser(char* line);
void RemoveOuterWhitespace(char* line);
void RunFunctions(char** functions, int functionCount);

//helper functions
char** Split(char string[], int* count);
void sig_handler(int signo);
//void FreeMainMemory();

//global variables
char** history;
int historyIndex;
char** pathVar;
int pathCounter;


//home directroy
const char *homedir;


pid_t pid;
bool exitStatus = false;

//home directory
const char *homedir;

int main(int argc, char *argv[]) {

    //Allocates memory for the myhistory command history
    historyIndex = 0;
	history = (char**)(malloc(sizeof(char*) * 20));
	for(int i = 0; i < 20; i++)
	{
	    history[i] = (char*)(malloc(sizeof(char) * MAXBYTES));
	}

	//initializes all entries of myhistory to a blank space, to know if the entry hasn't been made yet.
	for(int i = 0; i < 20; i++)
	{
		strcpy(history[i], " ");
	}

    //this section adds the program directory and all the normal path variable directories in to the shell's path variable.
    //The path variable is maintained in our shell
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//get home directory
	if ((homedir = getenv("HOME")) == NULL) {
		homedir = getpwuid(getuid())->pw_dir;
	}

    pathCounter = 0; //used to keep track of how many path variables are present, so pathVar can be resized if necessary
    pathVar = (char**)(malloc(sizeof(char*) * 100)); //an arbitrarily large array to store path variables in
    for(int i = 0; i < 100; i++)
        pathVar[i] = (char*)(malloc(sizeof(char) * 100)); // an arbitarily large string, to store directory paths in.

    char tempStr[1024];
    strcpy(tempStr, getenv("PATH"));


    //breaks up the line in to multiple strings (commands), delimited by ;
    char* token;

    //gets the first command
    token = strtok(tempStr, ":");
    pathVar[pathCounter] = token;

    //Remove the PATH= from the last pathVar added
    pathVar[pathCounter] += 5;
    pathCounter++;

    while(token != NULL)
    {
        token = strtok(NULL, ":");

        if(token == NULL)
            break;

        pathVar[pathCounter] = token;
        pathCounter++;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//General flowpath
/*
 * Main reads things in to the parser, line by line
 * the line parser function delimits functions/arguments based on newlines and semicolons and sends those functions to be ran (in RunFunctions())
 * RunFunctions formats the command/arguments to be used in an exec function call. Each command is ran in the order given, sequentially.
 * */

//Main is used to decide which mode (interactive/batch) to start in, and sends lines of commands to be parsed/ran

    if(argc == 1) //start in interactive mode
    {
        printf("Interactive mode started\n"); //DELETEME
        while(1)
        {
            printf("<PROMPT>: ");
            char* line = NULL;
            size_t size = 0;
            getline(&line, &size, stdin);

            LineParser(line);
            free(line);

            // if exitStatus equals true, exit program
            if (exitStatus == true)
            {
                exit(0);
            }
        }
        return 0;
    }else if(argc == 2) //start in batch mode
    {
        printf("Batch mode started\n"); //DELETEME

        //open file stream, read in first line
        FILE* fp;
        char* line = NULL;
        ssize_t read;
        size_t len = 0;

        fp = fopen(argv[1], "r");

        if(fp == NULL)
        {
            printf("%s does not exist. Exiting program\n", argv[1]);
            //FreeMainMemory();
            return 0;
        }
        else
        {
            printf("%s opened successfully.\n", argv[1]);
        }

        //take each line, pull out the first word, decide which function to call, and send it the arguments (delimited by ; or \n)

        //for each line in the input file, execute the commands in it (delimited by ; and \n)
        int counter = 1; //DELETEME

        while((read = getline(&line, &len, fp)) != -1)
        {
            //check for the line being empty, and skip trying to call it as a function
            if(!strcmp(line, "\n"))
                continue;

            //This function separates and calls all of the commands for each line
            LineParser(line);
            counter++; //DELETEME
            //free(line);
        }

        //FreeMainMemory();
        fclose(fp);

        // if exitStatus equals true, exit program
        if (exitStatus == true)
        {
            exit(0);
        }

        return 0;
    }
    else
    {
        //print error message that says the function has too many arguments, and exit program/return
        printf("The program has too many input arguments. Exiting program.\n");
        //FreeMainMemory();
        return 0;
    }
}

//This function is used to create a queue of commands to run, And then call each of them sequentially
void LineParser(char* line)
{
    //finds how many separate command sections there are in this line of code
    //checks each letter
    int commandIndex = 0;
    for(char *c = line; *c; c++)
    {
        if(*c == ';' || *c == '\n' || *c == '\0')
        {
            //increments to start concatenating the next command
            //adds an index for every semicolon
            if(*c == ';')
            {
                commandIndex++;
            }
        }
        //add for every semicolon, unless it's at the end.
    }
    commandIndex++; //adds an index for the end of the string, in case there's no semicolon at the end

    //if there was a semicolon at the end of the line, this prevents the semicolon and the end of the string from being double counted
    if(line[strlen(line)-2] == ';')
        commandIndex--;

    //printf("The number of commands is %i\n\n", commandIndex);

    //Now that we know how many separate commands there are, we can define our list of commands for this line
    char** queue[commandIndex];

    //breaks up the line in to multiple strings (commands), delimited by ;
    char delimiters[] = ";";
    char* token;
    int currentIndex = 0;

    //gets the first command
    token = strtok(line, delimiters);
    queue[currentIndex] = (char**)token;
    currentIndex++;

    //gets remaining commands
    while(token != NULL)
    {
        token = strtok(NULL, delimiters);
        queue[currentIndex] = (char**)token;
        currentIndex++;
    }

    //strips all leading/trailing whitespaces from the separated commands, for consistency
    for(int i = 0; i < commandIndex; i++)
        RemoveOuterWhitespace((char*)queue[i]);

    //This runs each of the parsed functions
    RunFunctions((char**)&queue, commandIndex);
}

//this function takes a string and removes the leading/trailing whitespaces
void RemoveOuterWhitespace(char* line)
{
    if(line == NULL)
        return;
    else if(strlen(line) == 0)
        return;

    //initializes default values for these indexes
    int firstIndex = 0;
    int lastIndex = strlen(line)-1;

    //find first non whitespace character
    int index = 0;
    while(line[index] == ' ')
    {
        index++;
    }
    firstIndex = index;

    //find last non whitespace character
    for(int i = 0; i < strlen(line); i++)
    {
        if(!isspace(line[i]))
            lastIndex = i;
    }

    //replaces the original string with a string that has no leading or trailing whitespaces
    strncpy(line, line+firstIndex, lastIndex-firstIndex+1);
    
    // if line equals exit, set exitStatus to true
    if (!(strcmp(line, "exit")))
    {
        exitStatus = true;
    }

    //gets rid of the extra letters at the end of the string
    line[lastIndex-firstIndex+1] = '\0';

    return;
}

//this function takes a list of string that represent console commands and input arguments, and it runs each command sequentially.
void RunFunctions(char** functions, int functionCount) {
    //a fork is made, and an exec function is called to run that function.
    //for each function, call split on it
    //split formats the function in to a char**, where inputArgs[0] is the program name, The last argument is NULL, and everything in between is input arguments
    //for the function.
    //the fork is used so the exec function doesn't kill the current process
    //the parent process waits for the child process (that is running exec) to finish before it moves on to the next one.

    //store stdout
    int stdout_save;
    stdout_save = dup(STDOUT_FILENO); /* save */

    for (int i = 0; i < functionCount; i++) {
        //this saves the currently running command to the myhistory list
        strcpy(history[historyIndex], functions[i]);
        historyIndex++;
        historyIndex = historyIndex % 20;

        //this code block formats the function/arguments in a way the exec function requires.
        int count = 0;
        char **inputArgs = Split(functions[i], &count);

        //This code block is used to add/remove/display path variable information. When this section is applicable, it skips trying to run a normal command via exec()
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        bool skipFunctionCall = false; //this is used to bypass the normal function calling, when required. For now, it's being used for when path is called.

        //this section prints through all of the 20 most recent commands, most recent to oldest.
        if (!(strcmp(inputArgs[0], "myhistory"))) {
            int tempHistoryIndex = historyIndex;
            for (int j = 0; j < 20; j++) {
                if (history[tempHistoryIndex][0] == ' ') {
                    tempHistoryIndex--;
                    if (tempHistoryIndex < 0)
                        historyIndex = 19;
                    continue;
                }
                printf("%s\n", history[tempHistoryIndex]);
                tempHistoryIndex--;
                if (tempHistoryIndex < 0)
                    tempHistoryIndex = 19;
            }
            skipFunctionCall = true;
        }

        //if the path command was called
        if (!(strcmp(inputArgs[0], "path"))) {

            //if only path was entered, print the path variable.
            if (count == 1) {
                printf("PATH=");
                if (pathCounter == 0)
                    printf("\n");
                for (int j = 0; j < pathCounter; j++) {
                    if (j < (pathCounter - 1))
                        printf("%s:", pathVar[j]);
                    else
                        printf("%s\n", pathVar[j]);
                    fflush(stdout);
                }
            } else if (count <= 3) //if an additional argument was entered, verify if it's + or -
            {
                if (count == 2) {
                    printf("Incorrect path format entered. Enter \"path +/- (path directory)\"\n");
                    continue;
                }

                //if additional argument is a +, add the next input argument to the path variable.
                if (inputArgs[1][0] == '+') {
                    strcpy(pathVar[pathCounter], inputArgs[2]);
                    pathCounter++;
                    printf("%s added\n", pathVar[pathCounter - 1]);
                }
                    //if the additional argument is a -, remove the next input argument from the path variable if it's present
                else if (inputArgs[1][0] == '-') {

                    //this section scans through the current path variable information to find the specified path to remove.
                    bool wasFound = false;
                    for (int j = 0; j < pathCounter; j++) {
                        if (!(strcmp(pathVar[j], inputArgs[2]))) {
                            wasFound = true;

                            //this section swaps the thing to be deleted to the end of the array, to maintain the order of the path variable.
                            pathVar[pathCounter] = pathVar[j];
                            for (int k = 0; k < (pathCounter - j + 1); k++) {
                                pathVar[j] = pathVar[j + 1];
                            }
                            pathCounter--;

                            printf("%s deleted\n", pathVar[pathCounter]);
                            break;
                        }
                    }
                    if (wasFound == false) {
                        printf("Path was not found.\n");
                    }
                } else {
                    printf("Incorrect path format entered. Enter \"path +/- (path directory)\"\n");
                }

            }

            skipFunctionCall = true;
        }

        if (!(strcmp(inputArgs[0], "exit"))) {
            skipFunctionCall = true;
            exitStatus = true;
        }

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        //cd functionality
        //if cd was called
        if ((strcmp(inputArgs[0], "cd")) == 0) {
            //check cwd
            //char s[100];
            // printing current working directory
            //printf("%s\n", getcwd(s, 100));
            //process cd arguments command.
            //int ret;
            //char *newDir;						// new directory variable for cd
            //char buffer[50];

            /*
            newDir = strtok(inputArgs[0]," ");					// newDir = "cd"
            newDir = strtok(NULL,"");							// newDir = "Path"
            chdir(newDir);
            */

            //if no arguments go to home
            if (count > 1) {
                //ret = chdir(inputArgs[1]);
                //getcwd(inputArgs[1],sizeof(inputArgs[1]));

                chdir(inputArgs[1]);

            } else if (count == 1) {
                // ret = chdir (" ");
                // getcwd(" ", 1);

                chdir(homedir);
            }

            // if(ret!=0)
            // {
            // perror("Error while process the cd command: ");
            // }


            //skip execvp later
            skipFunctionCall = true;

        }

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //another process is made
        //the child runs the requested function.
        //the parent waits for the child function to finish running before continuing.
        pid = fork();
        if (pid == 0) //the child process
        {
            setpgid(getpid(), getpid());

            if (skipFunctionCall == false) {
                bool noRun = false;
                if (count == 1) {
                    noRun = true;
                }

                if (noRun == true) {
                    if (skipFunctionCall == false) {
                        //for each directory, check if there's a function by that name.
                        //if not, print the "command not found"
                        bool runSuccess = false;
                        for (int j = 0; j < pathCounter; j++) {
                            char tempStr[2056];
                            strcpy(tempStr, pathVar[j]);
                            if (!strcmp(tempStr, " ")) //this is what path variables are when they are blanked/removed
                                continue;
                            strcat(tempStr, "/");
                            strcat(tempStr, inputArgs[0]);

                            if (execvp(tempStr, inputArgs) != -1) {
                                runSuccess = true;
                                break;
                            }
                        }

                        if (runSuccess == false)
                            printf("%s command not found\n", inputArgs[0]);

                    }

                    //exits early, to prevent the child process from continuing and running a lot of extra loop iterations.
                    //this only runs if the exec function fails to run.
                    _exit(0);

                }

                if ((skipFunctionCall == false) && (inputArgs[1][0] == '<')) {
                    //get from input file
                    bool fail = false;

                    //open file
                    FILE *fptr;
                    fptr = fopen(inputArgs[2], "r");

                    //if opened correctly then proceed
                    if (fptr == NULL) {
                        fail = true;
                        printf("error openeing file: %s\n", inputArgs[2]);
                    }

                    //attatch file content to inputArguments
                    char *line_buf = NULL;
                    size_t line_buf_size = 0;
                    int line_count = 0;
                    ssize_t line_size;

                    //get first line
                    line_size = getline(&line_buf, &line_buf_size, fptr);

                    //test
                    //printf("\n\nmy contents are: %s\n\n", line_buf);

                    //assign value to input arguments and delete argumebnts 3
                    strcpy(inputArgs[1], line_buf);
                    inputArgs[2] = NULL;
                    count = 2;

                    /* Free the allocated line buffer */
                    //free(line_buf);

                    /* Close the file now that we are done with it */
                    fclose(fptr);

                    //if no error then run command
                    if (fail == false) {
                        bool runSuccess = false;
                        for (int j = 0; j < pathCounter; j++) {
                            char tempStr[2056];
                            strcpy(tempStr, pathVar[j]);
                            if (!strcmp(tempStr, " ")) //this is what path variables are when they are blanked/removed
                                continue;
                            strcat(tempStr, "/");
                            strcat(tempStr, inputArgs[0]);

                            if (execvp(tempStr, inputArgs) != -1) {
                                runSuccess = true;
                                break;
                            }
                        }

                        if (runSuccess == false) {
                            printf("%s command not found\n", inputArgs[0]);
                        }
                    }

                } else if ((skipFunctionCall == false) && (inputArgs[1][0] == '>')) {
                    //output to a file

                    //open/create file for ouput to
                    int fd = open(inputArgs[2], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

                    //if these run they broke the code beyond repair and I don't know why
                    dup2(fd, 1);   // make stdout go to file
                    dup2(fd, 2);   // make stderr go to file - maybe don't do

                    close(fd);     // fd no longer needed

                    //delete arg 1 and 2
                    inputArgs[1] = NULL;
                    inputArgs[2] = NULL;

                    //run command
                    bool runSuccess = false;
                    for (int j = 0; j < pathCounter; j++) {
                        char tempStr[2056];
                        strcpy(tempStr, pathVar[j]);
                        if (!strcmp(tempStr, " ")) //this is what path variables are when they are blanked/removed
                            continue;
                        strcat(tempStr, "/");
                        strcat(tempStr, inputArgs[0]);

                        if (execvp(tempStr, inputArgs) != -1) {
                            runSuccess = true;
                            break;
                        }
                    }

                    if (runSuccess == false) {
                        printf("%s command not found\n", inputArgs[0]);
                    }

                }

                if (skipFunctionCall == false) {
                    //for each directory, check if there's a function by that name.
                    //if not, print the "command not found"
                    bool runSuccess = false;
                    for (int j = 0; j < pathCounter; j++) {
                        char tempStr[2056];
                        strcpy(tempStr, pathVar[j]);
                        if (!strcmp(tempStr, " ")) //this is what path variables are when they are blanked/removed
                            continue;
                        strcat(tempStr, "/");
                        strcat(tempStr, inputArgs[0]);

                        if (execvp(tempStr, inputArgs) != -1) {
                            runSuccess = true;
                            break;
                        }
                    }

                    if (runSuccess == false) {
                        //  display if command is not found
                        printf("%s command not found\n", inputArgs[0]);
                    }

                }

                //exits early, to prevent the child process from continuing and running a lot of extra loop iterations.
                //this only runs if the exec function fails to run.
                _exit(0);
            } else if (pid == -1) //there was an error calling fork()
            {
                printf("Fork process failed\n");
            } else //The parent process
            {
                int status;
                if (signal(SIGTSTP, sig_handler) == SIG_ERR);
                if (signal(SIGINT, sig_handler) == SIG_ERR);
                if (signal(SIGTERM, sig_handler) == SIG_ERR);
                if (signal(SIGQUIT, sig_handler) == SIG_ERR);
                waitpid(pid, &status, WCONTINUED | WUNTRACED);
                tcsetpgrp(0, pid);
                waitpid(pid, &status, 0);

                //frees memory allocated to the input args
                for (int i = 0; i < count; i++)
                    free(inputArgs[i]);
                free(inputArgs);

                //restore stdout
                dup2(stdout_save, STDOUT_FILENO); /* restore */
            }
        }

        if (exitStatus == true) {
            exit(0);
        }
    }
}

//This function splits the input string into a char** which is laid out for use in an exec function call
    //The count input argument is intended to be "passed by reference", so you conveniently know how long the function/argument array is
    //This function does return allocated memory, so free must be used after calling this.
char** Split(char line[], int* count)
{

    //This block makes a copy of the original string, so strtok doesn't destroy the original one.
    char tempStr[MAXBYTES];
    strcpy(tempStr, line);

    //This block is used to find exactly how many command/argument(s) there are.
    char* token;
    int argumentCount = 0;
    token = strtok(tempStr, " ");
    argumentCount++;
    while(token != NULL)
    {
        token = strtok(NULL, " ");
        //if(token != '\n')
        argumentCount++;
    }
    argumentCount--; //decrements once, because it increments an extra time at the end of that while loop

    //now we have the number of things to break apart, so make a char** of that size
    char** output = (char**)(malloc(sizeof(char*) * (argumentCount)));
    output[argumentCount-1] = NULL; //sets the last element to be a NULL, for the exec format
    for(int i = 0; i < argumentCount; i++)
    {
        output[i] = (char*)(malloc(MAXBYTES));
    }

    //tokenize the original line, and read each token in to the output char**
    token = strtok(line, " ");
    strcpy(output[0], token);
    int index = 1;
    while(true)
    {
        token = strtok(NULL, " ");
        if(token == NULL)
            break;
        strcpy(output[index], token);
        index++;
    }

    //returns the total number of arguments, via pass by reference
    *count = argumentCount;

    return output;
}


void sig_handler(int signo)
{
    kill(pid, signo);

    if(signo == SIGTSTP)
        kill(pid, SIGINT);
}

/*
void FreeMainMemory()
{
    for(int i = 0; i < 20; i++)
    {
        free(history[i]);
    }
    free(history);

    for(int i = 0; i < 100; i++)
        free(pathVar[i]);
    free(pathVar);
}
*/
