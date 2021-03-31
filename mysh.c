#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

//function prototypes
void executeInstruction(char *inst, char **tokenInst);
void printTheList();
void addToList(char *nextInput);

// each input will have directions
typedef struct currInput
{
  char *instruction;
  struct currInput *next;
} currInput;

//global variable currInput Head, current directory, and how many instructions we received.
currInput *head;
char *currentdir;
int instructions;

void makeCopy(char **tokenInst)
{
  // from file is tokenInst[1]
  // to file is tokenInst[2]
  FILE *fromFile = NULL;
  FILE *toFile = NULL;
  int freeOne, freeTwo = 0;
  char ch;
  char *fromPath;
  char *toPath;

  // relative path was sent as the source file.
  if (tokenInst[1][0] != '/')
  {
    fromPath = malloc(sizeof(char) * (strlen(currentdir) + strlen(tokenInst[1]) + 4));
    sprintf(fromPath, "%s/%s", currentdir, tokenInst[1]);
    freeOne = 1;
  }

  else
    fromPath = tokenInst[1];

  if (tokenInst[2][0] != '/')
  {
    toPath = malloc(sizeof(char) * (strlen(currentdir) + strlen(tokenInst[2]) + 4));
    sprintf(toPath, "%s/%s", currentdir, tokenInst[2]);
    freeTwo = 1;
  }

  else
    toPath = tokenInst[2];

  // need read permissions from here.
  fromFile = fopen(fromPath, "r");
  // need write permissions to here.
  toFile = fopen(toPath, "w");

  // ensure that both files were opened correctly.
  if (fromFile == NULL && toFile == NULL)
  {
    printf("Error opening BOTH the SOURCE and DESTINATION files.");
  }

  else if (fromFile == NULL)
  {
    printf("Error opening the SOURCE file. Make sure your path is correct.");
    // close the file that was actually opened and free memory.
    fclose(toFile);
  }

  else if (toFile == NULL)
  {
    printf("Error opening the DESTINATION file. Make sure your path is correct.");
    // close the file that was actually opened and free memory.
    fclose(fromFile);
  }

  // both files were opened successfully. Copy character by character into destination.
  else
  {
    while ((ch = fgetc(fromFile)) != EOF)
    {
      fputc(ch, toFile);
    }
    // print out success message.
    printf("\nSuccessfully copied contents from %s -> into -> %s\n", fromPath, toPath);
    // close up the files.
    fclose(fromFile);
    fclose(toFile);
  }
  // free the files that were malloced.
  if (freeOne == 1)
    free(fromPath);
  if (freeTwo == 1)
    free(toPath);
}

int checkName(char *fName)
{
  struct stat buffer;

  char *tempo = malloc(sizeof(char) * (strlen(currentdir) + strlen(fName) + 4));

  strcat(tempo, currentdir);
  strcat(tempo, "/");
  strcat(tempo, fName);

  if (stat(tempo, &buffer) == 0)
  {
    if (S_ISDIR(buffer.st_mode))
    {
      printf("Abode is.\n");
    }

    else
    {
      printf("Dwelt indeed.\n");
    }

    return 1;
  }

  else
  {
    printf("Dwelt not.\n");
  }
  free(tempo);
  return 0;
}

