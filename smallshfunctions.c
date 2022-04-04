#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

/*
Global Variables
*/
// Keep track of child's last exit status.
int lastExitStatus = 0;
// Toggle flag for foreground-only mode.
int backgroundProcessAllowed = 1;
// Array to keep track of child processes.
int childProcesses[128];
// Counter for child processes array.
int indexCounter = 0;

/*
Structure for parsing and saving data associated with command input.
*/
struct commandStructure {
    char* command;
    char* bashCommand;
    char* arguments[512];
    int argumentCounter;
    char* inputFileName;
    char* outputFileName;
    int outputRedirect;
    int inputRedirect;
    int backgroundProcess;
};



/*
Initializes childProcesses global array to point to all point to NULL.
*/
void initializeChildArray() {
    int z = 0;
    for (z ; z < 129; z++) {
        childProcesses[z] = NULL;
    }
}



/*
Iterates through childProcesses global array.
*/
void checkChildProcesses() {
    int childExit;
    int y = 0;
    int processID;
    
    for (y; y < 129; y++) {
        // If a pid value is found in array, calls waitpid.
        if (childProcesses[y] != NULL) {
            processID = waitpid(childProcesses[y], &childExit, WNOHANG);

            //Checks process ID is returned, check if it has exited and print appropriate message.
            if (processID > 0) {
                if (WIFEXITED(childExit) != 0) {
                    printf("Background child process %d finished. Status value: %d.\n"\
                            , processID, childExit);
                    fflush(stdout);
                    childProcesses[y] = NULL;
                }

                else if (WIFSIGNALED(childExit)) {
                    printf("Background child process %d terminated due to signal %d.\n"\
                            , processID, childExit);
                    fflush(stdout);
                    childProcesses[y] = NULL;                  
                }
            }
        }
    }
}



/*
Sets the SIGINT and SIGTSTP signal handlers for children.
SIGINT to SIG_DFL
SIGSTP to SIG_IGN
*/
void setSignalsForegroundChild() {
    struct sigaction SIGINT_foregroundChildAction = {0};
    struct sigaction SIGTSTP_foregroundChildAction = {0};

    SIGINT_foregroundChildAction.sa_handler = SIG_DFL;
    sigfillset(&SIGINT_foregroundChildAction.sa_mask);
    SIGINT_foregroundChildAction.sa_flags = 0;

    sigaction(SIGINT, &SIGINT_foregroundChildAction, NULL);


    SIGTSTP_foregroundChildAction.sa_handler = SIG_IGN;
    sigfillset(&SIGTSTP_foregroundChildAction.sa_mask);
    SIGTSTP_foregroundChildAction.sa_flags = 0;

    sigaction(SIGTSTP, &SIGTSTP_foregroundChildAction, NULL);

}



