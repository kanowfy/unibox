#ifndef UNIBOX_H
#define UNIBOX_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "attachment.h"

#define MAXLINE 1024
#define SMTP_PORT 25
#define POP3_PORT 110
#define MAX_ARG_COUNT 10

// enum Command defines possible command type
typedef enum {
    USER,
    PASS,
    LIST,
    RETR,
    MAILFROM,
    RCPTTO,
    DATA,
    GETATTACH,
    QUIT,
} Command;

// enum Proto defines possible protocol of a command
typedef enum { SMTP, POP3, BOTH, UNKNOWN } Proto;

// Cmd represents a user command
typedef struct {
    Command commandType;
    Proto protocol;
    int argCount;
    char rawCmd[MAXLINE]; // for commands that can be sent straight away
    char *args[MAX_ARG_COUNT];

} Cmd;
// Response represents a response from a server
typedef struct {
    bool success;
    char *message;
    char *body;
} Response;

Cmd *parseCommand(char *buffer);
void sendSMTPCommand(Cmd *command, int smtpSocket);
void sendPOP3Command(Cmd *command, int pop3Socket);
void sendAllCommand(Cmd *command, int smtpSocket, int pop3Socket);

#endif