void makeTheFile(char *fName)
{
  int filedescriptor;
  int absolute = -1;
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  char *pathFile = NULL;
  FILE *f_write;

  // a relative path is being used.
  if (fName[0] != '/')
  {
    pathFile = malloc(sizeof(char) * (strlen(currentdir) + strlen(fName) + 4));

    sprintf(pathFile, "%s/%s", currentdir, fName);

    filedescriptor = open(pathFile, O_WRONLY | O_EXCL | O_CREAT | O_TRUNC, mode);
  }

  // an absolute path was sent.
  else
  {
    absolute = 1;
    filedescriptor = open(fName, O_WRONLY | O_EXCL | O_CREAT | O_TRUNC, mode);
  }

  if (filedescriptor < 0 && absolute == 1)
    printf("ERROR: FILENAME ALREADY EXISTS OR YOU SENT A BAD PATH...");

  else if (filedescriptor < 0)
    printf("ERROR: FILENAME ALREADY EXISTS");

  else
  {
    if (absolute == 1)
      f_write = fopen(fName, "w");
    else
    {
      f_write = fopen(pathFile, "w");
      free(pathFile);
    }

    fputs("Draft\n", f_write);

    if (absolute != 1)
      printf("File: %s was created successfully in path: %s\n", fName, currentdir);

    else
      printf("File was created successfully in path: %s\n", fName);

    if (fclose(f_write))
    {
      perror("Failed closing the file... terminating program");
      exit(1);
    }
  }
}

// function iteratively reverses the linked list holding the history.
void reverseTheList(currInput *root)
{
  currInput *next = NULL;
  currInput *current = root;
  currInput *prev = NULL;

  // iteratively reverse the linked list.
  while (current != NULL)
  {
    next = current->next;
    current->next = prev;
    prev = current;
    current = next;
  }

  head = prev;
}

currInput *readHistory()
{
  char *fileName = "savedHistory.txt";
  FILE *filePointer;
  char *str = malloc(sizeof(char) * 2048);
  int counter = 0;
  // currInput *temp = NULL;

  filePointer = fopen(fileName, "r");

  // check to see if the file is in this folder
  if (filePointer == NULL)
  {
    printf("%s\n", "No file history found.");
    return NULL;
  }

  else
    printf("%s\n", "File history was read successfully.");

  char *input = NULL;

  while (fgets(str, 2048, filePointer) != NULL)
  {
    input = malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(input, str);

    for (int i = 0; i < strlen(input); i++)
    {
      // remove new line character
      if (input[i] == '\n')
        input[i] = '\0';
    }

    // add each new line into the history
    addToList(input);
    free(input);
  }

  // reverse the list because it was read in backwards.
  reverseTheList(head);

  fclose(filePointer);

  return head;
}

// writes the data from the linked list to a file...
void writeDataFile()
{
  char *fileName = "savedHistory.txt";
  FILE *filePointer;
  currInput *current = head;

  // write to file named savedHistory
  filePointer = fopen(fileName, "w");

  if (filePointer == NULL)
  {
    fprintf(stderr, "%s\n", "There was an issue saving the history file");
    fclose(filePointer);
  }

  while (current != NULL)
  {
    if (strlen(current->instruction) > 1)
      fprintf(filePointer, "%s\n", current->instruction);

    current = current->next;
  }

  fclose(filePointer);
}

// adds the newest line of input to the list of previous inputs...
void addToList(char *nextInput)
{
  // new currInput is created...
  currInput *temp = malloc(sizeof(currInput));
  temp->instruction = malloc(sizeof(char) * strlen(nextInput));
  strcpy(temp->instruction, nextInput);

  temp->next = head;
  head = temp;
}

// clears the linked list holding my history, recursively
void clearTheList(currInput *root)
{
  if (root == NULL)
    return

        clearTheList(root->next);

  free(root);
}

// goes through each node in linked list to print history.
void printTheList()
{
  currInput *temp = head;
  int counter = 0;

  while (temp != NULL)
  {
    if (strlen(temp->instruction) > 1)
    {
      printf("%d: %s\n", counter, temp->instruction);
      counter++;
    }

    temp = temp->next;
  }
}

