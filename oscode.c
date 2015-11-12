#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <strings.h>

typedef char HISTORY [10][100];

int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line;
    size_t linecap = 0;

    printf("%s", prompt);
    fflush(stdout);
    length = getline(&line, &linecap, stdin);

    if (length <= 0) {
        exit(-1);
    }

    // Check if background is specified..
    if ((loc = index(line, '&')) != NULL) {
        *background = 1;
        //*loc = ' ';
    } else
        *background = 0;

    while ((token = strsep(&line, " \t\n")) != NULL) {
    	int j = 0;
        for (j; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }
    return i;
}


void freecmd (char *args[], int count){   //reset  command
    
    int i = 0;
    for(i;i<count;i++)
    {
    	if(args[i] != NULL)
    	{
    		args[i] = NULL;
    	}
    }
}


void save(HISTORY history, int index , char* args[])
{
	int nIndex = (index+1)%10;
	char space[5] = " ";
    char command[100];
    int i = 0;
    strcpy(command,args[i]);
    i++;
    while(args[i]!=NULL)       //cat args into one string divided by space
    {
        strcat(command,space);
        strcat(command,args[i]);
        i++;
    }
  
    //copy args into history
    char *r = history[nIndex];
    int g = 0;
    for(g; g < 100; g++, r++)
     {
    
    char c = command[g];

    if(c != '\0'||c != '\n')				//check if the end of the string 
    {
    	if (c != ' ') 
    	{
     		*r = command[g];
    	} 
    	else 
    	{
      		*r = ' ';
    	}
	}else break;
  }
 
 
}

void print(HISTORY history, int index){
	int i = 0;
	int temp;
	
	temp = index%10;
	
	//h1,h2 are two help variables that help printing out right index of history
	int h1 = 0; //initialize h1
	for(i = temp;i>=0;i--)
	{
		
		if (history[i][0] != '\0' && history[i][0] != '\n')
		{
			printf("%d %s\n",index-h1,history[i] );

		}
		h1++;
    }
    
    int h2 = temp+1; //initialize h2
	for(i = 9;i>temp;i--)
	{
		if (history[i][0] != '\0' && history[i][0] != '\n')
		{
			printf("%d %s\n",index-h2,history[i] );
		}
		h2++;
	}
	
}

