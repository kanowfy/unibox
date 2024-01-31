#include "unibox.h"

int readMessage(char *buf, int bufLen);

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
        command->args[count++] = strdup(token);
        token = strtok(NULL, " ");
    }

    command->argCount = count;

    free(copied);

    return command;
}

// sendPOP3Command constructs and sends an SMTP command
void sendSMTPCommand(Cmd *command, int smtpSocket) {
    switch (command->commandType) {
        case MAILFROM:
            if (command->argCount == 0) {
                fprintf(stderr, "-ERR Missing sender.");
                return;
            }
            char fromBuffer[100];
            //TODO: Handle insufficient arguments
            sprintf(fromBuffer, "MAIL FROM: %s\r\n", command->args[0]);
            send(smtpSocket, fromBuffer, strlen(fromBuffer), 0);
            break;
        case RCPTTO:
            if (command->argCount == 0) {
                fprintf(stderr, "-ERR Missing receiver.");
                return;
            }
            char toBuffer[100];
            //TODO: Handle insufficient arguments
            sprintf(toBuffer, "RCPT TO: %s\r\n", command->args[0]);
            send(smtpSocket, toBuffer, strlen(toBuffer), 0);
            break;
        case DATA:
            // if command args > 0, check all files' existence, return error if file does not exists
            for (int i = 0; i < command->argCount; i++){
                if (access(command->args[i], F_OK) != 0){
                    fprintf(stderr, "-ERR invalid attachment \"%s\".\r\n", command->args[i]);
                    return;
                }
            }

            // if ok, tell user to enter 
            fprintf(stdout, "+OK Enter message, end with <CR><LF>.<CR><LF>\r\n");
            
            // store user message
            char *messageBuffer = malloc(MAXLINE*MAXLINE*sizeof(char));
            readMessage(messageBuffer, MAXLINE);

            if (command->argCount > 0) {
                char *formattedAttachment = encodeAttachments(command->args, command->argCount);
                strcat(messageBuffer, formattedAttachment);
            }

            char* ends = "\r\n.\r\n";
            strcat(messageBuffer, ends);

            send(smtpSocket, "DATA\r\n", strlen("DATA\r\n"), 0);

            send(smtpSocket, messageBuffer, strlen(messageBuffer), 0);

            free(messageBuffer);
            break;
        default:
            fprintf(stderr, "-ERR command not implemented.\r\n");
    }
}

// sendPOP3Command constructs and sends a POP3 command
void sendPOP3Command(Cmd *command, int pop3Socket) {
    switch (command->commandType) {
        case USER:
        case PASS:
        case LIST:
        case RETR:
            send(pop3Socket, command->rawCmd, strlen(command->rawCmd), 0);
            break;
        case GETATTACH:
            if (command->argCount != 2) {
                fprintf(stderr, "-ERR invalid number of arguments.\r\n");
                return;
            }

            int msgIndex = atoi(command->args[0]);
            if (msgIndex <= 0) {
                fprintf(stderr, "-ERR invalid message number.\r\n");
                return;
            }
            int attachmentIndex = atoi(command->args[1]);
            if (attachmentIndex <= 0) {
                fprintf(stderr, "-ERR invalid attachment index.\r\n");
                return;
            }

            char cmdBuf[100];
            sprintf(cmdBuf, "RETR %d\r\n", msgIndex);
            send(pop3Socket, cmdBuf, strlen(cmdBuf), 0);

            char *recvBuf = NULL;
            int totalReceived = 0;
            int bytesRead = 0;
            
            do {
                recvBuf = realloc(recvBuf, totalReceived + MAXLINE);

                bytesRead = recv(pop3Socket, recvBuf+totalReceived, MAXLINE, 0);

                if (bytesRead <= 0) {
                    break;
                }

                totalReceived += bytesRead;
            } while (bytesRead == MAXLINE);


            if (saveAttachment(recvBuf, attachmentIndex) != 0) {
                fprintf(stderr, "-ERR invalid attachment index\r\n");
            } else {
                fprintf(stdout, "+OK saved\r\n");
            }

            free(recvBuf);
            break;
        default:
            fprintf(stderr, "-ERR command not implemented");
    }
}

// sendAllCommand sends a common command to both servers, only QUIT is used for now
void sendAllCommand(Cmd *command, int smtpSocket, int pop3Socket) {
    send(smtpSocket, command->rawCmd, strlen(command->rawCmd), 0);
    send(pop3Socket, command->rawCmd, strlen(command->rawCmd), 0);
}

int readMessage(char *buf, int bufLen) {
    char line[256];
    int offset = 0;
    while (1) {
        if (fgets(line, 256, stdin) == NULL) {
            perror("error reading line input");
            exit(EXIT_FAILURE);
        }
        if (strcmp(line, ".\n") == 0) {
            return offset;
            break;
        }
        //printf("Entered line: %s\n", line);
        if (offset + strlen(line) + 1 >= bufLen) {
            printf("too long: %d, %ld", offset, strlen(line));
            exit(EXIT_FAILURE);
        }
        strcpy(buf + offset, line);
        offset += strlen(line);
    }
}