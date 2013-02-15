
#include "NamedPipes.h"
#ifdef _WIN32
    #include <windows.h>
    #define MAX_PIPES 4
    struct Pipe
    {
        HANDLE handle;
        int accessType;
        char name[100];
    };

    Pipe pipes[MAX_PIPES];
    static int pipe_index;
    #include <windows.h>
    #include <stdio.h>

	int pmake(const char *path, int permissions)
	{
        return 0;
	}

    int pclose(int Descriptor)
    {
        CloseHandle(pipes[Descriptor].handle);
        return 0;
    }

    int popen(const char *path, int flags)
    {
        if (pipe_index >= MAX_PIPES) return -1;
        char pipeName[100];
        char pipePrefix[] = "\\\\.\\pipe\\";
        strcpy(pipeName, pipePrefix);
        strcat(pipeName, path);

        for (int i=0; i < pipe_index; i++)
            if (strcmp(pipes[i].name, path)==0)
                if (flags & pipes[i].accessType)
                return -1; // already exists

        if (flags & WRONLY)
        {

            if (WaitNamedPipe(pipeName, NMPWAIT_WAIT_FOREVER) == 0)
            {
                printf("WaitNamedPipe failed. error=%d\n", GetLastError());
                return -1;
            }

            pipes[pipe_index].handle = CreateFile(pipeName,
                                                    GENERIC_WRITE,
                                                    0,
                                                    NULL, OPEN_EXISTING,
                                                    FILE_ATTRIBUTE_NORMAL,
                                                    NULL);
            if (pipes[pipe_index].handle == INVALID_HANDLE_VALUE)
            {
                printf("CreateFile failed with error %d\n", GetLastError());
                return -1;
            }

            //else:
            strcpy(pipes[pipe_index].name, path);//no name
            pipes[pipe_index].accessType = flags;
            pipe_index++;
            return pipe_index-1;
        }
        if (flags & RDONLY)
        {
            pipes[pipe_index].handle = CreateNamedPipe(pipeName, 	// Name
                                                        PIPE_ACCESS_DUPLEX | WRITE_DAC, // OpenMode
                                                        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // PipeMode
                                                        2, // MaxInstances
                                                        1024*1024, // OutBufferSize
                                                        1024*1024, // InBuffersize
                                                        2000, // TimeOut
                                                        NULL); // Security

            if (pipes[pipe_index].handle == INVALID_HANDLE_VALUE)
            {
                printf("Could not create the pipe \n");
                exit(1);
            }
            //else
            strcpy(pipes[pipe_index].name, path);
            pipes[pipe_index].accessType = flags;

            ConnectNamedPipe(pipes[pipe_index].handle, NULL);
            pipe_index++;
            return pipe_index-1;
        }
    }

    int pwrite(int Descriptor, void *Buffsrc, unsigned int BytesToWrite)
    {
        long unsigned int dwWritten;
        if (Buffsrc == NULL)
        {
            FlushFileBuffers(pipes[Descriptor].handle);
            return 0;
        }

        if (!WriteFile(pipes[Descriptor].handle, Buffsrc, BytesToWrite, &dwWritten, NULL))
		{
			printf("WriteFile failed\n");
			return -1;
		}
		return dwWritten;
    }

    int pread(int Descriptor, void* Buffdest, unsigned int BytesToRead)
    {
        long unsigned int dwBytesRead;
        if (!ReadFile(pipes[Descriptor].handle, Buffdest, BytesToRead, &dwBytesRead, NULL))
		{
      			printf("ReadFile failed -- probably EOF\n");
      			return -1;
		}
		return dwBytesRead;
    }
#endif