// much of this code was based off of code I found online for tokenizing input in C.
char **tokenizeInput(char *input)
{
  // create a buffer for each "word" separated by spaces...
  int count = 0;
  int charAmount = 128;
  int increment = 128;

  // global variable tracking how many instructions were tokenized on last call to tokenizeInput
  instructions = 0;
  char **tokens = malloc(sizeof(char *) * charAmount);
  char *token = NULL;

  // make sure memory can be allocated...
  if (tokens == NULL)
  {
    fprintf(stderr, "%s\n", "Error allocating memory in the tokenizer");
    exit(1);
  }

  // pass the input and the possible deliminators to string tokenizer...
  token = strtok(input, " \t\r\n\a");

  // keep appending tokens to the token array until we reach the end...
  while (token != NULL)
  {
    tokens[count++] = token;

    // indicates we need more token space
    if (count >= charAmount)
    {
      charAmount += increment;

      tokens = realloc(tokens, sizeof(char *) * charAmount);

      if (tokens == NULL)
      {
        fprintf(stderr, "%s\n", "Error re-allocating memory in the tokenizer");
        exit(1);
      }
    }

    // get the next token
    token = strtok(NULL, " \t\r\n\a");
  }

  // mark the final position as NULL so we know where to stop...
  tokens[count] = NULL;
  instructions = count;
  // printf("Instructions updated to %d\n", instructions);

  return tokens;
}

// Most of this code was found from a snippet online regarding reading input that could
// possibly be any size... I believe it came from stackoverflow, but I'm not sure anymore.
char *readInput()
{
  int count = 0;
  int bufferSize = 1024;
  int increment = 1024;
  char *buffer = malloc(sizeof(char) * bufferSize);
  int c;

  // make sure memory was properly allocated...
  if (buffer == NULL)
  {
    fprintf(stderr, "Error allocating memory\n");
    exit(1);
  }

  while (1)
  {
    // read each character, one at a time...
    c = getchar();

    // if we hit the EOF, replace it with a null terminator, instead.
    if (c == EOF || c == '\n')
    {
      buffer[count] = '\0';
      return buffer;
    }

    else
    {
      buffer[count] = c;
    }
    count++;

    // if we exceed the buffer, reallocate
    if (count >= bufferSize)
    {
      bufferSize += increment;
      buffer = realloc(buffer, bufferSize);

      // make sure the buffer was reallocated properly...
      if (buffer == NULL)
      {
        fprintf(stderr, "%s\n", "Error re-allocating more space in readInput");
        exit(1);
      }
    }
  }
  // just in case...
  return buffer;
}

void replayNum(char *inst)
{
  int number = atoi(inst);
  // increment number because we just had replay come through.
  number++;
  int count = 0;

  currInput *temp = head;

  while (temp != NULL)
  {
    // this is the instruction we were looking for.
    if (number == count)
    {
      // get the instruction and get a tokenized version of the instruction, then send it
      // to executeInstruction.
      char *tempString = malloc(sizeof(char) * (strlen(temp->instruction) + 1));
      strcpy(tempString, temp->instruction);

      char **tempToken = tokenizeInput(tempString);

      executeInstruction(tempString, tempToken);

      free(tempToken);

      free(tempString);

      break;
    }

    temp = temp->next;

    if (strlen(temp->instruction) > 1)
      count++;
  }

  if (number > count || number < 0)
    printf("Sent an invalid number to replay\n");
}