/*
Processes a command that is to be run by bash.
Input: commandStructure
*/
void otherCommand(struct commandStructure* command) {
    int outputFile;
    int inputFile;
    int outputFileBack;
    int inputFileBack;
    // Background Process condition.
    // backgroundProcess in commandStructure array is set to 1 for the command.
    // backgroundProcessAllowed global variable is set to 1.
    if ((backgroundProcessAllowed == 1) && command->backgroundProcess == 1) {
        int inputDirect;
        int outputDirect;

        // Start background child process.
        pid_t childProcessBack = -100;
        int childExitBack = -100;
        int backgroundProgram = 0;
        childProcessBack = fork();

        switch(childProcessBack) {
            // Errors
            case -1:
                printf("Error spawning child. Please try again.");
                fflush(stdout);
                return;

            // Success
            case 0:

                //Check Input and Output Redirection and close their files if Redirection applies.
                if ((command->inputRedirect == 1) || (command->outputRedirect == 1)) {
                    if (command->outputRedirect == 1) {
                        outputFileBack = open(command->outputFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (outputFileBack == -1) {
                            printf("Invalid output file '%s'. Please try again.\n", command->outputFileName);
                            fflush(stdout);
                            exit(1);
                        }
                        int outputRedirection = dup2(outputFileBack, 1);
                        if (outputRedirection == -1) {
                            printf("Invalid redirection output. Please try again.\n");
                            fflush(stdout);
                            exit(1);
                        }

                        // close(outputFileBack);
                    }

                    if (command->inputRedirect == 1) {
                        inputFileBack = open(command->inputFileName, O_RDONLY);
                        if (inputFileBack == -1) {
                            printf("Invalid input file '%s'. Please try again.\n", command->inputFileName);
                            fflush(stdout);
                            exit(1);
                        }
                        int inputRedirection = dup2(inputFileBack, 0);
                        if (inputRedirection == -1) {
                            printf("Invalid redirection input. Please try again.\n");
                            fflush(stdout);
                            exit(1);
                        }

                        // close(inputFileBack);
                    }
                }

                // Input and Output to /dev/null if needed.
                if (command->inputRedirect == 0) {
                    inputDirect = open("/dev/null/", O_RDONLY);
                    dup2(inputDirect, 0);
                }

                if (command->outputRedirect == 0) {
                    outputDirect = open("dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    dup2(outputDirect, 1);
                }

                // Run program in background child process.
                backgroundProgram = execvp(command->command, command->arguments);
                
                // Error message and exit if background program is invalid.
                if (backgroundProgram == -1) {
                    printf("Invalid background program. Please try again.\n");
                    fflush(stdout);
                    exit(1);
                }

                // Successful end of child background process.
                close(outputFileBack);
                close(inputFileBack);
                exit(0);
        }

        // After the child process is started, whether it succeeds or fails, print out its PID.
        // Add the child process PID to the global childProcesses array.
        printf("Background PID: %d\n", childProcessBack);
        fflush(stdout);
        childProcesses[indexCounter] = childProcessBack;
        indexCounter++;

        // Reset indexCounter to go back to 0, looping back to the beginning of the array once 128 is reached.
        if (indexCounter > 128) {
            indexCounter = 0;
        }
    }
    
    // Foreground Process condition.
    // backgroundProcess in commandStructure array is set to 0 for the command
    else {
        //Start foreground child process.
        pid_t childProcess = -100;
        int childExit = -100;
        int foregroundProgram = 0;
        childProcess = fork();

        //Wait for child process to finish.
        waitpid(childProcess, &childExit, 0);

        //Set signal handling for child process foreground.
        setSignalsForegroundChild();

        switch(childProcess) {
            // Errors
            case -1:
                printf("Error spawning child. Please try again.");
                fflush(stdout);
                return;

            // Success
            case 0:

                //Input and Output Redirection
                if ((command->inputRedirect == 1) || (command->outputRedirect == 1)) {
                    if (command->outputRedirect == 1) {
                        outputFile = open(command->outputFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (outputFile == -1) {
                            printf("Invalid output file '%s'. Please try again.\n", command->outputFileName);
                            fflush(stdout);
                            exit(1);
                        }
                        int outputRedirection = dup2(outputFile, 1);
                        if (outputRedirection == -1) {
                            printf("Invalid redirection output. Please try again.\n");
                            fflush(stdout);
                            exit(1);
                        }

                        // close(outputFile);
                    }

                    if (command->inputRedirect == 1) {
                        inputFile = open(command->inputFileName, O_RDONLY);
                        if (inputFile == -1) {
                            printf("Invalid input file '%s'. Please try again.\n", command->inputFileName);
                            fflush(stdout);
                            exit(1);
                        }
                        int inputRedirection = dup2(inputFile, 0);
                        if (inputRedirection == -1) {
                            printf("Invalid redirection input. Please try again.\n");
                            fflush(stdout);
                            exit(1);
                        }

                        // close(inputFile);
                    }
                }

                //Run program in foreground child process.
                foregroundProgram = execvp(command->command, command->arguments);
                
                // Error message and exit if background program is invalid.
                if (foregroundProgram == -1) {
                    printf("Invalid command. Please try again.\n");
                    fflush(stdout);
                    exit(1);
                }

                // Successful end of child foreground process.
                close(outputFile);
                close(inputFile);
                exit(0);
        }

        // If childExit was unchanged this loop, return.
        if (childExit == -100) {
            return;
        }

        // Otherwise, update lastExitStatus.
        if (WIFEXITED(childExit) != 0) {
            lastExitStatus = WEXITSTATUS(childExit);
        }

        // Update lastExitStatus with termination signal if one was received.
        else if (WIFSIGNALED(childExit) != 0) {
            lastExitStatus = WTERMSIG(childExit);
            printf(" Terminated, signal %d\n", lastExitStatus);
            fflush(stdout);
        }
        
        else {
            exit(1);
        }
    }

}



/*
Check if command is to be run by smallshell, and run it if so.
Otherwise, pass of the command to otherCommand().
input: commandStructure
*/
int shellCommand(struct commandStructure* command) {
    char directory[2048];
    int directoryErr;

    // Print out the commandStructure data. 
    // **FOR DEBUGGING PURPOSES ONLY. Left in for reuse if this program is to be revisisted in the future.**
    // printf("This Command is: %s\n", command->command);
    // printf("Bash Command is: %s\n", command->bashCommand);
    // printf("Argument 0 is: %s\n", command->arguments[0]);
    // printf("Argument 1 is: %s\n", command->arguments[1]);
    // printf("Argument 2 is: %s\n", command->arguments[2]);
    // printf("Argument counter is: %d\n", command->argumentCounter);
    // printf("Input File Name is: %s\n", command->inputFileName);
    // printf("Output File Name is: %s\n", command->outputFileName);
    // printf("OutputRedirect is: %d\n", command->outputRedirect);
    // printf("inputRedirect is: %d\n", command->inputRedirect);
    // printf("Background is: %d\n", command->backgroundProcess);

    //exit command.
    if (strcmp(command->command, "exit") == 0) {
        //Kill off all children found in the childProcesses global array.
        int i = 0;
        for (i; i < 129; i++) {
            if (childProcesses[i] != NULL) {
                kill(childProcesses[i], 15);
                childProcesses[i] = NULL;
            }
        }
        exit(0);
    }

    //status command.
    if (strcmp(command->command, "status") == 0) {
        printf("Status exit value: %d\n", lastExitStatus);
        return(0);
    }

    //cd command.
    if (strcmp(command->command, "cd") == 0) {
        if (command->argumentCounter == 1) {
            // Get home directory and change to it.
            chdir(getenv("HOME"));
            getcwd(directory, sizeof(directory));
        }

        if (command->argumentCounter == 2) {
            // If argument counter is 2, change to the directory in command->arguments[1].
            directoryErr = chdir(command->arguments[1]);
            if (directoryErr != 0) {
                //Invalid directory, print error.
                printf("Invalid directory change. Please try again.\n");
                fflush(stdout);
                return(0);
            }
            getcwd(directory, sizeof(directory));
        }

        // If more than 2 total arguments are given with "cd" command, invalid command.
        if (command->argumentCounter > 2) {
            printf("Invalid directory change. Please try again.\n");
            fflush(stdout);
            return(0);
        }

    }

    else {
        otherCommand(command);
    }

    return(0);
}
