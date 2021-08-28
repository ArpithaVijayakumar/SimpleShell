/******************************************************************************
 *
 *  File Name........: main.c
 *
 *  Description......: Simple driver program for ush's parser
 *
 *  Author...........: Arpitha Vijayakumar
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parse.h"
#include <dirent.h>
#include <errno.h>
static char *rootDir;
static char *host;
static char *cwd;
extern char **environ;

bool isBuiltInCommand(Cmd c)
{
  bool isBuiltIn = false;
  if(c == NULL){
    return isBuiltIn;
  }
  //Job control fg,bg,kill,jobs and &
  if ( !strcmp("bg", c->args[0]) || !strcmp("cd", c->args[0]) || !strcmp("fg", c->args[0]) || !strcmp("echo", c->args[0]) ||
    !strcmp("jobs", c->args[0]) || !strcmp("kill", c->args[0]) || !strcmp("logout", c->args[0]) || !strcmp("nice",c->args[0]) ||
    !strcmp("pwd",c->args[0]) || !strcmp("setenv",c->args[0]) || !strcmp("unsetenv",c->args[0]) || !strcmp("where",c->args[0]) ){
      isBuiltIn = true; 
    }
    
  return isBuiltIn;
}

void processCd(Cmd c)
{
  if(c->args[1]== NULL){
    if(chdir(rootDir)==0){
      strcpy(cwd,rootDir);
      }else{
        printf("Can not change diectory \n");
      }
    }else if(!strcmp(c->args[1],"~")){
      if(chdir(rootDir)==0){
      strcpy(cwd,rootDir);
      //printf("%s \n",cwd);
      }
      else{
        printf("Can not change diectory \n");
      }
    }else
    {
      if(chdir(c->args[1])==0){
        strcat(cwd,"/");
        strcat(cwd,c->args[1]);
        //printf("%s \n",cwd);
      }else{
        printf("No such file or directory \n");
      }     
    }  
}

void processEcho(Cmd c){
  for(int i =1; c->args[i]!= NULL; i++){
    printf("%s ", c->args[i]);
  }
  printf("\n");
}

void processLogout(Cmd c){
   exit(0);
}

void processPwd(Cmd c){
  printf("%s \n",cwd );  
}

void processSetEnv(Cmd c){
  if (c->args[1] == NULL){
    int i = 0;
    printf("\n");
    while(environ[i] != NULL){//https://stackoverflow.com/questions/4291080/print-the-environment-variables-using-environ
      printf("[%s] \n", environ[i]);
      i+=1;
   }
  }else if(c->args[1] != NULL && c->args[2] == NULL){//https://www.ibm.com/support/knowledgecenter/SSLTBW_2.4.0/com.ibm.zos.v2r4.bpxbd00/setenv.htm
    if(setenv(c->args[1], "", 1)!= 0){
      printf("Error setting the environment variables");
    }
  }else if(c->args[1] != NULL && c->args[2] != NULL){
    if(setenv(c->args[1], c->args[2], 1) != 0){
      printf("Error setting the environment variables");
    }
  }else{
    printf("Invalid number of arguments for setenv \n");
  }
}

void processUnSetEnv(Cmd c){
  if (c->args[1] == NULL)
    return;
  unsetenv(c->args[1]);
}

//https://c-for-dummies.com/blog/?p=1769
void processWhere(Cmd c){
  char *string,*found;
  DIR *dr;
  struct dirent *de;
    string = strdup(getenv("PATH"));
    //printf("Original string: '%s'\n",string);

    while( (found = strsep(&string,":")) != NULL ){

        dr = opendir(found);
    if (dr == NULL)  // opendir returns NULL if couldn't open directory 
    { 
        printf("Could not open current directory \n" );
        break; 
    } 
    while ((de = readdir(dr)) != NULL) {
      if(!strcmp(de->d_name,c->args[1])){
          printf("%s/%s\n",found,de->d_name);
      }
    }    
    closedir(dr);

    }
}

void processNice(Cmd c){
  int ret;
  if(c->args[1]==NULL){
    nice(4);
    return;
  }else if (c->args[1]!= NULL && c->args[2]== NULL){
    ret = nice(atoi(c->args[1]));
    if(errno == EPERM){
      printf("Access error \n");
    }
    return;
  }
  if(c->args[1]!= NULL && c->args[2]!= NULL){
    int rc = fork();
  if(rc < 0){
    printf("Fork failed\n");
    exit(1);
  }else if(rc == 0){
    ret = nice(atoi(c->args[1]));
    if(errno == EPERM){
      printf("Access error\n");
    }
    if(!strcmp(c->args[2], "cd")) 
      processCd(c);
    else if(!strcmp(c->args[2], "echo"))
      processEcho(c);
    else if(!strcmp(c->args[2], "logout"))
      processLogout(c);
    else if(!strcmp(c->args[2], "pwd"))
      processPwd(c);
    else if(!strcmp(c->args[2], "setenv"))
      processSetEnv(c);
    else if(!strcmp(c->args[2], "unsetenv"))
      processUnSetEnv(c);
    else if(!strcmp(c->args[2], "where"))
      processWhere(c);
    else{
    execvp(c->args[2], c->args+2);
    perror("execvp error");
    exit(0);
    }
  }else{ 
    waitpid(rc, NULL, 0);
  }
  return ;
  }
}

void processBuiltInCommand(Cmd c){
  if (!strcmp(c->args[0], "cd")) 
    processCd(c);
  else if (!strcmp(c->args[0], "echo"))
    processEcho(c);
  else if (!strcmp(c->args[0], "logout"))
    processLogout(c);
  else if (!strcmp(c->args[0], "nice"))
    processNice(c);
  else if (!strcmp(c->args[0], "pwd"))
    processPwd(c);
  else if (!strcmp(c->args[0], "setenv"))
    processSetEnv(c);
  else if (!strcmp(c->args[0], "unsetenv"))
    processUnSetEnv(c);
  else if (!strcmp(c->args[0], "where"))
    processWhere(c);
}

int processFileSystemCommand(Cmd c){
  int rc = fork();
  if(rc < 0){
    printf("Fork failed\n");
    exit(1);
  }else if(rc == 0){
    execvp(c->args[0], c->args);
    perror("execvp error \n");
    exit(0);
  }else{ 
    waitpid(rc, NULL, 0);
  }
  return 0;
}

void checkAndExecute(Cmd c){
  if(isBuiltInCommand(c))processBuiltInCommand(c);
  else processFileSystemCommand(c);
}

static void prCmd(Cmd c)
{
  int i;
  int inFp, oFp;
  int stdInFp , stdOutFp , stdErrFp;

  if ( c ) {
    //printf("%s%s ", c->exec == Tamp ? "BG " : "", c->args[0]);
    /*if(c->out == Tpipe && c->in == Tpipe){
      printf("INSIDE COUT AND CIN PIPE");
      checkAndExecute(c);
      return;
    }

    if(c->out == Tpipe){
      printf("INSIDE COUT PIPE");
      checkAndExecute(c);
      return;
    }*/
    
    if ( c->in == Tin ){
      stdInFp = dup(1);
      inFp = open(c->infile, O_RDONLY);
      if (inFp ==-1){
            printf("Error opening the file. Error Number % d\n", errno); // print which type of error have in a code
            return;
        }   
      dup2(inFp,0);
      close(inFp);
    }
    if ( c->out != Tnil ){
      stdOutFp = dup(1);
      stdErrFp = dup(0);
      switch ( c->out ) {
      case Tout:
        oFp = open(c->outfile, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRGRP | S_IWGRP | S_IXGRP);
        if (oFp ==-1){
            printf("Error opening the file. Error Number % d\n", errno); // print which type of error have in a code
            return;
        }                    
        dup2(oFp,1); // point c->outfile as stdout for current execution
        close(oFp);
        checkAndExecute(c);
        dup2(stdOutFp,1); //point back to standard output
        //printf(">(%s) ", c->outfile);
        break;
      case Tapp:
        oFp = open(c->outfile, O_CREAT | O_WRONLY | O_APPEND, S_IRWXU | S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRGRP | S_IWGRP | S_IXGRP);
        if (oFp ==-1){
            printf("Error opening the file. Error Number % d\n", errno); // print which type of error have in a code
            return;
        }            
        dup2(oFp,1); // point c->outfile as stdout for current execution
        close(oFp);
        checkAndExecute(c);
        dup2(stdOutFp,1); //point back to standard output
        //printf(">>(%s) ", c->outfile);
        break;
      case ToutErr:
        oFp = open(c->outfile, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRGRP | S_IWGRP | S_IXGRP);
        if (oFp ==-1){
            printf("Error opening the file. Error Number % d\n", errno); // print which type of error have in a code
            return;
        }            
        dup2(oFp,1); //point c->outfile as stdout for current execution
        dup2(oFp,2); //point c->outfile as stderr for current execution
        close(oFp);
        checkAndExecute(c);
        dup2(stdOutFp,1);//point back to standard output
        dup2(stdErrFp,2); //point back to standard output
        //printf(">&(%s) ", c->outfile);
        break;
      case TappErr:
        oFp = open(c->outfile, O_CREAT | O_WRONLY | O_APPEND , S_IRWXU | S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRGRP | S_IWGRP | S_IXGRP);
        if (oFp ==-1) {
            printf("Error opening the file. Error Number % d\n", errno); // print which type of error have in a code
            return;
        }            
        dup2(oFp,1); //point c->outfile as stdout for current execution
        dup2(oFp,2); //point c->outfile as stderr for current execution
        close(oFp);
        checkAndExecute(c);
        dup2(stdOutFp,1);//point back to standard output
        dup2(stdErrFp,2); //point back to standard output
        //printf(">>&(%s) ", c->outfile);
        break;
      case Tpipe:
        checkAndExecute(c);
        //if(isBuiltInCommand(c))processBuiltInCommand(c);
        //else execvp(c->args[0],c->args);
      //  printf("| ");
        break;
      case TpipeErr:
        checkAndExecute(c);
        //if(isBuiltInCommand(c))processBuiltInCommand(c);
        //  else execvp(c->args[0],c->args);
      //  printf("|& ");
       break;
      default:
        //fprintf(stderr, "Shouldn't get here\n");
        //exit(-1);
        break;
      }

    } else{
    checkAndExecute(c);
  }
  if ( c->in == Tin ){
      dup2(stdInFp,0);
      close(stdInFp);
    }
 }    
}