// executes a program and prints out its PID
pid_t runInBackground(char **tokenInst)
{
  pid_t pid;
  char **args;
  int status;

  // if we did not receive a path to the executable, append it to the end of currentdir
  if (tokenInst[1][0] != '/')
  {
    char ch = '/';
    char *end = "\0";

    // allocate space for all arguments
    args = malloc(sizeof(char *) * (instructions));
    // allocate space for the first argument string
    args[0] = malloc(sizeof(char *) * (strlen(currentdir) + 2) + strlen(tokenInst[1]));
    // create a string that takes currentdir and appends / and \0 onto it.
    args[0] = strcat(args[0], currentdir);
    args[0] = strcat(args[0], &ch);
    args[0][strlen(currentdir) + 1] = '\0';
    args[0] = strcat(args[0], tokenInst[1]);

    int i;

    // take the rest of the tokenized input and put them into arguements array
    for (i = 1; i < instructions - 1; i++)
    {
      args[i] = malloc(sizeof(char *) * strlen(tokenInst[i + 1] + 1));
      args[i] = strcpy(args[i], tokenInst[i + 1]);
    }
    // last argument should be null
    args[i] = NULL;
  }

  // we received a path to the executable
  else
  {
    // if we receive a path to where the executable is located...
    args = malloc(sizeof(char *) * instructions);
    int i;

    // copy tokenized strings into arguments list.
    for (i = 0; i < instructions - 1; i++)
    {
      args[i] = malloc(sizeof(char *) * strlen(tokenInst[i + 1] + 1));
      args[i] = strcpy(args[i], tokenInst[i + 1]);
    }
    args[i] = NULL;
  }

  // fork so we don't terminate our current terminal.
  pid = fork();

  // Indicates we are in the child partition.
  if (pid == 0)
  {
    printf("The PID of the new program is %d\n", (int)getpid());

    // ensure program runs correctly.
    if (execv(args[0], args) == -1)
    {
      fprintf(stderr, "%s\n", "Failed to execute the program.");
    }
    exit(1);
  }
  // Indicates a forking error.
  else if (pid < 0)
  {
    // Error forking
    fprintf(stderr, "%s\n", "Error forking");
  }

  return 1;
}

