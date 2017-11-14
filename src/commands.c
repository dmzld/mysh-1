#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <pthread.h>

#include "commands.h"
#include "built_in.h"

#define SERVER "/tmp/test_server"

void* thread_server(void * commands);
int creation(struct single_command *com);

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};


static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;//???
    }
  }

  return -1; // Not found
}


 /* Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])//second validate
{
  if (n_commands > 0) {
    struct single_command* com = (*commands);
    
	if(n_commands == 1){
		assert(com->argc!=0);
		return creation(com);
		//return 0;
	} else{
		
		struct single_command* com2 = (*commands+1);
		int client_sock;
		struct sockaddr_un server_addr;
		//create thread
		pthread_t thread_id;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_create(&thread_id, &attr, thread_server, com2);//server(thread)
		
		//child
		if(fork()==0){
			if((client_sock = socket(PF_FILE,SOCK_STREAM,0))==-1){
				printf("c_socket error\n");
				exit(1);
			}
			memset(&server_addr,0,sizeof(server_addr));
			server_addr.sun_family = AF_UNIX;
			strcpy(server_addr.sun_path,SERVER);
			//execv(a)
			while(1){
				if(connect(client_sock,(struct sockaddr*)&server_addr,sizeof(server_addr))==-1){
					printf("connect error\n");
					exit(1);
				}
				
				dup2(client_sock,1);
				creation(com);
				close(client_sock);
				exit(0);
			}
//			pthread_join(thread_id,NULL);
		}
	}
  }	
	return 0;
}

	
		
int creation(struct single_command *com){
	//assert(com->argc != 0);
    int built_in_pos = is_built_in_command(com->argv[0]);// 
	int status;
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {//if error in  vaildate
		if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {//if error in do
			fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
	  } else {
  	     fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
  	     return -1;
      }
	} else if (strcmp(com->argv[0], "") == 0) {
  	    return 0;
	} else if (strcmp(com->argv[0], "exit") == 0) {
		return 1;
	} else {
		int pid3;
		if((pid3=fork())<0){
			printf("fork error");
		} else if(pid3==0){
			
			if(strchr(com->argv[0],'/')==NULL){//path resolution
				char fpath[128];
				char PATH[]="/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin";
				char* ptr2 = strtok(PATH,":");
							
				while(ptr2 != NULL){
					strcpy(fpath,ptr2);
					strcat(fpath,"/");
					strcat(fpath,com->argv[0]);
					execv(fpath,com->argv);
					ptr2 = strtok(NULL,":");
					}
	
			} else{//there is full path in argv[0]
				execv(com->argv[0],com->argv);
			}

		} else{
			wait(&status);
		}
		return 0;
	}
	return 0;
}
	



void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;//address++
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);//initialize
}

void* thread_server(void *commands){
	
	struct single_command *com2 = (struct single_command *)commands;
	int status,c_size;
	int server_sock, client_sock;
	struct sockaddr_un server_sockaddr,client_sockaddr;
	
	if((access(SERVER,F_OK)==0)){
		unlink(SERVER);
	}
	//socket
	server_sock = socket(PF_FILE, SOCK_STREAM, 0);
	memset(&server_sockaddr, 0, sizeof(server_sockaddr));
	//memset(&client_sockaddr, 0, sizeof(client_sockaddr));
	server_sockaddr.sun_family = AF_UNIX;
	strcpy(server_sockaddr.sun_path, SERVER);
	
	//bind
	if((bind(server_sock, (struct sockaddr*)&server_sockaddr,sizeof(server_sockaddr)))==-1){
		printf("bind error\n");
		//close(server_sock);
		exit(1);
	}
	
	while(1){
		//listen
		if((listen(server_sock,10))==-1){
			printf("listen error\n");
			exit(1);
		}
		//accept
		c_size = sizeof(client_sockaddr);
		client_sock = accept(server_sock, (struct sockaddr *)&client_sockaddr,&c_size);
		if(client_sock ==-1){
			printf("accept error\n");
			exit(1);
		}
		//b
		if(fork()==0){
			dup2(client_sock,0);
			creation(com2);//fork()
			close(client_sock);
			close(server_sock);
			exit(0);
		
		}
		else wait(&status);
		close(client_sock);
		close(server_sock);
		break;
	}
	pthread_exit(0);
}