//https://gist.github.com/iomonad/a66f6e9cfb935dc12c0244c1e48db5c8
static void prPipe(Pipe p)
{
  pid_t pid;
  int i = 0;
  int j = 0;
  Cmd c;
  int fdd = 0;
  int nfd[2];
  if ( p == NULL )
    return;

  //printf("Begin pipe%s\n", p->type == Pout ? "" : " Error");
  for ( c = p->head; c != NULL; c = c->next ) {
      if (c->out == Tpipe || c->in == Tpipe || c->out == TpipeErr || c->in == TpipeErr){
      pipe(nfd);
      if ((pid = fork()) == -1) {
        perror("fork");
        exit(1);
      }
      else if (pid == 0) {
        //printf("---%d %d child",i,j);
        dup2(fdd, 0);
        if (c->out == Tpipe){
        dup2(nfd[1], 1);
      }
      if (c->out == TpipeErr){
        dup2(nfd[1], 1);
        dup2(nfd[1], 2);
      }
        close(nfd[0]);
        prCmd(c);
        exit(0);
    }
    else {
      //printf("---%d %d parent",i,j);
      waitpid(pid,NULL,0);    /* Collect childs */
      close(nfd[1]);
      fdd = nfd[0];
    }

  }
  else
  {
    prCmd(c);
  }
  }
 // printf("End pipe\n");
  prPipe(p->next);
}
/*-----------------------------------------------------------------------------
 *
 * Name...........: init()
 *
 * Description....: Setting host name and current working directory before executing any other function
 *
 * Input Param(s).: 
 *  none
 *
 * Return Value(s): none
 *
 */
