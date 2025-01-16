//Custom Linux Shell
//Developed by Owen Treanor

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <sys/wait.h>
#include <signal.h>

#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

#define BYTES 4096  //Macro for number of bytes of memory created

char currentDirectory[BYTES];  //String for current directory

pid_t SHELL_PID;    //Store pid

typedef struct{     //Struct used for a process in the lp command
    int pid;
    char* user;
    char* command;

} Process;

volatile __sig_atomic_t interrupted = 0;    //Variable to flag if SIGINT happened

void handle(int sig){   //Handler that kills any child processes when SIGINT is signaled
    interrupted = 1;
    if(getpid() != SHELL_PID){  //Avoid killing the shell process
        if(kill(getpid(), SIGKILL) == 1){
            fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));
        }
    }
    printf("\n");
}


void do_cd(char** argv, int size){  //Function to do the cd command
    if(size == 1 || (size == 2 && argv[1][0] == '~')){  //If the user just enters cd or ~ then this handles that
        uid_t uid = getuid();
        struct passwd* p;
        if((p = getpwuid(uid)) != NULL){       //Get the uid to get the user's home directory
            char path[BYTES];
            char* pw = p->pw_dir;
            for(int i = 0; i < strlen(pw); i++){
                path[i] = pw[i];    //Put the name of the directory into the path string
            }
            int len = strlen(pw);
            if(size == 2 && strlen(argv[1]) != 1){  //Case for when the user enters ~ with a path following
                int index = 1;
                while(argv[1][index] != 0){ //Copy the rest of the path into the path variable
                    path[len] = argv[1][index];
                    len++;
                    index++;
                }
                path[len] = 0;  //Add null terminator
            }
            if(chdir(path) != 0){   //Change the working directory to the new path
                fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", path, strerror(errno));
            }
            for(int i = 0; i < BYTES; i++){
                path[i] = 0;    //Reset the buffer
            }
        }
        else{
            fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
        }
        if((getcwd(currentDirectory, BYTES)) == NULL){  //Modify the currentDirectory variable
            fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
        }
    }
    else if(size == 2){ //Case for when the user enters a path
        char* path = argv[1];
        if(chdir(path) == 0){   //Change the directory to the path
            if((getcwd(currentDirectory, BYTES)) == NULL){  //Modify the currentDirectory variable
                fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
            }
        }
        else{
            fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", path, strerror(errno));
        }
    }
    else{
        fprintf(stderr, "Error: Too many arguments to cd.\n");
    }
}

void do_lf(){   //Function to do the lf command
    DIR* dp;
    struct dirent* dirp;
    dp = opendir(currentDirectory);
    while((dirp = readdir(dp)) != NULL){    //Iterate through each file in the current directory
        char* nextFile = dirp->d_name;
        if((nextFile[0] == '.' && nextFile[1] == 0) || (nextFile[0] == '.' && nextFile[1] == '.' && nextFile[2] == 0)){
            continue;   //Skip . and ..
        }
        else{
            printf("%s\n", nextFile);   //Print out each file
        }   
    }
    closedir(dp);
}

int compare(const void* a, const void* b){  //Function to compare pids that is used for printing processes in order
    Process* l = (Process*)a;
    Process* r = (Process*)b;
    if(l->pid < r->pid) return -1;
    else if(l->pid > r->pid) return 1;
    return 0;   //Should never happen 
}

void printProcesses(Process* processes, int size){  //Print each of the processes for the lp command
    qsort(processes, size, sizeof(Process), compare);   //Sort the processes
    for(int i = 0; i < size; i++){
        printf("%5d %s %s\n", processes[i].pid, processes[i].user, processes[i].command);
        free(processes[i].user);
        free(processes[i].command);
    }
}


