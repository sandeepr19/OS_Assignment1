#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<errno.h>

int historyNumber;
int countOfCommands;

struct node {
	char *value;
	struct node *next;
};

void tokenize(char *tempBuffer, char *tokens[])
{
	char *token1 = strtok(tempBuffer, " ");
	int i = 0;
	while (token1) {
		tokens[i] = token1;
		token1 = strtok(NULL, " ");
		i++;
	}
}

/*
This command is used to check for duplicates before inserting path
*/
int checkForDuplicates(struct node **header, char *value)
{
	struct node *temp = *header;
	while (temp != NULL) {
		if (strcmp(temp->value, value) == 0)
			return 1;
		temp = temp->next;
	}
	return 0;
}

void insertNode(struct node **p, char *value, int count)
{
	struct node *newNode, *temp;
	if (count > 100) {
		temp = *p;
		*p = (*p)->next;
		free(temp);
	}
	if (count == -1 && checkForDuplicates(p, value)) {
		printf("error: Duplicate path cannot be inserted %s\n", value);
		return;
	}
	newNode = (struct node *)malloc(sizeof(struct node));
	if (newNode == NULL) {
		printf("error: %s\n", strerror(errno));
		exit(0);
	}
	newNode->value = value;
	newNode->next = NULL;
	if (*p == NULL)
		*p = newNode;
	else {
		temp = *p;
		while (temp->next != NULL)
			temp = temp->next;
		temp->next = newNode;
	}
	/*
	struct node *dummy;
	dummy = *p;
	while (dummy != NULL) {
		printf("data %d\n", dummy);
		//printf("%s", tokens[1]);
		printf("value %s\n", dummy->value);
		dummy = dummy->next;
	}
	*/
}

void deleteAllNodes(struct node **p)
{
	struct node *prev;
	while (*p != NULL) {
		prev = *p;
		(*p) = (*p)->next;
		free(prev);
	}
	free(*p);
}

void deleteNode(struct node **p, char *val)
{
	struct node *temp, *prev;
	temp = *p;
	while (temp != NULL) {
		if (strcmp(temp->value, val) == 0)
			break;
		prev = temp;
		temp = temp->next;
	}
	if (temp == NULL)
		printf("error: %s not present\n", val);
	else if (*p == temp)
		*p = temp->next;
	else {
		prev->next = temp->next;
		free(temp);
	}
}


void displayPath(struct node **p)
{
	struct node *temp;
	temp = *p;
	int i = 0;
	if (temp == NULL)
		return;
	while (temp != NULL) {
		if (i != 0)
			printf(":");
		printf("%s", temp->value);
		temp = temp->next;
		i++;
	}
	printf("\n");
}

void displayHistory(struct node **header)
{
	struct node *temp;
	temp = *header;
	int i = 1;
	while (temp != NULL) {
		printf("[%d] %s\n", i, temp->value);
		temp = temp->next;
		i++;
	}
}


void acceptInput(int exceedSize, int maxSize, char *inputBuffer, char *output)
{
	while (fgets(inputBuffer, maxSize, stdin) != NULL) {
		if (exceedSize)
			output = (char *) realloc(output,
				strlen(output) + strlen(inputBuffer) + 1);
		strcat(output, inputBuffer);
		if (inputBuffer[strlen(inputBuffer)-1] != '\n')
			exceedSize = 1;
		else
			break;
	}
}

void changeDirectory(char *tokens[], int count)
{
	if (count == 1) {
		printf("error: Please provide arguements\n");
	} else {
		int ret;
		ret = chdir(tokens[1]);
		if (ret != 0) {
			printf("error: %s\n", strerror(errno));
			exit(0);
		}
	}
}

/*
This command is used to delete the path and history lists
*/
void deleteLists(struct node **pathHeader, struct node **historyHeader)
{
	if (*pathHeader != NULL)
		deleteAllNodes(pathHeader);
	if (*historyHeader != NULL)
		deleteAllNodes(historyHeader);
	exit(0);
}

/*
This function is used to carry out different operations related to
path command such as displaying paths, adding and deleting path
*/
void processPath(struct node **pathHeader, int count, char *tokens[])
{
	int i;
	char *path;
	if (strcmp(tokens[1], "+") != 0 && strcmp(tokens[1], "-") != 0) {
		printf("error: Please provide valid arguements\n");
		return;
	}
	for (i = 2; i < count; i++) {
		path = malloc(strlen(tokens[i]) + 1);
		if (path == NULL) {
			printf("error: %s\n", strerror(errno));
			exit(0);
		}
		strcpy(path, tokens[i]);
		if (strcmp(tokens[1], "+") == 0)
			insertNode(pathHeader, path, -1);
		else if (strcmp(tokens[1], "-") == 0)
			deleteNode(pathHeader, path);
	}
}

/*
This function checks if a particular command is executable
*/
void checkForExecution(struct node **pathHeader, char *tokens[],
int countOfTokens)
{
	struct node *temp;
	temp = *pathHeader;
	char *arg;
	while (temp != NULL) {
		arg = malloc(strlen(temp->value) + strlen(tokens[0]) + 2);
		if (arg == NULL) {
			printf("error: %s\n", strerror(errno));
			exit(0);
		}
		strcpy(arg, temp->value);
		strcat(arg, "/");
		strcat(arg, tokens[0]);
		if (access(arg, X_OK) == 0)
			break;
		temp = temp->next;
	}
	if (temp != NULL) {
		char *args[countOfTokens+1];
		args[0] = arg;
		int i;
		for (i = 1; i < countOfTokens; i++)
			args[i] = tokens[i];
		args[i] = (char *)0;
		execv(args[0], args);
	} else {
		printf("error: Command cannot be executed\n");
	}
}

