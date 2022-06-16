#include "shell.h"

void initialize(void) {
    if (prompt)
        prompt = "vush$ ";
}

void run_command(node_t *node) {
    switch (node->type) {
        case NODE_COMMAND:
            handleCommand(node);
            break;
        case NODE_PIPE:
            handlePipes(node);
            break;
        case NODE_REDIRECT:
            handleRedirects(node);
            break;
        case NODE_SUBSHELL:
            handleSubShell(node);
            break;
        case NODE_SEQUENCE:
            handleSequences(node);
            break;
        case NODE_DETACH:
            handleDetach(node);
            break;
    }
    if (prompt)
        prompt = "vush$ ";
}

void handleCommand(node_t *n) {
    signal(SIGINT, handleSignal);
    signal(SIGTSTP, handleSignal);

    if(!strcmp(n->command.program, "exit")) {
        if(!strcmp(n->command.argv[1], "1")) {
            exit(1);
        }
        else{
            exit(42);
        }
    }      
    else if(!strcmp(n->command.program, "cd")) {
        if(chdir(n->command.argv[1])) 
            perror("No such file or directory");
    }
    else if(!strcmp(n->command.program, "set")) {
        char delimiter[] = "=";
        char* var = strtok(n->command.argv[1], delimiter);
        setenv(var, strtok(NULL, delimiter), 1);
    }
    else if(!strcmp(n->command.program, "unset")) {
        unsetenv(n->command.argv[1]);
    }
    else{
        int status;
        pid_t pid = fork();
        if(pid == 0) {
            if(execvp(n->command.program, n->command.argv) < 0){
                perror("Error");
            }
        }
        else 
            wait(&status);
    }
}

void handleSignal() {}

void handleSequences(node_t *n) {
    switch(n->sequence.first->type) {
        case NODE_COMMAND:
            handleCommand(n->sequence.first);
            break;
        case NODE_REDIRECT:
            handleRedirects(n->sequence.first);
            break;
        case NODE_DETACH:
            handleDetach(n->sequence.first);
            break;
        case NODE_SUBSHELL:
            handleSubShell(n->sequence.first);
            break;
        case NODE_PIPE:
            handlePipes(n->sequence.first);
            break;
        case NODE_SEQUENCE:
            handleSequences(n->sequence.first);
    }
    switch(n->sequence.second->type) {
        case NODE_COMMAND:
            handleCommand(n->sequence.second);
            break;
        case NODE_REDIRECT:
            handleRedirects(n->sequence.second);
            break;
        case NODE_SEQUENCE:
            handleSequences(n->sequence.second);
            break;
        case NODE_DETACH:
            handleDetach(n->sequence.second);
            break;
        case NODE_SUBSHELL:
            handleSubShell(n->sequence.second);
            break;
        case NODE_PIPE:
            handlePipes(n->sequence.second);
    }
}

void handlePipes(node_t *n) {
    node_t *firstCommand = n->pipe.parts[0];
    node_t *secondCommand = n->pipe.parts[1];

    pid_t pid1, pid2;
    int pipefd[2];
    pipe(pipefd);

    pid1 = fork();
    if(pid1 == 0) {
        close(pipefd[PIPE_RD]);
        dup2(pipefd[PIPE_WR], STDOUT);
        handleCommand(firstCommand);
        exit(EXIT_SUCCESS);
    }
    pid2 = fork();
    if(pid2 == 0) {
        close(pipefd[PIPE_WR]);
        dup2(pipefd[PIPE_RD], STDIN);
        handleCommand(secondCommand);
        exit(EXIT_SUCCESS);
    }
    close(pipefd[PIPE_RD]);
    close(pipefd[PIPE_WR]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void handleRedirects(node_t *n) {
    int pid = fork();
    if(pid == 0){
        switch(n->redirect.mode) {
            case REDIRECT_DUP:
                close(STDOUT);
                dup2(n->redirect.fd, n->redirect.fd2);
                close(n->redirect.fd);
                close(n->redirect.fd2);
                break;
            case REDIRECT_INPUT: {
                int fdIn = open(n->redirect.target, O_RDONLY);
                dup2(fdIn, n->redirect.fd);
                close(fdIn);
                break;
            }
            case REDIRECT_OUTPUT: {
                int fdOut = creat(n->redirect.target , O_WRONLY) ;
                dup2(fdOut, n->redirect.fd);
                close(fdOut);
                break;
            }
            case REDIRECT_APPEND: {
                int fdOut = open(n->redirect.target , O_WRONLY | O_APPEND) ;
                dup2(fdOut, n->redirect.fd);
                close(fdOut);
                break;
            }  
        }
        execvp(n->redirect.child->command.program, n->redirect.child->command.argv);
    }
    else {
        waitpid(pid, 0, 0);
    }
}

void handleDetach(node_t *n) {
    pid_t pid;
    switch(n->detach.child->type) {
        case NODE_COMMAND: {
            pid = fork();
            if(pid == 0) {
                handleCommand(n->detach.child);
                exit(EXIT_SUCCESS);
            }
        }
            break;
        case NODE_SEQUENCE:
            pid = fork();
            if(pid == 0) {
                handleSequences(n->detach.child);
                exit(EXIT_SUCCESS);
            }
            break;
        default:
            break;
    }
}

void handleSubShell(node_t *n) {
    pid_t pid;
    pid = fork();
    if(pid == 0) {
        handleSequences(n->subshell.child);
        exit(EXIT_SUCCESS);
    }
    else {
        waitpid(pid, NULL, 0);
    }
}