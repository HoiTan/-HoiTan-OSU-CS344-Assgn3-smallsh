#define  _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include "helper.h"

#define MAX_LENGTH 2048
#define MAX_ARG 512
#define MAX_BACKGROUND_PROCESSES 100

// GLOBAL VARIABLE
int backgroundStatus;  //0 for foreground only ignore &, 1 for allowing background too
int backgroundProcesses[MAX_BACKGROUND_PROCESSES];
int backgroundProcessesNum = 0;

struct command
{
    char *commandLine[MAX_ARG];
    char *inputFile;
    char *outputFile;
    int background;
};

struct command promptUser()
{
    char *input = malloc(MAX_LENGTH * sizeof(char));
    size_t len = MAX_LENGTH;
    struct command cmd;

    // Clear the command struct
    for (int i = 0; i < MAX_ARG; i++)
        cmd.commandLine[i] = NULL;
    cmd.inputFile = NULL;
    cmd.outputFile = NULL;
    cmd.background = 0;

    printf(": ");
    fflush(stdout);
    getline(&input, &len, stdin);

    // Replace the new line character '\n' with null terminator
    int newLine = 0;
    for (int i = 0; i < len && !newLine; i++)
    {
        if (input[i] == '\n')
        {
            input[i] = '\0';
            newLine = 1;
        }
    }

    // Check if it is a blank line or comment line
    if (strcmp(input, "") == 0 || input[0] == '#')
    {
        cmd.commandLine[0] = "";
        return cmd;
    }

    // Expand and Replace $$ with pid number
    replaceDoubleDollar(input);

    // store the arguments into command
    char* saveptr;
    char* token = strtok_r(input, " ", &saveptr);
    for (int i = 0; token != NULL; i++) 
    {
        cmd.commandLine[i] = strdup(token);
        token = strtok_r(NULL, " ", &saveptr);
    }

    free(input);
    // Test: Print the stored tokens
    // printf("Stored tokens:\n");
    // for (int i = 0; cmd.commandLine[i] != NULL; i++) 
    // {
    //     printf("cmd[%d]: %s\n", i, cmd.commandLine[i]);
    // }
    return cmd;
}

// replace the $$ with pid
void replaceDoubleDollar(char *str)
{
    pid_t pid = getpid();
    int pidLen = snprintf(NULL, 0, "%d", pid); // Determine the length of the process ID string
    char Pid[pidLen + 1]; // Allocate Pid with the exact size required

    sprintf(Pid, "%d", pid); // Convert the process ID to a string

    int len = strlen(str);
    char newStr[len * pidLen + 1];

    int i = 0;
    int j = 0;

    while (i < len)
    {
        if (str[i] == '$' && str[i + 1] == '$')
        {
            strcpy(&newStr[j], Pid);
            j += pidLen;
            i += 2;
        }
        else
        {
            newStr[j++] = str[i++];
        }
    }

    newStr[j] = '\0';
    strcpy(str, newStr);
    // printf("this is what you entered: %s\n", str);
}

// read the input command, break it down and call the according process function
void breakDownCmd(struct command cmd, int exitStatus)
{   
    //set input, output file, and fore or background mode
    for (int i = 0; cmd.commandLine[i]; i++) 
        {
        // Set background status according to the command line and the backgroundStatus of the process
            if (!strcmp(cmd.commandLine[i], "&"))
            {
                if (backgroundStatus == 1)
                {
                    // printf("background is on\n");
                    // fflush(stdin);
                    cmd.background = 1;
                }
                cmd.commandLine[i] = NULL;
            }   

            // Store the output file
            else if (!strcmp(cmd.commandLine[i], ">")) 
            {
                if (cmd.commandLine[i + 1] != NULL) 
                {
                    cmd.outputFile = strdup(cmd.commandLine[i + 1]);
                    cmd.commandLine[i + 1] = NULL;
                    cmd.commandLine[i] = NULL;
                    if (cmd.outputFile != NULL) {
                        // printf("outputfile is: %s\n", cmd.outputFile);
                        fflush(stdin);
                    }
                } else {
                    printf("Missing argument for output redirection.\n");
                    fflush(stdin);
                }
            }

            // Store the input file
            else if (!strcmp(cmd.commandLine[i], "<")) 
            {
                if (cmd.commandLine[i + 1] != NULL) 
                {
                    cmd.inputFile = strdup(cmd.commandLine[i + 1]);
                    cmd.commandLine[i+1] = NULL;
                    cmd.commandLine[i] = NULL;
                    if(cmd.outputFile != NULL) {
                        // printf("inputfile is: %s", cmd.inputFile);
                        fflush(stdin);
                    }
                } else
                {
                    printf("Missing argument for intput redirection.\n");
                    fflush(stdin);
                }

            }
        }
    //execute the commands
    for (int i = 0; cmd.commandLine[i]; i++) 
    {
        // exit
        if (!strcmp(cmd.commandLine[0], "exit")) 
            exit(0);

        // cd 
        else if (!strcmp(cmd.commandLine[0], "cd")) 
        {
            cd(cmd);
            return;
        }

        // status 
        else if (!strcmp(cmd.commandLine[0], "status"))
        {
            status(exitStatus);
            return;
        }

        // other commands
        else
        {
            otherCmd(cmd, &exitStatus);
            return;
        }
    }    
}