void init(){
  host = malloc(256);
  cwd = malloc(1024);
  rootDir = malloc(1024);
  gethostname(host,256);
  getcwd(cwd,1024);
  strcpy(rootDir,cwd);
}

void executeUshrc(){
  int fp_stdin;
  char ushrc[8] ="/.ushrc";
  char fullPath[1024];
  strcpy(fullPath,rootDir);
  strcat(fullPath,ushrc);
  if( access( fullPath, F_OK ) != 0 ) {
    printf("File does not exist");
    return;
  }
  int fp_ushrc = open(fullPath, O_RDONLY);
  Pipe p;
  if(fp_ushrc == -1)
    return;
  fp_stdin = dup(fileno(stdin));
  dup2(fp_ushrc,0);
  close(fp_ushrc);
  p = parse();
  prPipe(p);
  freePipe(p);
  dup2(fp_stdin,0);
}

int main(int argc, char *argv[])
{
  int ret;
  Pipe p;
  init();
  signal(SIGTERM,SIG_DFL); // catches term signal and configured to default
  signal(SIGQUIT,SIG_IGN); // ignoring quit signal
  executeUshrc();
  fflush(NULL); 
  while ( 1 ) {
    printf("[%s%%  ~]$",host);
    p = parse();
    prPipe(p);
    freePipe(p);
  }
}

/*........................ end of main.c ....................................*/

