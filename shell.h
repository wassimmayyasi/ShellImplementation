#ifndef SHELL_H
#define SHELL_H

#include "parser/ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>

#define STDIN 0
#define STDOUT 1
#define PIPE_RD 0
#define PIPE_WR 1

struct tree_node;

extern char *prompt;

void initialize(void);

void run_command(struct tree_node *n);

void handleCommand(node_t *n);
void handleSignal();
void handleSequences(node_t *n);
void handlePipes(node_t *n);
void handleRedirects(node_t *n);
void handleDetach(node_t *n);
void handleSubShell(node_t *n);

#endif
