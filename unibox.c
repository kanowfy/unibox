#include "unibox.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

int readMessage(char *buf, int bufLen);
char* encodeAttachments(char **fns, int num_file);

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
    printf("command: %s\n, arg0: %s\n", command->rawCmd, command->args[0]);
    switch (command->commandType) {
        case MAILFROM:
            char fromBuffer[MAXLINE];
            //TODO: Handle insufficient arguments
            sprintf(fromBuffer, "MAIL FROM: %s\r\n", command->args[0]);
            send(smtpSocket, fromBuffer, strlen(fromBuffer), 0);
            break;
        case RCPTTO:
            char toBuffer[100];
            //TODO: Handle insufficient arguments
            sprintf(toBuffer, "RCPT TO: %s\r\n", command->args[0]);
            send(smtpSocket, toBuffer, strlen(toBuffer), 0);
            break;
        case DATA:
            // if command args > 0, check all files' existence, return error if file does not exists
            for (int i = 0; i < command->argCount; i++){
                if (access(command->args[i], F_OK) != 0){
                    fprintf(stderr, "-ERR invalid attachment \"%s\"\r\n", command->args[i]);
                    return;
                }
            }

            // if ok, tell user to enter 
            fprintf(stdout, "+OK Enter message, end with <CR><LF>.<CR><LF>\r\n");
            
            // store user message
            char messageBuffer[10000000];
            readMessage(messageBuffer, MAXLINE);

            char *formattedAttachment = encodeAttachments(command->args, command->argCount);
            strcat(messageBuffer, formattedAttachment);
            printf("RESULT: %s\n", messageBuffer);
            //send(smtpSocket, messageBuffer, strlen(messageBuffer), 0);
        default:
            fprintf(stderr, "-ERR command not implemented");
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
        default:
            fprintf(stderr, "-ERR command not implemented");
    }
    //send(pop3Socket, command->rawCmd, strlen(command->rawCmd), 0);
}

// sendAllCommand sends a common command to both servers, only QUIT is used for now
void sendAllCommand(Cmd *command, int smtpSocket, int pop3Socket) {
    send(smtpSocket, command->rawCmd, strlen(command->rawCmd), 0);
    send(pop3Socket, command->rawCmd, strlen(command->rawCmd), 0);
}

int readMessage(char *buf, int bufLen) {
    char line[256];
    int offset = 0;
    printf("End data with <CR><LF>.<CR><LF>\n");
    while (1) {
        if (fgets(line, 256, stdin) == NULL) {
            perror("error reading line input");
            exit(EXIT_FAILURE);
        }
        if (strcmp(line, ".\n") == 0) {
            return offset;
            break;
        }
        printf("Entered line: %s\n", line);
        if (offset + strlen(line) + 1 >= bufLen) {
            printf("too long: %d, %ld", offset, strlen(line));
            exit(EXIT_FAILURE);
        }
        strcpy(buf + offset, line);
        offset += strlen(line);
    }
}

char* encodeAttachments(char **fns, int num_file) {
    const char* delimiter = "----------DELIMTER----------\n";
    BIO *mem_bio = BIO_new(BIO_s_mem());
    BIO_write(mem_bio, delimiter, strlen(delimiter));
    BIO *base64_bio = BIO_new(BIO_f_base64());
    BIO_push(base64_bio, mem_bio);

    for (int i = 0; i < num_file; i++) {
        char header[1024];
        sprintf(header, "File name: %s\n", fns[i]);
        BIO_write(mem_bio, header, strlen(header));

        FILE *fp = fopen(fns[i], "rb");
        if (fp != NULL) {
            char buf[4096];
            int bytes_read;
            while ((bytes_read = fread(buf, 1, sizeof(buf), fp)) > 0) {
                BIO_write(base64_bio, buf, bytes_read);
            }
            fclose(fp);
        }
        BIO_flush(base64_bio);
        BIO_write(mem_bio, delimiter, strlen(delimiter));
    }

    BUF_MEM *mem_ptr;
    BIO_get_mem_ptr(mem_bio, &mem_ptr);

    char *encoded_data = (char *)malloc(mem_ptr->length + 1);
    memcpy(encoded_data, mem_ptr->data, mem_ptr->length);
    encoded_data[mem_ptr->length] = '\0'; // Null-terminate the string

    BIO_free_all(base64_bio);

    return encoded_data;
}
