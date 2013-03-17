#ifndef IO_UNIT_H
#define IO_UNIT_H

#include "../util/utils.h"
/*
Transfers occur in the following sequence of events:
1)	Host pushes descriptor into the inbound FIFO, lowest-index first
2)	If transfer is write, host pushes write data into the inbound FIFO
3)	If transfer is read, host pops read data from the outbound FIFO
*/

        #define MODE_READ 0
        #define MODE_WRITE 1

        #define CNXVECTOR_SIZE_IN_BYTES (2 * CNXVECTOR_SIZE_IN_WORDS)
        #define CNXVECTOR_SIZE_IN_WORDS NUMBER_OF_MACHINES
        #define CNXVECTOR_SIZE_IN_DWORDS (CNXVECTOR_SIZE_IN_WORDS / 2)
        #define MAX_CNXVECTORS 2048

        #define DESCRIPTOR_SIZE_IN_DWORDS    3
        #define DESCRIPTOR_SIZE_IN_WORDS     (2*DESCRIPTOR_SIZE_IN_WORDS)
        #define DESCRIPTOR_SIZE_IN_BYTES     (4*DESCRIPTOR_SIZE_IN_DWORDS)
        #define MODE_POS                0
        #define LOCAL_STORE_ADDR_POS    1
        #define CNXVECTOR_COUNT_POS        2

        #define IO_UNIT_MAX_SIZE        (DESCRIPTOR_SIZE_IN_DWORDS + MAX_CNXVECTORS * CNXVECTOR_SIZE_IN_DWORDS)

enum IO_MODE
{
    READ_MODE = 0,
    WRITE_MODE = 1
};

struct IO_UNIT_DESCRIPTOR
{
    UINT32 Mode;
    UINT32 LsAddress;
    UINT32 NumOfCnxvectors;
};

struct IO_UNIT_CORE
{
    IO_UNIT_DESCRIPTOR Descriptor;
    UINT32 Content[IO_UNIT_MAX_SIZE];
};

class io_unit
{
    public:
        io_unit();
        virtual ~io_unit();

        UINT16* getcnxvector(int cnxvectorNumber);
        void setIOParams(int mode, int LsAddress, int NumOfCnxvectors);

        void preWritecnxvectors(UINT16 destAddress, UINT16 *srcAddress, UINT16 numcnxvectors);
        //void preWritecnxvectorsAppend(UINT16 destAddress, UINT16 *srcAddress, UINT16 numcnxvectors);
        void preReadcnxvectors(UINT16 srcAddress,UINT16 numcnxvectors);

        IO_UNIT_CORE* getIO_UNIT_CORE();
        INT32 getSize();

        static int initialize();
        static int deinitialize();

        static int vwrite(void*);
        static int vwriteNonBlocking(void *);
        static int vwriteIsEnded();
        static void vwriteWaitEnd();

        static int vread(void*);

        static int vpipe_read_32;
        static int vpipe_write_32;

    protected:
    private:
        IO_UNIT_CORE Iouc;
        INT32 Size;
};

#endif // IO_SYSTEM_H
