/*
    This program simulates the GNU/Linux Shell
    Copyright © 2012, Chinmay Kulkarni
 
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>


void left_redirection( char **final, int i)
{
	int fd= open(final[i+1],  O_RDONLY, S_IRUSR);
	if (fd == -1) 
	{
		perror("Error! \n");		
		exit(errno);
	}
	final[i] = NULL;			
	int fd1 = dup2(fd,0);		
	assert(fd1 ==0);		
	dup2(fd,2);
	close(fd);
	execvp(final[0], final);  			
}


void right_redirection( char **final, int i)
{
	int fd= open(final[i+1], O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);
	if (fd == -1) 
	{
		perror("Error!  \n");		
		exit(errno);
	}
	final[i] = NULL;          //close file of file descriptor '1' i.e. stdout..NOTE: 0 -stdin, 1- stdout, 2- stderr
	int fd1 = dup2(fd,1);	  //dup(fd) assigns lowest available file descriptor to "fd"..which due to 
	assert(fd1 ==1);	  //close(1) will be 1(stdout)so,fd1==1.. dup2(fd,1) does combo of close(1) and dup(fd)
	dup2(fd, 2);		  //If assertion is false, terminate program..we do same for std_error (2).	
	close(fd);	  
	execvp(final[0], final);
}  			

void pipe_function(char **final, int loc, int count)
{
	char *child[64];
	char *parent[64];
	int m,i,j;
	int arr[2];
	pid_t pid;
	for (m=0; m<64; m++)
	{
		child[m]=(char *)(malloc(sizeof(char)*64));
		parent[m]=(char *)(malloc(sizeof(char)*64));
	}

	for ( i=0; i<loc; i++)
	{
		for(j=0; final[i][j]!=' ' && final[i][j]!='\0'; j++)
			child[i][j]=final[i][j];
	}
	child[i] = NULL;
	
	for (i=(loc+1); i<count; i++)
	{
		for(j=0; final[i][j]!=' ' && final[i][j]!='\0'; j++)
			parent[i-(loc+1)][j]=final[i][j];
	}
	parent[i-(loc+1)] = NULL;

/********** End of splitting***********/
	if (pipe(arr) == -1)
	{
		perror("Error!  Pipe operation failed.\n");
		exit(errno);
	}
	if ((pid = vfork()) < 0) 
	{
		perror("Error! Fork failed. Cannot create child process.\n");
		exit(errno);
	}

	if (pid == 0)							//child process
	{
		close(arr[0]);					
		dup2(arr[1],1);				//arr[0] is read end and arr[1] is write end of pipe
		close(arr[1]);
		execvp(child[0], child); 
		perror("execvp has failed \n");
		exit(errno);
	}
	else 								//parent process
	{	
		wait();
		close(arr[1]);
		dup2(arr[0],0);				
		close(arr[0]);	
		execvp(parent[0], parent);
		perror("execvp has failed \n");
		exit(errno);
	}
}

void execute(char **final, int count, int flag)
{
	int i,status,loc;
	for ( i=0; i < count; i++)
	{
		if (count == 0) return;
		
		if (final[i][0] == '&')
		{
			if (i != (count-1))
			{
				perror("Improper usage of '&' \n");
				exit(errno);
			}
			else
			{
				flag = 1;
				final[i]=NULL;
				daemon(1,1);			      //to run the given command in the background
			}
		}
	
		else if ((strcmp(final[0],"echo") == 0) && (strcmp(final[1], "$?") == 0))
			printf("%d\n", status);	

		else if (i == 0 && (strcmp(final[i],"cd") == 0))  
		{
			chdir(final[i+1]);
			//printf("You are now in:~ %s \n", (char*)(get_current_dir_name()));
		}
	
		else if (final[i][0] == '>')
		{	
			if (i==0 || i==(count-1)) 
			{
				perror("Error! Invalid position for '>'  \n");		
				exit(errno);
			}
			else 
				right_redirection(final, i);
		}

		else if (final[i][0] == '<' )
		{
			if (i==0 || i==(count-1)) 
			{
				perror("Error! Invalid position for '>'  \n");		
				exit(errno);
			}
			else 
				left_redirection(final, i);
		}
			
		else if (final[i][0] == '|')
		{
			if(i==0 || i==(count-1))		
			{
				perror("Error! Invalid position for '|'  \n");		
				exit(errno);
			}
			else
				pipe_function(final, i, count);
		}
		
		else if(i == (count-1))
		{
			if ((strcmp(final[0], "ls") == 0))		
			{
				final[count]="--color=auto";
				final[count+1]=NULL;
				execvp(final[0], final);
				printf("Erroneous execution \n");
			}
			execvp(final[0], final); 
		}		
		 				
	}
}

int main()
{
	pid_t pid;
	while(1)
	{
		int flag = 0;		
		int count = 0;		
		int i = 0;
		char ch;
		char *cmd;
		char *token = NULL;
		char *final[128];
		char *dir = (char*)(get_current_dir_name());
		printf("Chinmay_Shell-->>%s~$ ", dir);
		cmd = (char *) malloc(sizeof(char)*128);		
		while ((ch= getchar())!= '\n' )	
			cmd[i++] = ch;
		cmd[i] = '\0';	
		for ( i=0; i < 128; i++) 
			final[i] = (char *)malloc(sizeof(char)*128);
		for ( token = strtok(cmd, " "); (token!= NULL) && count<128; token = strtok(NULL, " ")) 
			strcpy(final[count++] , token);
		final[count] = NULL;
	
		pid = vfork();
		if(pid == -1)  						//error 
		{
			perror("Error! Cannot create process \n");
			exit(errno);
		}
		
		if(pid == 0)						//in child
			execute(final, count,flag);

		else							// in parent
		{
			if( flag == 0)
				wait();       				//wait for child only if flag == 0 i.e. 
		}							//don't wait in case of '&' (background)
	}
}




	
	




