#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>
#include <strings.h>
#include <readline/readline.h>
#include <fcntl.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>
#include <sys/stat.h>
#include <readline/history.h>
#include "parse.h"   /*include declarations for parse-related structs*/

#define indexToExecute 0
#define MAX_JOBS_TO_STORE 1000
#define JOB_UPDATE_THRESHOLD 100
#define MAX_COMMAND_LENGTH 500
#define MAX_PATH_LENGTH 500
#define MAX_HISTORY_ELEMENTS 10000
#define MAX_BUILD_PROMPT_LENGTH 100

#define KILL_COMMAND "kill"
#define JOBS_COMMAND "jobs"
#define CD_COMMAND "cd"
#define HISTORY_COMMAND "history"
#define BUILD_PROMPT "prompt"
#define HELP1 "help"
#define PUSHD "pushd"
#define POPD "popd"
#define VIEWD "viewd"
#define FOREGROUND "fg"

int runningJobs[MAX_JOBS_TO_STORE], startedJobsCount = 0, historyCommandCount = 0;
char runningCommands[MAX_JOBS_TO_STORE][MAX_COMMAND_LENGTH], historyCommands[MAX_HISTORY_ELEMENTS][MAX_COMMAND_LENGTH];
char* buildPromptString;
char dir_stack[MAX_JOBS_TO_STORE][MAX_COMMAND_LENGTH];
int stack_ptr = 0;
parseInfo *info;
int *job_iter; /*tells the job number*/
int *background_jobs;/*holds the jobs*/
enum
BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, EXIT,JOBS,KILL,HISTORY,CD,HELP};


/*builds prompt fot shell with current directory address*/
char *
buildPrompt()
{
	char* cwd;
	char buff[2048];
	cwd = getcwd( buff, 2048); /*gets current directory address*/
	if(cwd != NULL) {
		strcat(cwd," $ "); /*concatenates with $ */
		return cwd;
	}

	else
		return "not "; /*if error in getcwd the not is the prompt*/
}

/*returns non-0 value for builtin command*/
int
isBuiltInCommand(char * cmd){

	if ( strncmp(cmd, "exit", strlen("exit")) == 0){
		return EXIT;
	}
	if ( strncmp(cmd, "help", strlen("help")) == 0){
		return HELP;
	}
	if ( strncmp(cmd, "kill", strlen("kill")) == 0){
		return KILL;
	}
	if ( strncmp(cmd, "cd", strlen("cd")) == 0){
		return CD;
	}
	if ( strncmp(cmd, "history", strlen("history")) == 0){
		return HISTORY;
	}
	if ( strncmp(cmd, "jobs", strlen("jobs")) == 0){
		return JOBS;
	}
	return NO_SUCH_BUILTIN;
}

