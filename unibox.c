#include "unibox.h"

// parseCommand takes user input buffer and construct a new Cmd instance
Cmd *parseCommand(char *buffer) {
    Cmd *command = (Cmd *)malloc(sizeof(Cmd));

    char *copied = strdup(buffer);
    copied[strlen(copied) - 1] = '\0';
    sprintf(command->rawCmd, "%s\r\n", copied);

    char *token = strtok(copied, " ");
    char *clientCmd = token;
    if (strcmp(clientCmd, "USER") == 0) {
        command->commandType = USER;
        command->protocol = POP3;
    } else if (strcmp(clientCmd, "PASS") == 0) {
        command->commandType = PASS;
        command->protocol = POP3;
    } else if (strcmp(clientCmd, "LIST") == 0) {
        command->commandType = LIST;
        command->protocol = POP3;
    } else if (strcmp(clientCmd, "RETR") == 0) {
        command->commandType = RETR;
        command->protocol = POP3;
    } else if (strcmp(clientCmd, "MAILFROM") == 0) {
        command->commandType = MAILFROM;
        command->protocol = SMTP;
    } else if (strcmp(clientCmd, "RCPTTO") == 0) {
        command->commandType = RCPTTO;
        command->protocol = SMTP;
    } else if (strcmp(clientCmd, "GETATTACH") == 0) {
        command->commandType = GETATTACH;
        command->protocol = POP3;
    } else if (strcmp(clientCmd, "DATA") == 0) {
        command->commandType = DATA;
        command->protocol = SMTP;
    } else if (strcmp(clientCmd, "QUIT") == 0) {
        command->commandType = QUIT;
        command->protocol = BOTH;
    } else {
        command->protocol = UNKNOWN;
    }

    int count = 0;
    token = strtok(NULL, " ");
    while (token != NULL && count < MAX_ARG_COUNT) {
        command->args[count++] = token;
        token = strtok(NULL, " ");
    }

    command->argCount = count;

    free(copied);

    return command;
}

void sendSMTPCommand(Cmd *command, int smtpSocket) {
    send(smtpSocket, command->rawCmd, strlen(command->rawCmd), 0);
}
void sendPOP3Command(Cmd *command, int pop3Socket) {
    send(pop3Socket, command->rawCmd, strlen(command->rawCmd), 0);
}
void sendAllCommand(Cmd *command, int smtpSocket, int pop3Socket) {
    send(smtpSocket, command->rawCmd, strlen(command->rawCmd), 0);
    send(pop3Socket, command->rawCmd, strlen(command->rawCmd), 0);
}
