#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SUCCESS 0
#define FAILURE (-1)

/*
 * LinkTable Node Type
 */
typedef struct LinkTableNode {
  struct LinkTableNode *pNext;
} tLinkTableNode;

/*
 * LinkTable Type
 */
typedef struct LinkTable tLinkTable;

/*
 * LinkTable Type
 */
struct LinkTable {
  tLinkTableNode *pHead;
  tLinkTableNode *pTail;
  int SumOfNode;
  pthread_mutex_t mutex;
};

/*
 * Create a LinkTable
 */
tLinkTable *CreateLinkTable() {
  tLinkTable *pLinkTable = (tLinkTable *)malloc(sizeof(tLinkTable));
  if (pLinkTable == NULL) {
    return NULL;
  }
  pLinkTable->pHead = NULL;
  pLinkTable->pTail = NULL;
  pLinkTable->SumOfNode = 0;
  pthread_mutex_init(&(pLinkTable->mutex), NULL);
  return pLinkTable;
}

/*
 * Delete a LinkTable
 */
int DeleteLinkTable(tLinkTable *pLinkTable) {
  if (pLinkTable == NULL) {
    return FAILURE;
  }
  while (pLinkTable->pHead != NULL) {
    tLinkTableNode *p = pLinkTable->pHead;
    pthread_mutex_lock(&(pLinkTable->mutex));
    pLinkTable->pHead = pLinkTable->pHead->pNext;
    pLinkTable->SumOfNode -= 1;
    pthread_mutex_unlock(&(pLinkTable->mutex));
    free(p);
  }
  pLinkTable->pHead = NULL;
  pLinkTable->pTail = NULL;
  pLinkTable->SumOfNode = 0;
  pthread_mutex_destroy(&(pLinkTable->mutex));
  free(pLinkTable);
  return SUCCESS;
}

/*
 * Add a LinkTableNode to LinkTable
 */
int AddLinkTableNode(tLinkTable *pLinkTable, tLinkTableNode *pNode) {
  if (pLinkTable == NULL || pNode == NULL) {
    return FAILURE;
  }
  pNode->pNext = NULL;
  pthread_mutex_lock(&(pLinkTable->mutex));
  if (pLinkTable->pHead == NULL) {
    pLinkTable->pHead = pNode;
  }
  if (pLinkTable->pTail == NULL) {
    pLinkTable->pTail = pNode;
  } else {
    pLinkTable->pTail->pNext = pNode;
    pLinkTable->pTail = pNode;
  }
  pLinkTable->SumOfNode += 1;
  pthread_mutex_unlock(&(pLinkTable->mutex));
  return SUCCESS;
}

/*
 * Delete a LinkTableNode from LinkTable
 */
int DelLinkTableNode(tLinkTable *pLinkTable, tLinkTableNode *pNode) {
  if (pLinkTable == NULL || pNode == NULL) {
    return FAILURE;
  }
  pthread_mutex_lock(&(pLinkTable->mutex));
  if (pLinkTable->pHead == pNode) {
    pLinkTable->pHead = pLinkTable->pHead->pNext;
    pLinkTable->SumOfNode -= 1;
    if (pLinkTable->SumOfNode == 0) {
      pLinkTable->pTail = NULL;
    }
    pthread_mutex_unlock(&(pLinkTable->mutex));
    return SUCCESS;
  }
  tLinkTableNode *pTempNode = pLinkTable->pHead;
  while (pTempNode != NULL) {
    if (pTempNode->pNext == pNode) {
      pTempNode->pNext = pTempNode->pNext->pNext;
      pLinkTable->SumOfNode -= 1;
      if (pLinkTable->SumOfNode == 0) {
        pLinkTable->pTail = NULL;
      }
      pthread_mutex_unlock(&(pLinkTable->mutex));
      return SUCCESS;
    }
    pTempNode = pTempNode->pNext;
  }
  pthread_mutex_unlock(&(pLinkTable->mutex));
  return FAILURE;
}

/*
 * Search a LinkTableNode from LinkTable
 * int Conditon(tLinkTableNode * pNode);
 */
tLinkTableNode *SearchLinkTableNode(tLinkTable *pLinkTable,
                                    int (*Conditon)(tLinkTableNode *pNode,
                                                    void *args),
                                    void *args) {
  if (pLinkTable == NULL || Conditon == NULL) {
    return NULL;
  }
  tLinkTableNode *pNode = pLinkTable->pHead;
  while (pNode != NULL) {
    if (Conditon(pNode, args) == SUCCESS) {
      return pNode;
    }
    pNode = pNode->pNext;
  }
  return NULL;
}

/*
 * get LinkTableHead
 */
