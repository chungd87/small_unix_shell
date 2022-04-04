#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include "smallshfunctions.c"
/* Danny Chung | CS344_400_W2022 | chungdan@oregonstate.edu */



/*
Fills in structure for parsing and saving data associated with command input.
Returns a structure that can be used by the shell for processing.
If the command is a comment or NULL, 'main' program will discard it and start over.
Takes input of a string.
Returns commandStructure.
*/
struct commandStructure* parseInputCommand(char* inputCommand) {
    struct commandStructure* commandData = malloc(sizeof(struct commandStructure));

    //Save a full command line for bash command in case it is needed, and remove '&' from end if present.
    commandData->bashCommand = calloc(strlen(inputCommand) + 1, sizeof(char));
    strcpy(commandData->bashCommand, inputCommand);
    if (strncmp(" &\n", &commandData->bashCommand[strlen(inputCommand) - 3], strlen(" &\n")) == 0) {
        commandData->bashCommand[strlen(commandData->bashCommand)-2] = '\0';
    }

    //Take first token of input, and initialize some values of commandStructure commandData.
    char* savePointer;
    char* token = strtok_r(inputCommand, " \n", &savePointer);
    char poundSign = '#';
    char inputRedir = '<';
    char outputRedir = '>';
    char ampersand = '&';
    commandData->argumentCounter = 0;
    commandData->outputRedirect = 0;
    commandData->inputRedirect = 0;
    commandData->backgroundProcess = 0;

    //Return if first token is NULL. 'main' will reprompt for another command.
    if (token == NULL) {
        return commandData;
    }

    //Return if first token is #, or space.
    if ((strchr(token, poundSign) != NULL) || ((strcmp(token, "\n") == 0)) || ((strcmp(token, " \n") == 0))) {
        return commandData;
    }
    
    //Put first real token into commandData->command, and commandData->arguments[]. Increment argumentCounter.
    commandData->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(commandData->command, token);
    commandData->arguments[commandData->argumentCounter] = token;
    commandData->argumentCounter++;

    //Take remaining tokens until a NULL token is reached.
    token = strtok_r(NULL, " \n", &savePointer);

    while (token != NULL) {
        // Case for when a '>' token is encountered.
        // Turn commandData->outputRedirect flag to true (1).
        if (strchr(token, outputRedir) != NULL) {
            commandData->outputRedirect = commandData->outputRedirect + 1;

            //Check if there has already been a '>' symbol before, and another is found, error. 'main' will reprompt.
            if (commandData->outputRedirect > 1) {
                printf("Input/Output Redirection Error. Please limit to one output and one input redirection per command.");
                fflush(stdout);
                return commandData;
            }

            // After '>' is encountered, the next argument should be the output File Name.
            token = strtok_r(NULL, " \n", &savePointer);

            // If it's not, error. 'main' will reprompt.
            if ((token == NULL) || (strchr(token, inputRedir) != NULL) || (strchr(token, outputRedir) != NULL) \
                    || ((strcmp(token, "\n") == 0)) || ((strcmp(token, " \n") == 0)) ) {
                printf("Invalid input/output redirection. Please try again.");
                fflush(stdout);
                return commandData;
            }

            // Put output File Name into commandData->outputFileName
            commandData->outputFileName = calloc(strlen(token) + 1, sizeof(char));
            strcpy(commandData->outputFileName, token);

        }

        // Case for when a '<' token is encountered.
        // Turn commandData->inputRedirect flag to true (1).
        else if (strchr(token, inputRedir) != NULL) {
            commandData->inputRedirect = commandData->inputRedirect + 1;

            //Check if there has already been a '<' symbol before, and another is found, error. 'main' will reprompt.
            if (commandData->inputRedirect > 1) {
                printf("Input/Output Redirection Error. Please limit to one output and one input redirection per command.");
                fflush(stdout);
                return commandData;
            }

            // After '<' is encountered, the next argument should be the input File Name.
            token = strtok_r(NULL, " \n", &savePointer);

            // If it's not, error. 'main' will reprompt.
            if ((token == NULL) || (strchr(token, inputRedir) != NULL) || (strchr(token, outputRedir) != NULL) \
                    || ((strcmp(token, "\n") == 0)) || ((strcmp(token, " \n") == 0)) ) {
                printf("Invalid input/output redirection. Please try again.");
                fflush(stdout);
                return commandData;
            }

            // Put input File Name into commandData->inputFileName
            commandData->inputFileName = calloc(strlen(token) + 1, sizeof(char));
            strcpy(commandData->inputFileName, token);
        }

        //If token is not one of the previous symbols, put the token into the arguments array, and increment it.
        else {
            commandData->arguments[commandData->argumentCounter] = token;
            commandData->argumentCounter++;
        }

        //Next token. If it is null, exits while loop.
        token = strtok_r(NULL, " \n", &savePointer);
    }