// starts an executable program
int startTheProgram(char **tokenInst)
{
  pid_t pid, wpid;
  char **args;
  int status;

  // if we did not receive a path to the executable, append it to the end of currentdir
  if (tokenInst[1][0] != '/')
  {
    char ch = '/';
    char *end = "\0";

    // allocate space for all arguments
    args = malloc(sizeof(char *) * (instructions));
    // allocate space for the first argument string
    args[0] = malloc(sizeof(char *) * (strlen(currentdir) + 2) + strlen(tokenInst[1]));
    // create a string that takes currentdir and appends / and \0 onto it.
    args[0] = strcat(args[0], currentdir);
    args[0] = strcat(args[0], &ch);
    args[0][strlen(currentdir) + 1] = '\0';
    args[0] = strcat(args[0], tokenInst[1]);

    int i;

    // take the rest of the tokenized input and put them into arguements array
    for (i = 1; i < instructions - 1; i++)
    {
      args[i] = malloc(sizeof(char *) * strlen(tokenInst[i + 1] + 1));
      args[i] = strcpy(args[i], tokenInst[i + 1]);
    }
    // last argument should be null
    args[i] = NULL;
  }

  // we received a path to the executable
  else
  {
    // if we receive a path to where the executable is located...
    args = malloc(sizeof(char *) * instructions);
    int i;

    // copy tokenized strings into arguments list.
    for (i = 0; i < instructions - 1; i++)
    {
      args[i] = malloc(sizeof(char *) * strlen(tokenInst[i + 1] + 1));
      args[i] = strcpy(args[i], tokenInst[i + 1]);
    }
    args[i] = NULL;
  }

  // fork so we don't terminate our shell.
  pid = fork();

  // Indicates we are in the child partition.
  if (pid == 0)
  {
    // check that execution worked properly.
    if (execv(args[0], args) == -1)
    {
      fprintf(stderr, "%s\n", "Failed to execute the program.");
      exit(1);
    }
  }
  // Indicates a forking error.
  else if (pid < 0)
  {
    perror("Error forking");
  }
  // Indicates we are in the parent process
  else
  {
    // wait for the child process to terminate before continuing.
    do
    {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

// kill the PID that is passed in...
void killCall(char *PIDtoKill)
{
  int pidNum = atoi(PIDtoKill);
  pid_t killReturn;

  // call the kill function
  killReturn = kill(pidNum, SIGKILL);

  if (killReturn)
  {
    printf("%s\n", "Unable to kill process");
  }

  else
  {
    printf("%s\n", "Process killed successfully");
  }
}

// {repeat || 4 || /Users/phillipmitchell/Desktop}
void repeatProcess(char **tokenInst)
{
  pid_t pid;
  char **args;
  int status;

  // if we did not receive a path to the executable, append it to the end of currentdir
  if (tokenInst[1][0] != '/')
  {
    char ch = '/';
    char *end = "\0";

    // allocate space for multiple arguments
    args = malloc(sizeof(char *) * (instructions));

    // allocate space for the first argument string
    args[0] = malloc(sizeof(char *) * (strlen(currentdir) + 2) + strlen(tokenInst[1]));
    args[0] = strcat(args[0], currentdir);
    // append the / character to the end of the path...
    args[0] = strcat(args[0], &ch);
    args[0][strlen(currentdir) + 1] = '\0';
    // append the null terminator to the end of the string
    args[0] = strcat(args[0], tokenInst[1]);

    int i;

    for (i = 1; i < instructions - 2; i++)
    {
      // put the tokenized strings into the argument list
      args[i] = malloc(sizeof(char *) * strlen(tokenInst[i + 1] + 1));
      args[i] = strcpy(args[i], tokenInst[i + 1]);
    }
    // mark the last argument as NULL.
    args[i] = NULL;
  }

  // we received a path to the executable
  else
  {
    // if we receive a path to where the executable is located, allocate space for args
    args = malloc(sizeof(char *) * (instructions - 1));
    int i;

    for (i = 0; i < instructions - 2; i++)
    {
      // put each tokenized input into the argument array.
      args[i] = malloc(sizeof(char *) * strlen(tokenInst[i + 1] + 1));
      args[i] = strcpy(args[i], tokenInst[i + 1]);
    }
    args[i] = NULL;
  }

  // fork here so process can run while not terminating our terminal.
  pid = fork();

  // Indicates we are in the child partition.
  if (pid == 0)
  {
    // printf("PID is %d, ", (int)getpid());

    // check for an error executing the command.
    if (execv(args[0], args) == -1)
    {
      fprintf(stderr, "%s\n", "Failed to execute the program.");
    }
  }
  // Indicates a forking error.
  else if (pid < 0)
  {
    // Error forking
    fprintf(stderr, "%s\n", "Error forking");
  }
  // Parent process
  else
  {
    // print the parent PID
    printf("%d, ", (int)pid);
  }

  // free the arguments we created before returning.
  for (int j = 0; j < instructions - 1; j++)
    free(args[j]);

  free(args);

  return;
}

// sees what the user types in and calls the appropriate helper command.
void executeInstruction(char *inst, char **tokenInst)
{
  // No command was sent...
  if (inst == NULL || strcmp(inst, "") == 0 || (strlen(inst) < 2))
  {
    printf("%s\n", "Not a valid command");
    return;
  }

  // whereami command prints current directory
  if (strcmp(inst, "whereami") == 0)
  {
    printf("%s\n", currentdir);
  }

  // history command prints the current history
  else if (strcmp(inst, "history") == 0)
  {
    printTheList();
  }

  // Clear history. Reset head to NULL as there is no longer any history.
  else if (strcmp(inst, "history -c") == 0)
  {
    clearTheList(head);
    head = NULL;
  }

  // user is requesting to move to a different directory.
  else if (strcmp(tokenInst[0], "movetodir") == 0)
  {
    if (instructions < 2)
      printf("%s\n", "Must Specify a directory to move to.");

    else
    {
      // allocate space and create a string for the new path, which will be appended
      // to currentdir.
      char *temp = malloc(sizeof(char) * 2048);
      strcat(temp, currentdir);
      strcat(temp, tokenInst[1]);

      char *fullpath = realpath(temp, NULL);

      // see if the path the user entered is a real path...
      if (fullpath == NULL)
        printf("%s\n", "Directory does not exist");

      else
      {
        printf("Successfully changed directory to %s\n", fullpath);
        currentdir = fullpath;
      }
      free(temp);
    }
  }

  // make file but it's spelled maik
  else if (strcmp(tokenInst[0], "maik") == 0)
  {
    if (instructions < 2)
    {
      printf("Not enough instructions passed... only see %d instruction\n", instructions);
    }

    // else if (checkName(tokenInst[1]) == 1)
    // {
    //   printf("ERROR: FILE NAME ALREADY EXISTS");
    // }

    else
    {
      makeTheFile(tokenInst[1]);
    }
  }

  // replay an old command
  else if (strcmp(tokenInst[0], "replay") == 0)
  {

    if (instructions < 2)
    {
      printf("Not enough instructions passed... only see %d instructions\n", instructions);
    }

    else
    {
      int stringLen = strlen(tokenInst[1]);

      int i = 0;
      int good = 1;

      // check that a number was sent as the second value...
      for (i = 0; i < stringLen; i++)
      {
        if (isdigit(tokenInst[1][i]) == 0)
        {
          printf("%s\n", "Please pass in a number as the second parameter for replay");
          good = 0;
          break;
        }
      }
      // user sent a valid instruction, so replay it.
      if (good)
        replayNum(tokenInst[1]);
    }
  }

  // start the program the user requested.
  else if (strcmp(tokenInst[0], "start") == 0)
  {
    if (instructions < 2)
      printf("%s\n", "Not enough instructions were passed with argument");

    else
      startTheProgram(tokenInst);
  }

  // start program user requested as a background process.
  else if (strcmp(tokenInst[0], "background") == 0)
  {
    if (instructions < 2)
      printf("%s\n", "Not enough instructions were passed with argument");

    else
      runInBackground(tokenInst);
  }

  // kill command
  else if (strcmp(tokenInst[0], "dalek") == 0)
  {
    if (instructions < 2)
      printf("%s\n", "Not enough instructions were passed with this argument");

    else
      killCall(tokenInst[1]);
  }

  // dwelt command
  else if (strcmp(tokenInst[0], "dwelt") == 0)
  {
    if (instructions < 2)
      printf("%s\n", "Not enough instructions were passed with this argument");

    else
      checkName(tokenInst[1]);
  }

  // extra credit for repeat
  else if (strcmp(tokenInst[0], "repeat") == 0)
  {
    if (instructions < 3)
      printf("%s", "Not enough instructions were passed with this argument");

    else
    {
      printf("PIDs: ");

      for (int j = 0; j < atoi(tokenInst[1]); j++)
        repeatProcess(tokenInst + 1);
    }

    printf("\n");
  }

  else if (strcmp(tokenInst[0], "coppy") == 0)
  {
    if (instructions < 3)
      printf("%s", "Not enough instructions were passed with this argument");

    else
      makeCopy(tokenInst);
  }

  // User sent a command that is not supported by the shell.
  else
    printf("%s\n", "Invalid/Unsupported command.");
}

// program loops until user types in "byebye"
void runMainProgram()
{
  char *input = NULL;
  char **tokenizedInput = NULL;
  int status;

  while (1)
  {
    printf("\n# ");
    input = readInput();

    if (strcmp(input, "byebye") == 0)
    {
      writeDataFile();
      exit(0);
    }

    addToList(input);

    char *tempString = malloc(sizeof(char) * (strlen(input) + 1));
    strcpy(tempString, input);

    tokenizedInput = tokenizeInput(tempString);

    executeInstruction(input, tokenizedInput);

    free(tempString);
  }
}

int main()
{
  // get the current directory at the start of running the program and set it as currentdir
  char cwd[1000];
  getcwd(cwd, sizeof(cwd));
  currentdir = cwd;

  // initialize head to NULL before starting, then read in any possible history.
  head = NULL;
  head = readHistory();

  // run the main program...
  runMainProgram();

  // free up the linked list of history.
  clearTheList(head);
  // free(cwd);

  return 0;
}
