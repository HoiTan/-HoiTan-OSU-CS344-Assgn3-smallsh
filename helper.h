struct command promptUser();
void replaceDoubleDollar(char *str);
void analyzeCmd(struct command cmd);
void breakDownCmd(struct command cmd, int status);
void cd(struct command cmd);
void status(int status);
void otherCmd(struct command cmd, int* exitStatus);
void checkBackgroundProcesses();
void handle_SIGINT(int signo);