void cd(struct command cmd)
{
    if (cmd.commandLine[1]) 
    {
        if (chdir(cmd.commandLine[1]) != 0)
            perror("chdir() to failed");
    } else // return to home dir
    {
        if (chdir(getenv("HOME")) != 0) 
            perror("chdir() to /HOME failed");
    }
}

void status(int exitStatus)
{
    if(WIFEXITED(exitStatus)) // returns true if the child was terminated normally
        printf("exit value %d\n", WEXITSTATUS(exitStatus));    
    else                      // child was terminated abnormally  
        printf("terminated by signal %d\n", WTERMSIG(exitStatus));
}

// process non built-in-command
void otherCmd(struct command cmd, int* exitStatus)
{
    pid_t childPid = fork();

    if(childPid == -1){
		perror("fork() failed!");
		exit(1);
	} 
    else if(childPid == 0)
    {
		// The child process execute this

        // Set up a signal handler for SIGINT for foreground
        struct sigaction SIGINT_action = {0};
        SIGINT_action.sa_handler = SIG_DFL; // Default behavior for SIGINT

        if (sigaction(SIGINT, &SIGINT_action, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }

		// printf("Child's pid = %d\n", getpid());
        // fflush(stdout);

        // redirect input file
        if(cmd.inputFile != NULL)
        {
            int inFile = open(cmd.inputFile, O_RDONLY);
            if(inFile == -1)
            {
                perror("Unable to open the file.\n");
                *exitStatus = 1;
            } else
            {
                if (dup2(inFile, 0) == -1) {
                    perror("Unable to redirect the file!\n");
                }
            }
        }

        // redirect output file
        if(cmd.outputFile != NULL)
        {
            int outFile = open(cmd.outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if(outFile == -1)
            {
                perror("Unable to open the file.\n");
                *exitStatus = 1;
            } else
            {
                if (dup2(outFile, 1) == -1) {
                    perror("Unable to redirect the file!\n");
                }
            }
        }
        
        // execute the commad
        if(execvp(cmd.commandLine[0], cmd.commandLine) < 0)
        {
            // printf("%s: no such file or directory\n", cmd.commandLine[0]);
			perror("execvp");   /* execvp() returns only on error */
            fflush(stdout);
            exit(1);
        }
        
	} else
    {
		// Parent process executes this

        if(cmd.background == 0) // foreground command
            waitpid(childPid, exitStatus, 0);
        else                    // background command
        {
            printf("Background Process ID is %d\n", childPid);   
            fflush(stdin);
            backgroundProcesses[backgroundProcessesNum++] = childPid;
        }
    }
}
// Check for completed background processes
void checkBackgroundProcesses() {
    int i;
    for (i = 0; i < backgroundProcessesNum; i++) 
    {
        pid_t pid = backgroundProcesses[i];
        int status;
        int result = waitpid(pid, &status, WNOHANG);
        if (result == -1) 
        {
            // Error occurred
            perror("waitpid");
        } else if (result > 0)
        {
            // Process completed
            if (WIFEXITED(status)) 
            {
                // Child exited normally
                printf("Background process with PID %d exited normally with status %d\n", pid, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) 
            {
                // Child exited due to a signal
                printf("Background process with PID %d exited abnormally due to signal %d\n", pid, WTERMSIG(status));
            }
            // Remove the completed process from the array
            int j;
            for (j = i; j < backgroundProcessesNum - 1; j++) 
            {
                backgroundProcesses[j] = backgroundProcesses[j + 1];
            }
            backgroundProcessesNum--;
            i--; // Adjust the loop index
        }
    }
}

void handle_SIGSTP(int signo){
    if (backgroundStatus == 1)
    {
        char* message = "Entering foreground-only mode (& is now ignored)\n";
        write(1, message, 49);
        fflush(stdout);
        backgroundStatus = 0;
    }else
    {
        char* message = "Exiting foreground-only mode\n";
        write (1, message, 29);
        fflush(stdout);
        backgroundStatus = 1;
    }
}

int main(void)
{
    int exitStatus = 0;
    backgroundStatus = 1;
    // parts from module's exploration 
    // Initialize SIGINT_action struct to be empty
	struct sigaction SIGINT_action = {0}, SIGSTP_action = {0};
    
    // ignore the control C
	SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);
    
    // reset the control Z action
    SIGSTP_action.sa_handler = handle_SIGSTP;
    sigfillset(&SIGSTP_action.sa_mask);
    SIGSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGSTP_action, NULL);
    
    // executing small shell
    while(1)
    {
        struct command cmd = promptUser();
        breakDownCmd(cmd, exitStatus);
        checkBackgroundProcesses(); 
        
    };

    return 0;
}