//help to change the history back to args
void tokenhelper(char *args[], int *background, char history[])
{
   	int i = 0;
    char *token, *loc;

    // Check if background is specified..
    if ((loc = index(history, '&')) != NULL) {
        *background = 1;
        //*loc = ' ';
    } else
        *background = 0;

    while ((token = strsep(&history, " \t\n")) != NULL) {
    	int j = 0;
        for (j; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }
  
}


int main(void)
{
    char *args[20];
    int bg;
    HISTORY history;
    int index = 0;
    /*
    / make sure every history shows in different line
    */
    int h=0;
    for(h;h<10;h++)
    {
    	history[h][0] ='\n';
    }
    
    int pidIndex = 0;
    pid_t pidlist[10];



while(1){
    bg = 0;
	int cnt = getcmd("\n>>  ", args, &bg);
 	//make sure "non NULL terminated args array" errors not happen
    if(cnt < 20)
    {
        args[cnt] = NULL;
    }
    else
        args[19] = NULL;

    if(cnt == 0) continue;//continue if nothing typed
    /*-----------------------------------------------------*/
    if(strcasecmp(args[0],"history")==0 && cnt == 1)
    {
      
        save(history,index++,args);
        print(history,index);
        freecmd(args,cnt);
        continue;
    }
    /*
    / checking cd
    */

    else if(strcasecmp(args[0],"cd")==0 && cnt == 2)
    {
    	
    	chdir(args[1]);
        save(history,index++,args);
        freecmd(args,cnt);
        continue;
        
    
    }
    /*
    / pwd
    */
    else if(strcasecmp(args[0],"pwd")==0 && cnt == 1)
    {
        char* cwd;
        char* buffer;
        buffer = (char*)malloc(50); //allocate memory to a buffer to save the path
        printf("%s\n", buffer);
        cwd = getcwd(buffer,50);
        if(cwd!=NULL)
        {
            printf("%s\n",cwd);
        }
       free(buffer);//free the buffer
       save(history,index++,args);
       freecmd(args,cnt);
       continue;
    }
    /*
    * exit() kill all processes before exit
    */
	else if(strcasecmp(args[0],"exit")==0 && cnt == 1)
	{
		int i = 0;
		for(i;i<10;i++)
		{
			if(pidlist[i]!=0)
			{
				kill(pidlist[i],SIGTERM);
			}
		}
		exit(0);
	}
	/*
	*jobs
	*/
	else if(strcasecmp(args[0],"jobs")==0 && cnt == 1)
	{
		
		
		int i = 0;
		for(i;i<10;i++)
		{
			if(pidlist[i]!=0)
			{
				if(kill(pidlist[i],0)==0)
				{
					printf("[%d] %d\n",i,pidlist[i]);
				}
				else
				{
					pidlist[i] = 0;
				}
			}
		}
		save(history,index++,args);
		freecmd(args,cnt);
		continue;
	}
	/*
	*fg
	*/
	else if (strcasecmp(args[0],"fg")==0 && cnt == 2)
	{
		save(history,index++,args);
		int jobid = atoi(args[1]);
		pid_t pidfg = pidlist[jobid%10];

		if(pidfg>0){
			kill(pidfg,SIGCONT);		//bring the process to the foreground
			int status;
			waitpid(pidfg,&status,WUNTRACED); 
		}
		freecmd(args,cnt);
		continue;
	}

	/* recall command */


	 if(strcasecmp(args[0],"r")==0)
	{	
	
		int sindex = -2;
		if(cnt == 1)					//only r
		{
			
			sindex = (index)%10;

		}
		else if(cnt == 2) 				//r x
		{
			
			int k = 0;
			for(k;k<10;k++)
			{
				sindex = (index)%10;
				int sindex_ = abs(sindex-k);
				
				char c = *args[1];
				if(history[sindex_][0] == c)
				{
					sindex = sindex_;
					break;
				}
				else
				{
					sindex = -2;
				}


			 }
			
		}
		
		if(sindex!=-2){
			
			freecmd(args,cnt);		//reset args

			printf("RECALL COMMAND: %s\n",history[sindex]);
			
			tokenhelper(args,&bg,history[sindex]); //retoken command 
			//printf("%s\n",args[0] );
			//have to encode the previous code to achieve the goals
			//there is a better way to achieve,but I just don't have time to modify
			if(strcasecmp(args[0],"history")==0)
    			{
      
        		save(history,index++,args);
        		print(history,index);
        		freecmd(args,cnt);
        		continue;
    			}
    		 else if(strcasecmp(args[0],"cd")==0)
    			{
    			chdir(args[1]);
        		save(history,index++,args);
        		freecmd(args,cnt);
        		continue;
       			}
       		else if(strcasecmp(args[0],"jobs")==0)
				{
				int i = 0;
				for(i;i<10;i++)
					{
						if(pidlist[i]!=0)
						{
							if(kill(pidlist[i],0)==0)
							{
								printf("[%d] %d\n",i,pidlist[i]);
							}
							else
							{
								pidlist[i] = 0;
							}
						}
					}
					save(history,index++,args);
					freecmd(args,cnt);
					continue;
				}
			else if (strcasecmp(args[0],"fg")==0)
				{
					save(history,index++,args);
					int jobid = atoi(args[1]);
					pid_t pidfg = pidlist[jobid%10];

					if(pidfg>0){
						kill(pidfg,SIGCONT);		//bring the process to the foreground
						int status;
						waitpid(pidfg,&status,WUNTRACED); 
					}
					freecmd(args,cnt);
					continue;
				}
			fflush(stdout);
		}
	}


 
    
    pid_t pid ;
    pid = fork();

    if (pid < 0) {
      printf("Error\n");
      int i;
  	  for (i = 0; i < 10; i++) {
    		if (pidlist[i] != 0) {
      		kill(pidlist[i], SIGTERM);
    		}
  		}
    }

    if(pid == 0){
        //Child process
        return execvp(args[0],args);
    }
    else if(pid<0){
        perror("fork error");
    }
    else{
        //parent process
		int status;
        
        if (bg == 0){
			status = waitpid(pid, &status, WUNTRACED); 
            save(history,index++,args);
        }
        
        else{
			printf("[%d] %d - background\n", pidIndex,pid);
        	pidlist[pidIndex] = pid;
        	pidIndex = (pidIndex+1)%10; //length of pidlist is 10
        	kill(pid,SIGCONT);
        	save(history,index++,args);
        }
        freecmd(args,cnt); // reset command
    }
   
	}
}