void do_lp(){   //Function to do the lp command
    DIR* dp;
    struct dirent* dirp;
    char* tempDirectory = currentDirectory;     //Make a new variable so we remember the current directory
    if(chdir("/proc") != 0){    //Change the directory to /proc to be able to access each process
        fprintf(stderr, "Error: Cannot change directory to /proc. %s.\n", strerror(errno));
    }
    if((tempDirectory = getcwd(NULL, 0)) == NULL){  //Modify the tempDirectory variable
        fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
    }
    dp = opendir(tempDirectory);    
    Process* processes;
    if((processes = (Process*)malloc(BYTES*sizeof(Process))) == NULL){      //Create space on the heap to store each process in an array
        fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
    }
    Process* current;
    struct passwd* p;
    int index = 0;
    while((dirp = readdir(dp)) != NULL){    //Iterate through each file in the /proc folder
        char* nextFile = dirp->d_name;
        if(nextFile[0] >= 49 && nextFile[0] <= 57){     //Only consider files that start with 1-9 to filter out the nonprocesses
            current = &processes[index];    //Set current process
            current->pid = atoi(nextFile);    //Store the pid into the current process part of the array
            uid_t uid = getuid();
            if((p = getpwuid(uid)) == NULL){        //Set up the passwd object
                fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
            }
            current->user = strdup(p->pw_name); //Store the process name into the current process using strdup to copy it

            if(chdir(nextFile) != 0){   //Go into the process directory
                fprintf(stderr, "Error: Cannot change directory to /proc. %s.\n", strerror(errno));
            }
            FILE* fp = fopen("cmdline", "r");   //Open the cmdline file so we can get the command   
            if(fp == NULL){
                fprintf(stderr, "Error in opening file!\n");
            }
            char* cmd;
            if((cmd = (char*)malloc(BYTES)) == NULL){   //Make space on the heap for the command
                fprintf(stderr, "Error: malloc() failed. %s.\n", strerror(errno));
            }
            fgets(cmd, BYTES, fp);  //Store the file's contents into the cmd variable
            current->command = cmd;     //Store the command into the current process
            fclose(fp);     //Close the file
            if(chdir("..") != 0){   //Go back to the /proc folder
                fprintf(stderr, "Error: Cannot change directory to ... %s.\n", strerror(errno));
            }
            index++;
        }
    }
    if(chdir(currentDirectory) != 0){   //Change back to the current directory
        fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", currentDirectory, strerror(errno));
    }
    printProcesses(processes, index);   //Print the processes
    free(processes);    //Free processes
    free(tempDirectory);    //Free tempDirectory
    closedir(dp);   //Close the DIR


}

int main(){

    SHELL_PID = getpid();   //Get the PID of this process


    struct sigaction action = {0};  //Set up signal handler
    action.sa_handler = handle;
    sigaction(SIGINT, &action, NULL);


    if((getcwd(currentDirectory, BYTES)) == NULL){  //Get the current working directory
        fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
    }

    while(1){

        printf("%s[%s]> ", BLUE, currentDirectory); //Print the directory in blue
        printf("%s", DEFAULT);      //Set the color back to the default one
        
        char input[BYTES];

        if(fgets(input, sizeof(input), stdin) == NULL){     //Get the user input
            if(interrupted == 1){   //Skip everything if there was an interrupt
                interrupted = 0;
                continue;
            }
            else{
                fprintf(stderr, "Error: Failed to read from stdin. %s.\n", strerror(errno));
            }
        }

        char* arg = strtok(input, " \n");   //Split up the user input and remove spaces and new lines
        char* argv[BYTES];
        int argIndex = 0;

        while(arg != NULL){ //Set up each string in argv
            argv[argIndex] = arg;
            arg = strtok(NULL, " \n");
            argIndex++;
        }

        if(argv[0] == NULL){    //Ignore when the user just hits enter
            continue;
        }
        
        if(strcmp(argv[0], "cd") == 0){     //If the user enters cd then do that function
            do_cd(argv, argIndex);
        }
        else if(strcmp(argv[0], "exit") == 0){  //If the user exits then exit with success code
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(argv[0], "pwd") == 0){      //If the user enters pwd then just print it
            printf("%s\n", currentDirectory);
        }
        else if(strcmp(argv[0], "lf") == 0){    //If the user enters lf then do that function
            do_lf();
        }
        else if(strcmp(argv[0], "lp") == 0){    //If the user enters lp then do that function
            do_lp();
        }
        else{   //Otherwise we need to do exec
            pid_t pid;
            int stat;
            if((pid = fork()) == 0){    //Fork a new process
                execvp(argv[0], argv);      //Use execvp with the arguments to switch to the new code
                fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
                exit(EXIT_FAILURE); //If the exec failed then we end this process
            }
            else if(pid < 0){
                fprintf(stderr, "Error: fork() failed. %s.\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            else{
                if((wait(&stat)) == -1 && interrupted == 0){    //Wait for the child to end
                    interrupted = 0;    //If wait failed and we did not interrupt then print an error
                    fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
                }
            }

        }
        for(int i = 0; i < argIndex; i++){      //Clear out each argument
            argv[i] = NULL;
        }


    }
}

