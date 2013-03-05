
#include "NamedPipes.h"
#ifdef _WIN32
    #include <windows.h>
    #include <stdio.h>

    #define MAX_PIPES 8

    struct Pipe
    {
        HANDLE handle;
        int accessType;
        char name[100];
    };

    static Pipe pipes[MAX_PIPES];
    static int pipe_index;
    
	int pmake(const char *path, int permissions)
	{
        return 0;
	}

    int pclose(int Descriptor)
    {
        CloseHandle(pipes[Descriptor].handle);
        return 0;
    }

    static int CreateClient(const char* pipeName, const char *path, int flags)
    {
        //printf("Create client waitNamedPipe %s \n",pipeName);
        if (WaitNamedPipe(pipeName, NMPWAIT_WAIT_FOREVER) == 0)
        {
            //printf("WaitNamedPipe failed. error=%lu\n", GetLastError());
            return -1;
        }

        //printf("Create client file %s \n",pipeName);
        int access;

        if (flags == (O_RDONLY | O_WRONLY)) access = GENERIC_WRITE | GENERIC_READ;
        else if (flags & O_WRONLY) access = GENERIC_WRITE;
        else access = GENERIC_READ;
        pipes[pipe_index].handle = CreateFile(pipeName,
                                                access,
                                                0,
                                                NULL,
                                                OPEN_EXISTING,
                                                FILE_ATTRIBUTE_NORMAL,
                                                NULL);

        //printf("Create client file %s done \n",pipeName);
        if (pipes[pipe_index].handle == INVALID_HANDLE_VALUE)
        {
            //printf("CreateFile failed with error %lu\n", GetLastError());
            return -1;
        }

        //else:
        strcpy(pipes[pipe_index].name, path);//no name
        pipes[pipe_index].accessType = flags;
        //printf("%s is linked to index %d handle %lu \n", pipeName, pipe_index, pipes[pipe_index].handle);
        pipe_index++;
        return pipe_index-1;
    }

    static int CreateServer(const char* pipeName, const char *path, int flags)
    {
        //printf("Create server named pipe %s ... \n",pipeName);
        int access;

        if (flags == (O_RDONLY | O_WRONLY)) access = PIPE_ACCESS_DUPLEX | WRITE_DAC;
        else if (flags & O_RDONLY) access = PIPE_ACCESS_INBOUND | WRITE_DAC;
        else access = PIPE_ACCESS_OUTBOUND | WRITE_DAC;

        pipes[pipe_index].handle = CreateNamedPipe(pipeName, 	// Name
                                                    access, // OpenMode
                                                    PIPE_TYPE_BYTE | PIPE_NOWAIT, // PipeMode
                                                    1, // MaxInstances
                                                    1024*1024, // OutBufferSize
                                                    1024*1024, // InBuffersize
                                                    200, // TimeOut
                                                    NULL); // Security
        //printf("Create server named pipe %s done \n",pipeName);
        if (pipes[pipe_index].handle == INVALID_HANDLE_VALUE)
        {
            //printf("Could not create the pipe \n");
            return -1;
        }
        //else
        strcpy(pipes[pipe_index].name, path);
        pipes[pipe_index].accessType = flags;

        ConnectNamedPipe(pipes[pipe_index].handle, NULL);
        //printf("%s is linked to index %d handle %lu \n", pipeName, pipe_index, (long long int)(pipes[pipe_index].handle));
        pipe_index++;
        return pipe_index-1;
    }


    int popen(const char *path, int flags)
    {
        if (pipe_index >= MAX_PIPES) return -1;
        char pipeName[100];
        char pipePrefix[] = "\\\\.\\pipe\\";
        strcpy(pipeName, pipePrefix);
        strcat(pipeName, path);

        int Idx = CreateServer(pipeName,path, flags);
        if (Idx == -1)
        {
            //printf("Server creation failed \n");
            Idx = CreateClient(pipeName,path, flags);
        }

        //if (Idx ==-1) printf("Client creation failed \n");
        return Idx;
    }

    int pwrite(int Descriptor, void *Buffsrc, unsigned int BytesToWrite)
    {
        long unsigned int dwWritten;
        if (Buffsrc == NULL)
        {
            //printf("about to flush handle %lu\n", pipes[Descriptor].handle);
            //fflush(stdout);
            FlushFileBuffers(pipes[Descriptor].handle);
            return 0;
        }

        if (!WriteFile(pipes[Descriptor].handle, Buffsrc, BytesToWrite, &dwWritten, NULL))
		{
			//printf("WriteFile failed to pipe handle %d (error = %lu)\n", Descriptor, GetLastError());
			return -1;
		}

		//printf("Write of %d bytes in handle %d\n", BytesToWrite, pipes[Descriptor].handle);
		//fflush(stdout);
		return dwWritten;
    }

    int pread(int Descriptor, void* Buffdest, unsigned int BytesToRead)
    {
        long unsigned int dwBytesRead = 0;
        while (dwBytesRead < BytesToRead)
		{
		    ReadFile(pipes[Descriptor].handle, Buffdest, BytesToRead, &dwBytesRead, NULL);
      			//printf("ReadFile failed -- probably EOF\n");
      			//while(1);
      			//return -1;
		}
		//printf("reading %d from handle %d\n",dwBytesRead, pipes[Descriptor].handle);
		return dwBytesRead;
    }

    int pAvailable(int Descriptor)
    {
        DWORD bytesAvail = 0;
        BOOL isOK = PeekNamedPipe(pipes[Descriptor].handle, NULL, 0, NULL, &bytesAvail, NULL);
        if(!isOK)
        {
           // Check GetLastError() code
           return -1;
        }
        else
            return bytesAvail;
    }
#endif

