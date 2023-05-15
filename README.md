# OSU-CS344-Assgn3-smallsh
# Introduction
In this assignment you will write smallsh your own shell in C. smallsh will implement a subset of features of well-known shells, such as bash. Your program will:

1. Provide a prompt for running commands
2. Handle blank lines and comments, which are lines beginning with the # character
3. Provide expansion for the variable $$
4. Execute 3 commands exit, cd, and status via code built into the shell
5. Execute other commands by creating new processes using a function from the exec family of functions
6. Support input and output redirection
7. Support running commands in foreground and background processes
8. Implement custom handlers for 2 signals, SIGINT and SIGTSTP

## Learning Outcomes
After successful completion of this assignment, you should be able to do the following:

Describe the Unix process API (Module 4, MLO 2)
Write programs using the Unix process API (Module 4, MLO 3)
Explain the concept of signals and their uses (Module 5, MLO 2)
Write programs using the Unix API for signal handling (Module 5, MLO 3)
Explain I/O redirection and write programs that can employ I/O redirection (Module 5, MLO 4)
