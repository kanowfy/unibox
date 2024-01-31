#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <stdlib.h>
#include <string.h>

const char *delimiter = "----------DELIMTER----------\n";

char *encodeAttachments(char **fns, int num_file) {
    BIO *mem_bio = BIO_new(BIO_s_mem());
    BIO_write(mem_bio, delimiter, strlen(delimiter));
    BIO *base64_bio = BIO_new(BIO_f_base64());
    BIO_push(base64_bio, mem_bio);

    for (int i = 0; i < num_file; i++) {
        char header[1024];
        char *filename = strrchr(fns[i], '/');
        if (filename == NULL) {
            filename = fns[i];
        } else {
            filename += 1;
        }
        sprintf(header, "Attachment name: %s\n", filename);
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

void decodeAndSave(char *filename, char *encoded) {
    BIO *bio = BIO_new_mem_buf(encoded, -1);

    BIO *b64 = BIO_new(BIO_f_base64());
    if (strlen(encoded) <= 64) {
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    }

    BIO_push(b64, bio);

    FILE* out = fopen(filename, "wb");
    
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = BIO_read(b64, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, bytes_read, out);
    }

    BIO_free_all(bio);
    fclose(out);
}

int processAttachment(char *attachmentBlock, int idx, const char *delim) {
    char *fileNameStart = strstr(attachmentBlock, "Attachment name: ");

    if (fileNameStart != NULL) {
        fileNameStart += strlen("Attachment name: ");
        char *fileNameEnd = strstr(fileNameStart, "\r\n");

        if (fileNameEnd != NULL) {
            size_t fileNameLength = fileNameEnd - fileNameStart;

            char fileName[fileNameLength + 1];
            strncpy(fileName, fileNameStart, fileNameLength);
            fileName[fileNameLength] = '\0'; // Null-terminate the file name

            char *contentStart = fileNameEnd + 2; // 2 for \r\n
            char *nextDelim = strstr(contentStart, delim);
            char *contentEnd;
            if (nextDelim != NULL) {
                contentEnd = nextDelim - 2;
            } else {
                return -1;
            } // -2 to roll back 2 position

            if (contentEnd != NULL) {
                size_t contentLength = contentEnd - contentStart;

                char *content = malloc(contentLength + 1);
                strncpy(content, contentStart, contentLength);
                content[contentLength] = '\0';

                decodeAndSave(fileName, content);

                free(content);

                return 0;
            }
        }
    }

    return -1;
}

int saveAttachment(char *input, int pos) {
    const char *delim = "----------DELIMTER----------\r\n";
    int idx = 1;
    char *start = input;
    char *end;

    while ((end = strstr(start, delim)) != NULL) {
        start = end + strlen(delim);
        if (idx == pos) {
            return processAttachment(start, idx, delim);
        }
        idx++;
    }

    return -1;
}