/*
This command is specefically used to call the execv command
*/
void execCommand(struct node **pathHeader, char *tokens[], int countOfTokens)
{
	if (access(tokens[0], X_OK) == 0) {
		char *args[countOfTokens+1];
		args[0] = tokens[0];
		int i;
		for (i = 1; i < countOfTokens; i++)
			args[i] = tokens[i];
		args[i] = (char *)0;
		execv(args[0], args);
	} else {
		checkForExecution(pathHeader, tokens, countOfTokens);
	}
}

/*
This function is used to execute path, cd and other commands which
require fork, execv calls
*/
void executeCommand(char *value, char *tokens[], struct node **pathHeader,
	struct node **historyHeader, int countOfTokens)
{
	countOfCommands++;
	insertNode(historyHeader, value, countOfCommands);
	if (strcmp(tokens[0], "path") == 0) {
		if (countOfTokens == 1)
			displayPath(pathHeader);
		else
			processPath(pathHeader, countOfTokens, tokens);
	} else if (strcmp(tokens[0], "cd") == 0) {
		changeDirectory(tokens, countOfTokens);
	} else {
		short pid = fork();
		if (pid < 0) {
			printf("error: %s\n", strerror(errno));
			return;
		}
		if (pid == 0) {
			execCommand(pathHeader, tokens, countOfTokens);
			exit(1);
		} else {
			while (pid != wait(0))
				;
		}
	}
}

int getTokenCount(char *output)
{
	char *token = strtok(output, " ");
	int numberOfTokens = 0;
	while (token) {
		token = strtok(NULL, " ");
		numberOfTokens++;
	}
	return numberOfTokens;
}

int checkInput(char *input)
{
	char *value = malloc(strlen(input));
	if (value == NULL) {
		printf("error: %s\n", strerror(errno));
		exit(0);
	}
	if (1 < strlen(input) && strlen(input) <= 4 && input[0] == '!') {
		strncpy(value, input+1, strlen(input)-1);
		if (atoi(value) <= 100 && atoi(value) > 0) {
			historyNumber = atoi(value);
			return 1;
		}
	}
	return 0;
}

/*
This function is used in case of !n commands
*/
void executeCommandFromHistory(struct node **pathHeader
	, struct node **historyHeader)
{
	struct node *temp;
	temp = *historyHeader;
	char *value, *tempBuffer;
	int i = 1;
	while (temp != NULL) {
		if (i == historyNumber)
			break;
		temp = temp->next;
		i++;
	}
	if (temp != NULL) {
		value = malloc(strlen(temp->value)+1);
		if (value == NULL) {
			printf("error: %s\n", strerror(errno));
			exit(0);
		}
		tempBuffer = malloc(strlen(temp->value)+1);
		if (tempBuffer == NULL) {
			printf("error: %s\n", strerror(errno));
			exit(0);
		}
		strcpy(value, temp->value);
		strcpy(tempBuffer, value);
		int numberOfTokens = getTokenCount(value);
		char *tokens[numberOfTokens];
		tokenize(tempBuffer, tokens);
		executeCommand(temp->value, tokens, pathHeader, historyHeader
			, numberOfTokens);
	} else
		printf("error: The %d th command is not present in history\n"
			, historyNumber);
}

int main(int argc, char **argv)
{
	int maxSize = 512;
	historyNumber = 0;
	countOfCommands = 0;
	int exceedSize = 0;
	char *output = (char *) malloc((maxSize));
	if (output == NULL) {
		printf("error: %s\n", strerror(errno));
		exit(0);
	}
	char inputBuffer[maxSize];
	char *tempBuffer, *value;
	struct node *pathHeader, *historyHeader ;
	pathHeader = NULL;
	historyHeader = NULL;
	int invalidArguement = 0;
	while (1) {
		printf("$ ");
		acceptInput(exceedSize, maxSize, inputBuffer, output);
		output[strlen(output)-1] = '\0';
		value = malloc((strlen(output)+1)*sizeof(char));
		if (value == NULL) {
			printf("error: %s\n", strerror(errno));
			exit(0);
		}
		strcpy(value, output);
		tempBuffer = (char *)malloc((strlen(output)) + 1);
		if (tempBuffer == NULL) {
			printf("error: %s\n", strerror(errno));
			exit(0);
		}
		strcpy(tempBuffer, output);
		int numberOfTokens = getTokenCount(output);
		char *tokens[numberOfTokens];
		tokenize(tempBuffer, tokens);
		/*
		int i ;
		for(i=0;i<numberOfTokens;i++)
			printf("%s tokens\n",tokens[i]);
		*/
		if (strcmp(tokens[0], "history") == 0) {
			displayHistory(&historyHeader);
		} else if (numberOfTokens == 1 && checkInput(tokens[0]))
			executeCommandFromHistory(&pathHeader, &historyHeader);
		else if (strcmp(tokens[0], "exit") == 0) {
			deleteLists(&pathHeader, &historyHeader);
			free(tokens);
			free(pathHeader);
			free(historyHeader);
			free(output);
			free(value);
			free(tempBuffer);
			free(inputBuffer);
			exit(0);
		} else if (!invalidArguement) {
			executeCommand(value, tokens, &pathHeader,
				&historyHeader, numberOfTokens);
		}
		output = (char *)malloc(maxSize);
		if (output == NULL) {
			printf("error: %s\n", strerror(errno));
			exit(0);
		}
		invalidArguement = 0;

	}
}

