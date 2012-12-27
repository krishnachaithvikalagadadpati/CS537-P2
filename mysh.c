#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



#define BUF_SIZE 514
#define ARG_SIZE 32

char errorMessage[30] = "An error has occurred\n";
char promptMessage[6] = "mysh> ";
int isBatch = 0;

void
error() 
{
    write(STDERR_FILENO, errorMessage, strlen(errorMessage));
    exit(1);
}

void execCmd(char *command[]) {
    pid_t childPID;
    int childStatus;
    childPID = fork();
    if(childPID == 0) {
        execvp(command[0], command);
        error();
        exit(0);
    }
    else {
        waitpid (childPID, &childStatus, 0);
    }
    return;
}

int
main(int argc, char *argv[])
{
    // arguments
    FILE *file = NULL;
    char buffer[BUF_SIZE];

    // input params
    if(argc == 1) {
        file = stdin;
        write(STDOUT_FILENO, promptMessage, strlen(promptMessage));
    }
    else if(argc == 2) {
        // open and read batch file
        char *batchFile = strdup(argv[1]);
        file = fopen(batchFile, "r");
        if (file == NULL) {
        	error();
        }
		isBatch = 1;
    }
    else {
        error();
    }
    
    while(fgets(buffer, BUF_SIZE, file)) {
        //fprintf(stdout, "%lu\n", (unsigned long)strlen(buffer));  
		if(isBatch) {
			write(STDOUT_FILENO, buffer, strlen(buffer));
		}
        if(buffer[strlen(buffer)-1] != '\n') {
            write(STDERR_FILENO, errorMessage, strlen(errorMessage));
            continue;
        }
        int fd = 0;
	    //if input contains a '>', output to file
        if(strchr(buffer, '>') != NULL) {
		    //assign *fileOut to the correct inputted file name
            char *fileOut = strtok(strstr(strdup(buffer), ">"), " \t\n>");
            if(strtok(NULL, " \t\n>") != NULL) {
                write(STDERR_FILENO, errorMessage, strlen(errorMessage));
                continue;
            }             
            fclose(stdout);
		    //attempt to open the output file
            fd = open(fileOut, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
            if (fd < 0) {
                write(STDERR_FILENO, errorMessage, strlen(errorMessage));
                continue;
            }
            char* overwrite = strstr(buffer, ">");
            overwrite[0] = '\0';
        }
	    //if batch output the input?
	
        if(buffer[0] != '\0' && buffer[0] != '\n') {
            char *token = strtok(buffer, " \t\n");
        	//First built in command    
		    if(strcmp(token,"cd") == 0) {
                token=strtok(NULL," \t\n");
                if(token==NULL) {
                    chdir(getenv("HOME"));
                }
                else {
                    int chSuccess = chdir(token);
				    if(chSuccess != 0) {
					    write(STDERR_FILENO, errorMessage, strlen(errorMessage));
				    }
                }
		    //second built in command
            } else if(strcmp(token,"pwd") == 0) {
			    if(strtok(NULL," \n\t") != NULL){
				    write(STDERR_FILENO, errorMessage, strlen(errorMessage));
			    }
			    else{
                char pwd[BUF_SIZE];
                char *directory = getcwd(pwd, BUF_SIZE);
                write(STDOUT_FILENO, directory, strlen(directory));
                write(STDOUT_FILENO, "\n", 1);
			    }
		    //third built in command
            } else if(strcmp(token,"exit") == 0) {
			    if(strtok(NULL," \n\t") != NULL){
				    write(STDERR_FILENO, errorMessage, strlen(errorMessage));
			    }
			    else
                	exit(0);
            } else if(strstr(token, ".c") != NULL) {
                char *command[ARG_SIZE];
                char *executable = strdup(token);
                strncpy(strstr(executable, ".c"), "\0", 2);
                command[0] = "gcc";
                command[1] = token;
                command[2] = "-Wall";
                command[3] = "-o";
                command[4] = executable;
                command[5] = NULL;
                execCmd(command);
                command[0] = executable;
                int i = 1;        
                while(token!=NULL){
                    token=strtok(NULL," \t\n");
                    command[i] = token;
                    i++;
                }
                execCmd(command);
            } else {
                char *command[ARG_SIZE];
                command[0] = token;
        
                int i = 1;        
                while(token!=NULL){
                    token=strtok(NULL," \t\n");
                    command[i] = token;
                    i++;
                }
                execCmd(command);
            }

        }

        if(fd > 0) {
            close(fd);
            fd = open("/dev/tty", O_WRONLY);
            stdout = fdopen(fd, "w");
        }

        if(argc == 1) {
            write(STDOUT_FILENO, promptMessage, strlen(promptMessage));
        }
    }

    fclose(file);

    return 0;
}
