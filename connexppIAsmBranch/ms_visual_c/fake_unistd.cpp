

int open(char *name, int mode) 
{
	mode = mode;
	mode = name[0];
	return 0;
}
void close(int named_pipe) {}

int write(int pipe, void *elements, int size){ return -1;}
int read(int pipe, void *elements, int size){ return -1;}