tLinkTableNode *GetLinkTableHead(tLinkTable *pLinkTable) {
  if (pLinkTable == NULL) {
    return NULL;
  }
  return pLinkTable->pHead;
}

/*
 * get next LinkTableNode
 */
tLinkTableNode *GetNextLinkTableNode(tLinkTable *pLinkTable,
                                     tLinkTableNode *pNode) {
  if (pLinkTable == NULL || pNode == NULL) {
    return NULL;
  }
  tLinkTableNode *pTempNode = pLinkTable->pHead;
  while (pTempNode != NULL) {
    if (pTempNode == pNode) {
      return pTempNode->pNext;
    }
    pTempNode = pTempNode->pNext;
  }
  return NULL;
}

tLinkTable *head = NULL;

#define CMD_MAX_LEN 1024
#define CMD_MAX_ARGV_NUM 32
#define DESC_LEN 1024
#define CMD_NUM 10

char prompt[CMD_MAX_LEN] = "Input Cmd >";

/* data struct and its operations */

typedef struct DataNode {
  tLinkTableNode *pNext;
  char *cmd;
  char *desc;
  int (*handler)(int argc, char *argv[]);
} tDataNode;

int SearchConditon(tLinkTableNode *pLinkTableNode, void *arg) {
  char *cmd = (char *)arg;
  tDataNode *pNode = (tDataNode *)pLinkTableNode;
  if (strcmp(pNode->cmd, cmd) == 0) {
    return SUCCESS;
  }
  return FAILURE;
}
/* find a cmd in the linklist and return the datanode pointer */
tDataNode *FindCmd(tLinkTable *head, char *cmd) {
  tDataNode *pNode = (tDataNode *)GetLinkTableHead(head);
  while (pNode != NULL) {
    if (!strcmp(pNode->cmd, cmd)) {
      return pNode;
    }
    pNode = (tDataNode *)GetNextLinkTableNode(head, (tLinkTableNode *)pNode);
  }
  return NULL;
}

/* show all cmd in listlist */
int ShowAllCmd(tLinkTable *head) {
  tDataNode *pNode = (tDataNode *)GetLinkTableHead(head);
  while (pNode != NULL) {
    printf("    * %s - %s\n", pNode->cmd, pNode->desc);
    pNode = (tDataNode *)GetNextLinkTableNode(head, (tLinkTableNode *)pNode);
  }
  return 0;
}

int Help(int argc, char *argv[]) {
  ShowAllCmd(head);
  return 0;
}

int SetPrompt(char *p) {
  if (p == NULL) {
    return 0;
  }
  strcpy(prompt, p);
  return 0;
}
/* add cmd to menu */
int MenuConfig(char *cmd, char *desc, int (*handler)(int, char **)) {
  tDataNode *pNode = NULL;
  if (head == NULL) {
    head = CreateLinkTable();
    pNode = (tDataNode *)malloc(sizeof(tDataNode));
    pNode->cmd = "help";
    pNode->desc = "Menu List";
    pNode->handler = Help;
    AddLinkTableNode(head, (tLinkTableNode *)pNode);
  }
  pNode = (tDataNode *)malloc(sizeof(tDataNode));
  pNode->cmd = cmd;
  pNode->desc = desc;
  pNode->handler = handler;
  AddLinkTableNode(head, (tLinkTableNode *)pNode);
  return 0;
}

/* Menu Engine Execute */
int ExecuteMenu() {
  /* cmd line begins */
  while (1) {
    int argc = 0;
    char *argv[CMD_MAX_ARGV_NUM];
    char cmd[CMD_MAX_LEN];
    char *pcmd = NULL;
    printf("%s", prompt);
    /* scanf("%s", cmd); */
    pcmd = fgets(cmd, CMD_MAX_LEN, stdin);
    if (pcmd == NULL) {
      continue;
    }
    /* convert cmd to argc/argv */
    pcmd = strtok(pcmd, " ");
    while (pcmd != NULL && argc < CMD_MAX_ARGV_NUM) {
      argv[argc] = pcmd;
      argc++;
      pcmd = strtok(NULL, " ");
    }
    if (argc == 1) {
      int len = strlen(argv[0]);
      *(argv[0] + len - 1) = '\0';
    }
    tDataNode *p =
        (tDataNode *)SearchLinkTableNode(head, SearchConditon, (void *)argv[0]);
    if (p == NULL) {
      continue;
    }
    printf("%s - %s\n", p->cmd, p->desc);
    if (p->handler != NULL) {
      p->handler(argc, argv);
    }
  }
}

