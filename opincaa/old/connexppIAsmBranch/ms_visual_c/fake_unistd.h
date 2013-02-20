
int open(char *name, int mode);
int open(char *name, int mode, int flags);
void close(int named_pipe);
int write(int pipe, void* elements, int size);
int read(int pipe, void* elements, int size);