    // If final argument token is '&' character, remove it by making it null.
    // Then, change commandData->backGroundProcess flag to 1.
    if ((strcmp(commandData->arguments[commandData->argumentCounter-1], "&") == 0)) {
        commandData->backgroundProcess = 1;
        commandData->arguments[commandData->argumentCounter-1] = NULL;
    }

    //If final argument token is not '&' character, make sure the next argument in the array is null for termination.
    else if ((strcmp(commandData->arguments[commandData->argumentCounter-1], "&") != 0)) {
        commandData->arguments[commandData->argumentCounter] = NULL;
    }

    //Return the structure.
    return commandData;
}



/*
Expands '$$' string into the process ID number.
Takes input of a string.
*/
void stringExpansion(char* inputCommand) {
    int pid;
    pid = getpid();
    char moneySign = '$';
    char beforeMoneySigns[1024];
    char afterMoneySigns[1024];
    int i = 0;

    // If a '$' is encountered, then check if the next character is also '$'.
    // If so, split the string where the '$$' was encountered, insert pid, and then
    // copy back into the inputCommand variable.
    for (i; i < strlen(inputCommand); i++) {
        if ((inputCommand[i] == moneySign) && (inputCommand[i+1] == moneySign)) {
            snprintf(beforeMoneySigns, i+1, "%s", inputCommand);
            strcpy(afterMoneySigns, &inputCommand[i+2]);
            sprintf(inputCommand, "%s%d%s", beforeMoneySigns, pid, afterMoneySigns);
            i=0;
        }
    }
}



/*
Signal function for handling SIGSTP in the case of the parent.
Toggles global variable backgroundProcessAllowed.
Displays a message indicating the toggle status.
*/
void handle_SIGTSTP(int signo) {
    if (backgroundProcessAllowed == 1) {
        backgroundProcessAllowed = 0;
        char* message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 50);
        return;
    }

    else if (backgroundProcessAllowed == 0) {
        backgroundProcessAllowed = 1;
        char* message = "\nExited foreground-only mode.\n";
        write(STDOUT_FILENO, message, 30);
        return;
    }
}



/*
Sets the SIGINT and SIGTSTP signal handlers.
SIGINT to SIG_IGN
SIGTSTP to handle_SIGTSTP function.
*/
void setSignals() {
    struct sigaction SIGINT_action = {0};
    struct sigaction SIGTSTP_action = {0};

    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}



int main(void) {
    char inputCommand[2048];

    // Initialize the childProcesses global array.
    initializeChildArray();
    while (1) {
        // Set signal handlers.
        setSignals();

         // Check for zombie or ended child processes and clear them.
        checkChildProcesses();

        // Prompt for inputCommand.
        memset(inputCommand, 0, sizeof(inputCommand));
        fflush(stdout);

        // Short sleep to make sure background terminating messages can be printed before ': ' is reprompted.
        usleep(400);

        printf(": ");
        fflush(stdout);
        fgets(inputCommand, 2048, stdin);

        // Expand any '$$' string into PID.
        stringExpansion(inputCommand);

        // Create a commandStructure structure from the input command.
        struct commandStructure* command = parseInputCommand(inputCommand);

        // When the command is returned, check if it is valid.
        // If invalid, free it, and return to beginning of loop to get another input command.
        if ((command->argumentCounter < 1) || (command->inputRedirect > 1) || (command->outputRedirect > 1)) {
            free(command);
            continue;
        }

        // Process the command structure in the shell.
        shellCommand(command);

        // Free the command structure when done working with it.
        free(command);
    }

}