#define FONTSIZE 10
int PrintMenuOS() {
  int i, j;
  char data_M[FONTSIZE][FONTSIZE] = {
      "          ", "  *    *  ", " ***  *** ", " * *  * * ", " * *  * * ",
      " *  **  * ", " *      * ", " *      * ", " *      * ", "          "};
  char data_e[FONTSIZE][FONTSIZE] = {
      "          ", "          ", "    **    ", "   *  *   ", "  *    *  ",
      "  ******  ", "  *       ", "   *      ", "    ***   ", "          "};
  char data_n[FONTSIZE][FONTSIZE] = {
      "          ", "          ", "    **    ", "   *  *   ", "  *    *  ",
      "  *    *  ", "  *    *  ", "  *    *  ", "  *    *  ", "          "};
  char data_u[FONTSIZE][FONTSIZE] = {
      "          ", "          ", "  *    *  ", "  *    *  ", "  *    *  ",
      "  *    *  ", "  *    *  ", "   *  **  ", "    **  * ", "          "};
  char data_O[FONTSIZE][FONTSIZE] = {
      "          ", "   ****   ", "  *    *  ", " *      * ", " *      * ",
      " *      * ", " *      * ", "  *    *  ", "   ****   ", "          "};
  char data_S[FONTSIZE][FONTSIZE] = {
      "          ", "    ****  ", "   **     ", "  **      ", "   ***    ",
      "     **   ", "      **  ", "     **   ", "  ****    ", "          "};

  for (i = 0; i < FONTSIZE; i++) {
    for (j = 0; j < FONTSIZE; j++) {
      printf("%c", data_M[i][j]);
    }
    for (j = 0; j < FONTSIZE; j++) {
      printf("%c", data_e[i][j]);
    }
    for (j = 0; j < FONTSIZE; j++) {
      printf("%c", data_n[i][j]);
    }
    for (j = 0; j < FONTSIZE; j++) {
      printf("%c", data_u[i][j]);
    }
    for (j = 0; j < FONTSIZE; j++) {
      printf("%c", data_O[i][j]);
    }
    for (j = 0; j < FONTSIZE; j++) {
      printf("%c", data_S[i][j]);
    }
    printf("\n");
  }
  return 0;
}

int Quit(int argc, char *argv[]) {
  /* add XXX clean ops */
  exit(0);
}

int Time(int argc, char *argv[]) {
  time_t tt;
  struct tm *t;
  tt = time(NULL);
  t = localtime(&tt);
  printf("time:%d:%d:%d:%d:%d:%d\n", t->tm_year + 1900, t->tm_mon, t->tm_mday,
         t->tm_hour, t->tm_min, t->tm_sec);
  return 0;
}

int TimeAsm(int argc, char *argv[]) {
  time_t tt;
  struct tm *t;
  asm volatile("mov $0,%%ebx\n\t"
               "mov $0xd,%%eax\n\t"
               "int $0x80\n\t"
               "mov %%eax,%0\n\t"
               : "=m"(tt));
  t = localtime(&tt);
  printf("time:%d:%d:%d:%d:%d:%d\n", t->tm_year + 1900, t->tm_mon, t->tm_mday,
         t->tm_hour, t->tm_min, t->tm_sec);
  return 0;
}

int Fork(int argc, char *argv[]) {
  int pid;
  /* fork another process */
  pid = fork();
  if (pid < 0) {
    /* error occurred */
    fprintf(stderr, "Fork Failed!");
    exit(-1);
  } else if (pid == 0) {
    /*	 child process 	*/
    printf("This is Child Process!\n");
    exit(0);
  } else {
    /* 	parent process	 */
    printf("This is Parent Process!\n");
    /* parent will wait for the child to complete*/
    wait(NULL);
    printf("Child Complete!\n");
  }
  return 0;
}

int Exec(int argc, char *argv[]) {
  int pid;
  /* fork another process */
  pid = fork();
  if (pid < 0) {
    /* error occurred */
    fprintf(stderr, "Fork Failed!");
    exit(-1);
  } else if (pid == 0) {
    /*	 child process 	*/
    printf("This is Child Process!\n");
    execlp("./hello", "hello", NULL);
  } else {
    /* 	parent process	 */
    printf("This is Parent Process!\n");
    /* parent will wait for the child to complete*/
    wait(NULL);
    printf("Child Complete!\n");
  }
  return 0;
}

int main() {
  PrintMenuOS();
  int (*FunP)() = PrintMenuOS;
  FunP();
  SetPrompt("MenuOS>>");
  MenuConfig("version", "MenuOS V1.0(Based on Linux 3.18.6)", NULL);
  MenuConfig("quit", "Quit from MenuOS", Quit);
  MenuConfig("time", "Show System Time", Time);
  MenuConfig("time-asm", "Show System Time(asm)", TimeAsm);
  MenuConfig("fork", "Fork a new process", Fork);
  MenuConfig("exec", "Execute a program", Exec);
  FunP = ExecuteMenu;
  FunP();
  ExecuteMenu();
  return 0;
}
