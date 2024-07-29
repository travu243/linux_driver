#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(){
	
	pid_t p = fork();
	
	if(p<0){
		perror("fork fail"); 
    	exit(1);
	}
	else if ( p == 0){
        //child
        system("ls > ls_test.txt");
        return 0;
    }
     
    else{
        //Parent
		int wstatus;
		wait(&wstatus);
		if(wstatus==0){
			//success
			system("cat ls_test.txt");
		}
		if(wstatus!=0){
			//false
			puts("false");
			exit(1);
		}
	}
	return 0;
}





