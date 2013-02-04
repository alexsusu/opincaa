
int open(char *name, int mode);
void close(int named_pipe);
int write(int pipe, void* elements, int size);
int read(int pipe, void* elements, int size);