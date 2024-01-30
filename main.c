#include "unibox.h"

void printCommand(Cmd *command) {
    printf("Raw command: %s\n", command->rawCmd);
    printf("Command: %d\n", command->commandType);
    printf("Protocol: %d\n", command->protocol);
    for (int i = 0; i < command->argCount; i++) {
        printf("arg #%d: %s\n", i, command->args[i]);
    }
}

const char *usage = "\n\n"
                    "██╗   ██╗███╗   ██╗██╗██████╗  ██████╗ ██╗  ██╗\n"
                    "██║   ██║████╗  ██║██║██╔══██╗██╔═══██╗╚██╗██╔╝\n"
                    "██║   ██║██╔██╗ ██║██║██████╔╝██║   ██║ ╚███╔╝\n"
                    "██║   ██║██║╚██╗██║██║██╔══██╗██║   ██║ ██╔██╗\n"
                    "╚██████╔╝██║ ╚████║██║██████╔╝╚██████╔╝██╔╝ ██╗\n"
                    " ╚═════╝ ╚═╝  ╚═══╝╚═╝╚═════╝  ╚═════╝ ╚═╝  ╚═╝\n"
                    "Unibox - A 2-in-1 SMTP and POP3 client\n\n"
                    "Usage: unibox <smtp address> <pop3 address>\n";

int main(int argc, char **argv) {
    if (argc <= 2) {
        printf("%s", usage);
        exit(EXIT_FAILURE);
    }

    // struct timeval tv;
    int smtpSocket, pop3Socket, maxSocket;
    struct sockaddr_in smtpServerAddr, pop3ServerAddr;
    fd_set fdSet;
    char smtpBuffer[MAXLINE], pop3Buffer[MAXLINE], clientBuffer[MAXLINE];
    Cmd *command;

    smtpSocket = socket(AF_INET, SOCK_STREAM, 0);
    pop3Socket = socket(AF_INET, SOCK_STREAM, 0);

    if (smtpSocket < 0 || pop3Socket < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    memset(&smtpServerAddr, 0, sizeof(smtpServerAddr));
    smtpServerAddr.sin_family = AF_INET;
    smtpServerAddr.sin_addr.s_addr = inet_addr(argv[1]);
    smtpServerAddr.sin_port = htons(SMTP_PORT);

    memset(&pop3ServerAddr, 0, sizeof(pop3ServerAddr));
    pop3ServerAddr.sin_family = AF_INET;
    pop3ServerAddr.sin_addr.s_addr = inet_addr(argv[2]);
    pop3ServerAddr.sin_port = htons(POP3_PORT);

    if (connect(smtpSocket, (struct sockaddr *)&smtpServerAddr,
                sizeof(smtpServerAddr)) ||
        connect(pop3Socket, (struct sockaddr *)&pop3ServerAddr,
                sizeof(pop3ServerAddr))) {
        perror("Failed to connect to server");
        exit(EXIT_FAILURE);
    }

    // tv.tv_sec = 10;
    // tv.tv_usec = 500000;

    while (1) {
        // initialize file descriptor set;
        FD_ZERO(&fdSet);
        FD_SET(STDIN_FILENO, &fdSet);
        FD_SET(smtpSocket, &fdSet);
        FD_SET(pop3Socket, &fdSet);
        maxSocket = pop3Socket + 1;

        int rv = select(maxSocket, &fdSet, NULL, NULL, NULL);
        if (rv < 0) {
            perror("Failed to wait on file descriptor");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(STDIN_FILENO, &fdSet)) {
            if (fgets(clientBuffer, MAXLINE, stdin) != NULL) {
                // printf("%s", clientBuffer);
                command = parseCommand(clientBuffer);
                if (command->protocol == SMTP)
                    sendSMTPCommand(command, smtpSocket);
                else if (command->protocol == POP3)
                    sendPOP3Command(command, pop3Socket);
                else if (command->protocol == BOTH)
                    sendAllCommand(command, smtpSocket, pop3Socket);
                else
                    printf("-ERR unknown command\n");
                free(command);
            }
        }

        if (FD_ISSET(smtpSocket, &fdSet)) {
            recv(smtpSocket, smtpBuffer, MAXLINE, 0);
            // parse response
            printf("%s", smtpBuffer);
            memset(smtpBuffer, 0, MAXLINE);
        }

        if (FD_ISSET(pop3Socket, &fdSet)) {
            recv(pop3Socket, pop3Buffer, MAXLINE, 0);
            // parse response
            printf("%s", pop3Buffer);
            memset(pop3Buffer, 0, MAXLINE);
        }
    }
}
