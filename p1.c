
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

void redirect_input (char* file_name, int a_or_w);

void pwd_func ();

unsigned int plumb_pipe (unsigned int size_of_array, char* myargs[], char* arg_vector[], int pipe_fds[]);

int main (int argc, char *argv[]){

	int pipe_fds[2];

	char** myargs = 0;
	char** arg_vector = 0;


	/*Fork is used to 'split' the current process into two. 
	Fork returns a number less than 0 on failure. 
	Fork returns TWO values on success: PID of 0 == Child Process, 
	PID > 0 == Parent Process (Child's pid)
	*/

	//used when exiting to ensure that no more forks go through
	//**I'm mostly using it as a backup safeguard against continuing
	bool repeat_fork = true;

	while (repeat_fork == true){

		//ssize_t getline(char **lineptr, size_t *n, FILE *stream);

		//The maximum number of characters we can expect to see from the user 
		const unsigned int BUFFER_SIZE = 1024;

		//Used to get input from the command line in getline() below
		char input_buffer[BUFFER_SIZE];
		char* p_input_buffer = input_buffer;
		size_t size_of_buffer = BUFFER_SIZE - 1;

		unsigned int current_pos = 0;

		unsigned int num_pipes = 0;
		unsigned int num_appends = 0;
		unsigned int num_overwrites = 0;
		unsigned int redir_pos = 0;

		//used to change the color on the command line to cyan, then change back to default
		printf("\x1b[96m");
		printf("mysh> ");
		printf("\x1b[0m");

		if(getline(&p_input_buffer, &size_of_buffer, stdin) < 0){
			fprintf(stderr, "There was an Error Parsing the Command Line\n");
			continue;
		}
		

		//used to determine the number of commands/arguments so that we can appropriately malloc 
		unsigned int size_of_array = 1;
		unsigned int index_counter = 0;

		//printf("length of buffer string:%d\n", strlen(input_buffer));

		//counting the number of indices we will need in myargs
		for(unsigned int i = 0; i < strlen(input_buffer); i++){
			
			//If we reached the end of a word, count the word. if its a space and we haven't counted any characters, skip it. 
			if(isspace(input_buffer[i])){
				if(index_counter != 0){
					size_of_array++;
					index_counter = 0;
				}
				else {
					continue;
				}
			}	

			//if the character is a pipe and we reached the end of another word, count both. Otherwise, just the pipe.
			else if (input_buffer[i] == '|'){
				if(index_counter != 0){
					size_of_array++;
					index_counter = 0;
				}

				size_of_array++;
				num_pipes++;
			}

			//if we are redirecting, take note of what kind of character it is and add to the counter(s)
			else if (input_buffer[i] == '>'){
				if(index_counter != 0){
					size_of_array++;
					index_counter = 0;
				}

				if(input_buffer[i + 1] == '>'){
					i++;
					num_appends++;
				}

				else {
					num_overwrites++;
				}

				size_of_array++;
			}

			//if we just counted a character not mentioned above, take note of it and keep moving forward
			else {
				index_counter++;
			}

		}

		//printf("number of elements in the array:%d\n", size_of_array); 

		//If the user hits just the enter key we act like there was no command and move forward in the loop
		if(strlen(input_buffer) == size_of_array && strlen(input_buffer) < 2){
			continue;
		}

		//allocating the space we will need to hold all the arguments entered
		myargs = (char**)malloc(size_of_array * sizeof(char*));

		//set them all to NULL as both a precaution and as required by execvp down below
		for(unsigned int j = 0; j < size_of_array; j++){
			myargs[j] = NULL;
		}

		//**it would have been nice to put everything below into a function, but it requires a ton of variables we need 
		//**later on and the logic of each type of if statement is somewhat distinct from its peers 
		//below we will be copying the input from the command line into our myargs array
		//where we will begin copying characters from the input
		unsigned int copy_start = 0;
		//how many characters from that point to copy
		unsigned int chars_to_copy = 0;

		for(unsigned int p = 0; p < strlen(input_buffer); p++){

			//if its a space and we have no characters to copy, advance
			if(isspace(input_buffer[p]) && chars_to_copy == 0){
				copy_start++;
				continue;
			}

			//input we can use
			else {

				//if its a space and we have stuff to copy, count the word
				if(isspace(input_buffer[p]) || p == strlen(input_buffer) - 1){
					myargs[current_pos] = strndup(input_buffer + copy_start, chars_to_copy);
					//puts(myargs[current_pos]);
					current_pos++;
					copy_start = p + 1;
					chars_to_copy = 0;
				}

				//if it is a redirection character
				else if (input_buffer[p] == '>'){
					if(chars_to_copy > 0){
						myargs[current_pos] = strndup(input_buffer + copy_start, chars_to_copy);
						//puts(myargs[current_pos]);
						current_pos++;
						copy_start = p;
						chars_to_copy = 0;
						p--;
					}
					else {
						//if it is an append (>>) character
						if(input_buffer[p + 1] == '>'){
							myargs[current_pos] = strndup(input_buffer + copy_start, chars_to_copy + 2);
							//puts(myargs[current_pos]);
							redir_pos = current_pos;
							current_pos++;
							p++;
							copy_start = p + 1;
							chars_to_copy = 0;
						}

						//if it is an overwrite (>) character
						else {
							myargs[current_pos] = strndup(input_buffer + copy_start, chars_to_copy);
							redir_pos = current_pos;
							current_pos++;
							copy_start = p + 1;
							chars_to_copy = 0;
						}
					}
					
				}

				//it is a pipe character
				else if (input_buffer[p] == '|'){
					

					if(chars_to_copy > 0){
						myargs[current_pos] = strndup(input_buffer + copy_start, chars_to_copy);
						//puts(myargs[current_pos]);
						current_pos++;
						copy_start = p;
						chars_to_copy = 0;
						p--;
					}
					else {
						myargs[current_pos] = strndup(input_buffer + copy_start, chars_to_copy + 1);
						current_pos++;
						copy_start = p + 1;
						chars_to_copy = 0;
					}
				}

				//it is a character not named above
				else {
					chars_to_copy++;
				}
			}

		}

		//used in strcmp to determine if we should handle the function or if execvp should
		char* pwd = strdup("pwd");
		char* cd = strdup("cd");
		char* exit_program = strdup("exit");

		//if the command is pwd AND we aren't redirecting or piping input
		if (strcmp(myargs[0], pwd) == 0 && myargs[1] == NULL){

			pwd_func();

		}

		//if we wish to change the working directory
		else if (strcmp(myargs[0], cd) == 0){
			
			//int chdir(const char *path);

			//if the user simply inputs cd, we go to the home directory
			if(myargs[1] == NULL){

				char* home = getenv("HOME");

				chdir(home);
			}
			
			else {

				//if the directory we wish to go to does not exist
				if(access(myargs[1], F_OK) < 0){
					printf("The path '%s' could not be found\n", myargs[1]);
				}

				else {

					//if the path exists, change the working directory to it
					char* new_path = myargs[1];

					chdir(new_path);
				}
				
			}
			
			//***used to print the resulting file path
			//char* cwd;
			//char buff[PATH_MAX + 1];

			//cwd =
			//getcwd(buff, sizeof(buff));

			//!!!handle error type(S)!!!
			//printf("%s\n", cwd);

		}

		//if the user wishes to exit the program
		else if (strcmp(myargs[0], exit_program) == 0){
			
			repeat_fork = false;

			close(pipe_fds[0]);
			close(pipe_fds[1]);

			free(myargs);
			free(arg_vector);

			wait(NULL);

			exit(0);

		}

		else {

			//fork the process into a parent and child
			int rc = fork();

			if (rc < 0){
				fprintf(stderr, "There was an error with fork()\n");
				exit(1);
			}



			//this is the child process
			else if (rc == 0){

				if(num_pipes > 2){
					fprintf(stderr, "Error: User Entered Too Many Pipes\n");
					exit(1);
				}

				if(num_overwrites + num_appends > 1){
					fprintf(stderr, "Error: User Entered Too Many Redirection Characters\n");
					exit(1);
				}

				if(redir_pos != 0 && myargs[redir_pos + 1] == NULL){
					fprintf(stderr, "Error: User Did Not Enter a File Name\n");
					exit(1);
				}


				//while there are still unprocessed pipe commands
				while (num_pipes > 0){

					//get the location of the first pipe in the argumets list and set up a pipe
					unsigned int pipe_loc = plumb_pipe(size_of_array, myargs, arg_vector, pipe_fds);

					//we use the loop below to rearrange and delete the now unnecessary pipe function and arguments
					//**we are basically moving all the arguments after it forward in the array and NULLing where they were
					unsigned int j = 0;
					redir_pos -= pipe_loc + 1;

					pipe_loc++;

					while (pipe_loc < size_of_array){

						myargs[j] = myargs[pipe_loc];
						myargs[pipe_loc] = NULL;

						j++;
						pipe_loc++;
					}

					myargs[pipe_loc] = NULL;

					close(pipe_fds[1]);

					// the function needs its stdin connected to the read side of the pipe.
					// connection to current stdin must be closed.
					close(STDIN_FILENO);

					if (dup2(pipe_fds[0], STDIN_FILENO) < 0)
					{
						perror("dup2 in second child failed.");
						exit(EXIT_FAILURE);
					}

					// The read side of the pipe is duplicated at stdout. The
					// original file descriptor is now unnecessary.
					close(pipe_fds[0]);
				

					wait(NULL);

					//we have now processed one of the pipes
					num_pipes--;
				}
				

				//if the user wishes to append/overwrite to a file
				if(num_appends > 0 || num_overwrites > 0){
					if(num_appends > 0){

						redirect_input(myargs[redir_pos + 1], 0);

					}

					else if (num_overwrites > 0){

						redirect_input(myargs[redir_pos + 1], 1);

					}

					//cleaning up the redirection arguments because we have handled their set up
					while(redir_pos < size_of_array){
						//printf("%s", myargs[redir_pos]);
						myargs[redir_pos] = NULL;
						redir_pos++;
					}
				}
				
				//if the user wishes to redirect the output of pwd to a file
				if (strcmp(myargs[0], pwd) == 0){

					pwd_func();
					close(pipe_fds[0]);
					close(pipe_fds[1]);
					exit(0);
					
				}

				//every other function not handled above goes through here to be executed
				else {
					int err = 0;

					err = execvp(myargs[0], myargs);

					if (err != 0){
						//we were unable to read the command given, so we ensure that the command 
						//is cleared and this process is terminated
						printf("Error Executing Command '%s'\n", myargs[0]);
						myargs[0] = '\0';
						exit(1);
					}
				}

			}

			//this is the "original" parent process
			else {
				//wait(NULL);
				waitpid(rc, 0, 0);

				free(myargs);
				free(arg_vector);
				//printf("RC of the parent is: %d\n", rc);	
			}
		}
	}

	free(myargs);
	free(arg_vector);

	return 0;
}



