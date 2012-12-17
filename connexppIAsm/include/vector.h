#ifndef VECTOR_H
#define VECTOR_H


class vector
{
    public:
        //vars
        static int dwBatch[1000][1000];
        static int dwInBatchCounter[1000];
        static int dwBatchIndex;

        //methods: static

        static vector addc(vector other_left, vector other_right);
        static void reduce(vector other_left);
        static void setBatchIndex(int BI);

        // methods: non-static
        vector(int, int);
        virtual ~vector();

        // methods: non-static : overloaded operators
        vector operator+(vector);
        vector operator-(vector);
        vector operator=(vector);
        vector operator=(int);

    protected:
    private:
        int mval; /* 0 for R0, .. 15 for R15 */
        int ival; /* intermediate assembly of instruction, usually R[RIGHT] concatenated with R[LEFT] */
        int imval; /* immediate value: will make distinction between 6 and 9 bits-opcode */;
        int opcode;
};

#endif // VECTOR_H