/* To execute pipes */
void executePipe(){
	struct commandType *com;
	int pfds[2],status,in,out,i = 0;
	pid_t pid, pid1,pid2,pid3,pid4;
	char ** argvm;
	char *file[]={"cp","temp1","temp2",NULL};
	char x;
	pipe(pfds);
	pid = fork();
	if(pid == 0){
		com = &info->CommArray[0];
		argvm = com->VarList;
		out = open("temp1", O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
		dup2(out,1);
		close(out);
		if (execvp(*argvm, argvm) < 0) {
			printf("*** ERROR: exec failed. got it\n");
			exit(1);
		}
	}
	wait(&status);
	for(i = 1; i < info->pipeNum; i++){

		pid1 = fork();
		if(pid1==0){
			com = &info->CommArray[i];
			printf("%s\n",com->VarList[0]);
			argvm = com->VarList;
			pid3=fork();
			if(pid3==0)
			{
				if (execvp(*file, file) < 0) {
					printf("*** ERROR: exec failed. got it\n");
					exit(1);
				}
			}
			wait(&status);
			in = open("temp2", O_RDONLY);
			dup2(in,0);
			out = open("temp1", O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			dup2(out,1);
			close(out);
			if (execvp(*argvm, argvm) < 0) {
				printf("*** ERROR: exec failed. got it\n");
				exit(1);
			}
			wait(&status);
		}
		wait(&status);
	}
	pid4=fork();
	if(pid4==0)
	{
		if (execvp(*file, file) < 0) {
			printf("*** ERROR: exec failed. got itYO\n");
			exit(1);
		}
	}
	wait(&status);
	pid2 = fork();
	if(pid2 == 0){
		com = &info->CommArray[i];
		argvm = com->VarList;
		printf("%s\n",com->VarList[0]);
		in = open("temp2", O_RDONLY);
		dup2(in,0);
		close(in);
		if (execvp(*argvm, argvm) < 0) {
			printf("*** ERROR: exec failed. got it\n");
			exit(1);
		}
	}
	wait(&status);
}

void pushDirectory(struct commandType *CommandStruct) /* pushd command : adds directory to stack */
{
	pid_t  pid;
	int wait_dummy;
	if ((pid = fork()) < 0) {     /* fork a child process           */
		printf("Error: forking child process failed\n");
		exit(1);
	}
	else if (pid == 0) {

		char* cmd = (char*)malloc(MAX_COMMAND_LENGTH);
		int i=0;

		if(CommandStruct->VarList[1] == NULL)
		{
			fprintf(stdout, "Enter correct directory to push\n");
			exit(1);
		}
		sprintf(cmd,"%s",CommandStruct->VarList[1]);
		for(i=2; i<CommandStruct->VarNum; i++)
			sprintf(cmd,"%s %s",cmd,CommandStruct->VarList[i]);

		strcpy(dir_stack[stack_ptr] , cmd);
		stack_ptr += 1;

		chdir(CommandStruct->VarList[1]);

		getcwd(cmd, MAX_COMMAND_LENGTH);
	}


}


void popDirectory() /* popd command : removes directory from stack */
{
	pid_t  pid;
	int wait_dummy;
	if ((pid = fork()) < 0) {     /* fork a child process           */
		printf("Error: forking child process failed\n");
		exit(1);
	}
	else if (pid == 0) {
		if(stack_ptr == 0)
		{
			fprintf(stdout, "No directories added yet\n");
			exit(1);
		}
		chdir(dir_stack[stack_ptr-1]);
		stack_ptr -= 1;
	}
}

void viewDirectory()
{
	pid_t  pid;
	int wait_dummy;
	if ((pid = fork()) < 0) {     /* fork a child process           */
		printf("Error: forking child process failed\n");
		exit(1);
	}
	else if (pid == 0) {
		int i;
		if(stack_ptr == 0)
		{
			fprintf(stdout, "No other directories added yet\n");
			exit(1);
		}
		for(i = 0; i < stack_ptr; i++)
			fprintf(stdout, "%s\n", dir_stack[i]);
	}
}



/*general execute command*/
void execute(char **argvm) {
	pid_t  pid;
	int wait_dummy;
	if ((pid = fork()) < 0) {     /* fork a child process           */
		printf("Error: forking child process failed\n");
		exit(1);
	}
	else if (pid == 0) {          /* for the child process:         */

		if(info->boolBackground==1) { /*if background process is created*/
			printf("Background Process \n");
			background_jobs[*job_iter]=getpid(); /*get process id of the process*/
			*job_iter=*job_iter+1; /*increase iterate value*/
			printf("Jobs %d\n",*job_iter); /*prints number of jobs*/
		}

		if(info->boolInfile){ /*if infile is true*/
			int in = open(info->inFile, O_RDONLY); /*read only*/
			dup2(in,0); /*make this primary file*/
			close(in);
		}
		if(info->boolOutfile){ /*if outfile is true*/
			int out = open(info->outFile, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			dup2(out,1); /*make this secondary file to put output here*/
			close(out);
		}

		if (execvp(*argvm, argvm) < 0) {     /* execute the command  */
			printf("Error: exec failed\n");
			exit(1);
		}
	}
	else if (info->boolBackground==0){ /*If this is not a background process then wait for the process to complete*/
		wait(&wait_dummy);       /* wait for completion  */
	}
}



int
main (int argc, char **argv)
{

	pid_t pid1;
	int ret, hist=0, i=0, ret_child, status, flag=0,k=0;
	int ShmID = shmget(IPC_PRIVATE, 20*sizeof(int), IPC_CREAT | 0666); /*shared memory*/
	int ShmID2 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666); /*shared memory*/
	char * cmdLine, history[10000][80];
	struct commandType *com; /*com stores command name and Arg list for one command.*/
	/*initialises history to blank*/
	for(i=0;i<10;i++) {
		strcpy(history[i],"");
	}
	/*attach and initialize variables*/
	background_jobs = (int *) shmat(ShmID, NULL, 0);
	job_iter = (int *) shmat(ShmID2, NULL, 0);
	*job_iter=0;

#ifdef UNIX
	fprintf(stdout, "This is the UNIX/LINUX version\n");
#endif

#ifdef WINDOWS
	fprintf(stdout, "This is the WINDOWS version\n");
#endif

	while(1){
		/*insert your code to print prompt here*/

#ifdef UNIX
		cmdLine = readline(buildPrompt());
		if (cmdLine == NULL) {
			fprintf(stderr, "Unable to read command\n");
			continue;
		}
#endif

		/*insert your code about history and !x !-x here*/
		/*manages displaying of history*/
		if (cmdLine[0]=='!')  { /*seeing the past eg !-1*/
			if(cmdLine[1]== '-') {
				i=cmdLine[2]-'0';
				if(flag==1) {
					strcpy(cmdLine,history[(hist-i+10)%10]);
				}
				else if(flag==0 && ((hist-i)>=0)) {
					strcpy(cmdLine,history[(hist-i+10)%10]);
				}
				else {
					printf("No valid history\n");
				}
			}

			else { /*seeing the array eg !2 */
				i=cmdLine[1]-'0';
				if (flag==0) {
					if(((i-1)>=0) && ((i-1)<10))
						strcpy(cmdLine,history[i-1]);
					else
						printf("No valid history\n");
				}
				else {
					if(((i-1)>=0) && ((i-1)<10)) {
						strcpy(cmdLine,history[(hist+i-1)%10]);
					}
					else
						printf("No valid history\n");
				}
			}
		}

		/*storing history and telling with flag that it has exceeded 10*/
		if(hist>9) {
			hist=0;
			flag=1;
		}
		else {
			strcpy(history[hist],cmdLine);
			hist += 1;
		}


		/*calls the parser*/
		info = parse(cmdLine);
		if (info == NULL){
			free(cmdLine);
			continue;
		}
		/*prints the info struct*/
		print_info(info);

		/*com contains the info. of the command before the first "|"*/

		com=&info->CommArray[0];

		/*checks for background process and put it in background_jobs array*/
		ret_child = waitpid(-1, &status, WNOHANG);
		printf("ret number is %d\n",ret_child);
		if(ret_child > 0){
			for(i=0;i<*job_iter;i++){
				if(background_jobs[i] == ret_child){
					break;
				}
			}
			/*removing the terminated job from jobs list and shifting remaining jobs up*/
			for(;i<(*job_iter)-1;i++){
				background_jobs[i] = background_jobs[i+1];
			}
			*job_iter -= 1;
		}


		if ((com == NULL)  || (com->command == NULL)) {
			free_info(info);
			free(cmdLine);
			continue;
		}

		/*exit if no bakground process, else print the background process ID*/
		else if(isBuiltInCommand(com->command) == EXIT){
			if(*job_iter != 0){
				printf("Kill the following processes to exit");
				for(i=0;i<*job_iter;i++)
					printf("Process ID: %d\n",background_jobs[i]);
			}
			else
				exit(1);
		}

		/*executes kill pid, killes background process with a given pid*/
		else if(isBuiltInCommand(com->command) == KILL){
			if(com->VarList[1]==NULL) {
				printf("Invalid command\n");
			}
			else
				kill(atoi(com->VarList[1]),SIGKILL);
		}


		/*prints all the background jobs*/
		else if(isBuiltInCommand(com->command) == JOBS){
			printf("Total Jobs: %d\n",*job_iter);
			for(i=0;i<*job_iter;i++)
				printf("Process ID: %d\n",background_jobs[i]);
		}

		/*uses chdir to change director, spaces are allowed in folder name*/
		else if(strcmp(com->command,"cd") == 0) {
			char * temp =(char*) malloc(1024);
			strncpy(temp,cmdLine+3,strlen(cmdLine));
			ret = chdir((temp));
			if (ret != 0) {
				printf("Invalid Directory Address \n");
			}
			free(temp);
		}

		/*history command to display history in order*/
		else if (strcmp(cmdLine,"history") == 0) {
			i=hist;
			k=1;
			if(flag==1) {
				for(i=hist;i<10;i++)
					printf("%d %s \n",k++,history[i]);
				for(i=0;i<hist;i++)
					printf("%d %s \n",k++,history[i]);
			}
			else {
				for(i=0;i<hist+1;i++)
					printf("%d %s \n",k++,history[i]);
			}
		}
		/*prints help for built-in commands*/
		else if(isBuiltInCommand(com->command) == HELP){
			printf(" These are following built-in commands of the shell\n");
			printf("1. kill <process id> - Kills the process with the corresponding process id\n");
			printf("2. cd <directory name> - Will change your directory\n");
			printf("3. jobs - Provides a list of background processes with their process ids.\n");
			printf("4. history - Provides the list of your 10 latest commands.\n");
			printf("5. !<k> - k is an integer, if it is positive between [1,10] then it picks kth item from history output else if it is negative then it picks kth item from back\n");
			printf("6. exit - Terminates the shell\n");
			printf("7. help - Helps you like now\n");
		}
		/*To deal with one pipe */
		else if(info->pipeNum > 0)
			executePipe();

		if(strncmp(com->command, PUSHD,strlen(PUSHD))==0)  /* Pushd command  */
			pushDirectory(com);

		if(strncmp(com->command, POPD,strlen(POPD))==0)  /* Popd command  */
			popDirectory();

		if(strncmp(com->command, VIEWD,strlen(VIEWD))==0)  /* Viewd command  */
			viewDirectory();

		/*apart from all these command execute file commands*/
		else {
			execute(com->VarList); /*uses execvp nad fork mainly*/
		}

		free_info(info);
		free(cmdLine);
	}/* while(1) */
}