//redirect the output to a file. We take the file to output to and the type of redirection
void redirect_input (char* file_name, int a_or_w){

	//First, we're going to open a file and close the current output
	//w for write, a for append
	close(STDOUT_FILENO);
	FILE* file;

	if(a_or_w == 0){
    	file = fopen(file_name, "a");
	}
	else {
    	file = fopen(file_name, "w");
	}

	if(file < 0){
    	fprintf(stderr, "Error Opening the File: %s", file_name);
    }

    //gets the file identifier for the file we just opened so that we can use it in dup2
    int file_identifier = fileno(file);
 
    //Now we redirect standard output to the file using dup2
    if(dup2(file_identifier, STDOUT_FILENO) < 0){
    	puts("Error Sending Output to File");
    	exit(EXIT_FAILURE);
    }
 
    //Now standard out has been redirected, we can write to the file

}

//used in two places to print the current working directory
void pwd_func (){
	//getting ready to find the current working directory
	//char *getcwd(char *buf, size_t size);

	char* cwd;
	char buff[PATH_MAX + 1];

	cwd = getcwd(buff, sizeof(buff));

	printf("%s\n", cwd);

}


//here we handle the plumbing required to pipe the output of a function into the input of another
unsigned int plumb_pipe (unsigned int size_of_array, char* myargs[], char* arg_vector[], int pipe_fds[]){

	//the location in our array where the pipe is located
	unsigned int pipe_loc = 0;

	//composing a secondary array that we can use to find and copy the pipe information over
	arg_vector = (char**)malloc(size_of_array * sizeof(char*));


	for(unsigned int j = 0; j < size_of_array; j++){
		arg_vector[j] = NULL;
	}

	for(unsigned int j = 0; j < size_of_array; j++){
		if(strcmp(myargs[j], "|") != 0){
			arg_vector[j] = myargs[j];
		}

		else {
			pipe_loc = j;
			break;
		}
	}

	if (pipe_loc == 0 || pipe_loc == size_of_array - 2){
		fprintf(stderr, "Error: User Gave the Pipe Insufficient Functions\n");
		exit(1);
	}


	if (pipe(pipe_fds) < 0)
	{
		perror("Failed to create pipe");
		exit(EXIT_FAILURE);
	}

	// pipe_fds[0] is a read side.
	// pipe_fds[1] is a write side.

	int pid;

	if ((pid = fork()) < 0){
		perror("First fork failed.");
		exit(EXIT_FAILURE);
	}

	else if (pid == 0){
		// This is the child

		//the read side can be closed immediately.
		close(pipe_fds[0]);

		// This child's current connection to the stdout must be closed.
		close(STDOUT_FILENO);
		if (dup2(pipe_fds[1], STDOUT_FILENO) < 0){

			perror("dup2 in child failed.");
			exit(EXIT_FAILURE);
		}

		// The write side of the pipe is duplicated at stdout. The
		// original file descriptor is now superfluous.
		close(pipe_fds[1]);

		//executing the function on the left of the pipe now that the plumbing has been taken care of
		execvp(arg_vector[0], arg_vector);

		fprintf(stderr, "First exec has failed.\n");
		exit(EXIT_FAILURE);
	}


	wait(NULL);

	//returning the location of the pipe so that we can clean up
	return pipe_loc;

}


