/*
 * File:   NamedPipes.h
 *
 * Header file for
 */

#ifndef NAMEDPIPES_H
#define NAMEDPIPES_H

#ifdef _WIN32
    #define O_WRONLY (1 << 0)
    #define O_RDONLY (1 << 1)
    #define O_RDWR   (O_RDONLY | O_WRONLY)

    int pmake(const char *path, int permissions);

    int popen(const char *path, int flags);
    int pclose(int Descriptor);

    int pwrite(int Descriptor, void *Buffsrc, unsigned int BytesToWrite);
    int pread(int Descriptor, void* Buffdest, unsigned int BytesToRead);

    int pAvailable(int Descriptor);

#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <sys/stat.h>

    #define popen open
    #define pwrite write
    #define pread read
    #define pclose close
    #define pmake mkfifo
#endif

#endif // NAMEDPIPES_H
