
1) The code was changed to get the current working directory in the shell prompt.

2) Both absolute and relative pathnames are allowed to specify the commands.

3) STDIN and STDOUT, both were redirected using the dup2 command.

4) Running background commands were made possible using the & after the command.

5) A brief history of 10 commands were made possible using a FIFO list, using arrays.

6) !-i , !i , (i=integer) type of commands were made possible, where !-i means the last ith command and !i mean ith command in the history list.

7) The following built-in commands were also made : jobs, cd, history, exit, kill, help.

8) If the user choose to exits while background processes are running, he/she will be informend about them and can exit only after killing those processes.

9) The parser was not changed.

10) Makefile was changed, a flag -D_XOPEN_SOURCE=600 was added to remove a warning, because ipc.h header file requires that we use SVID or SUS standard.

11) Extra Feature : having spaces in folder name is allowed, and directory can be changed using the cd command. 

12) All the forking was taken care by the execute() command, to make the code look neater. Both direct use of strcmp and isBuiltInCommand() was used to check for certain command, for different cases, to show that both are possible.

13) "help" function was added to show all the built-in command.

14) The code is well commented for understanding. 

15) "ret number" was also added to all output to see the waitpid integer status, incase of a background process.

16) Pipes was added.

17) WC was added also.

18) pushd, popd and viewd was also added.
