#ifndef ATTACHMENT_H 
#define ATTACHMENT_H

char* encodeAttachments(char **fns, int num_file);
int saveAttachment(char *input, int attachmentIdx);

#endif