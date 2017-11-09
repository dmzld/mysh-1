#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "commands.h"
#include "built_in.h"

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

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])//second validate
{
  if (n_commands > 0) {
    struct single_command* com = (*commands);//com = struct first value

    assert(com->argc != 0);//exception code 

    int built_in_pos = is_built_in_command(com->argv[0]);// 

    if (built_in_pos != -1) {//if built_in_pos exist & no error
		
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
		int pid;
		if((pid=fork())<0){
			printf("fork error");
		}else if(pid==0){
			if(strcmp(com->argv[0],"ls")==0
				|| strcmp(com->argv[0],"cat")==0){
				char path[]="/bin/";
				strcat(path,com->argv[0]);
				com->argv[0]=path;
				//free path ??
				execv(com->argv[0],com->argv);
			} else if(strcmp(com->argv[0],"vim")==0){
				char path[]="/usr/bin/";
				strcat(path,com->argv[0]);
				com->argv[0]=path;
				//free path??
				execv(com->argv[0],com->argv);
			} else{//there is full path in argv[0]
				execv(com->argv[0],com->argv);
			}
		}else{
			wait(&pid);//&pid <- correct?
		}
		//fprintf(stderr, "%s: command not found\n", com->argv[0]);
      //return -1;
    }
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
