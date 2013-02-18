/*
 * File:   Kernel.cpp
 *
 * This is the  class containing a kernel
 * (a vector of Instructions) for executing on the Connex Array
 *
 */

#include "Kernel.h"
#include "NamedPipes.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>

/*
* Constructor for creating a new Kernel
*
* @param name the name of the new kernel\
*
* @throws string if the name is invalid (NULL or empty)
*/
Kernel::Kernel(string name)
{
    if(name.length() == 0)
    {
        throw new string("Invalid kernel name");
    }
    this->name = name;
}

/*
* Destructor for the Kernel class
*
*/
Kernel::~Kernel()
{

}

/*
* Appends an existing instruction to the kernel
*
* @param instruction the instruction to add
*/
void Kernel::append(Instruction instruction)
{
    instructions.push_back(instruction.assemble());
}

/*
* Writes the kernel to a memory location
*
* @param buffer the memory location to write the kernel to
*/
void Kernel::writeTo(void * buffer)
{
    memcpy(buffer, instructions.data(), instructions.size() * sizeof(unsigned));
}

/*
* Writes the kernel to a file descriptor
* @param fileDescriptor the file descriptor to write the kernel to
*/
void Kernel::writeTo(int fileDescriptor)
{
    pwrite(fileDescriptor, instructions.data(), instructions.size() * sizeof(unsigned));

    /* Flush the descriptor */
    pwrite(fileDescriptor, NULL, 0);
}

/*
 * Returns the number of instructions in this kernel
 *
 * @return the number of instructions in this kernel
 */
unsigned Kernel::size()
{
    return instructions.size();
}

/*
 * Returns the name of this kernel
 *
 * @return the name of this kernel
 */
string Kernel::getName()
{
    return name;
}

/*
 * Returns a string representing the disassembled kernel, one
 * instruction per line.
 *
 * @return the disassembled kernel
 */
string Kernel::disassemble()
{
    string kernel;
    for(vector<unsigned>::iterator element = instructions.begin(); element != instructions.end(); element++)
    {
        kernel += Instruction(*element).disassemble();
    }

    return kernel;
}
