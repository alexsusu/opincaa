//------------------------------------------------------------------------------
//
//		Interleaved Multithreading Processor
//			- Opcodes -
//
//------------------------------------------------------------------------------
//     $Id: Opcodes.java,java 1.1 2009/05/14 12:35:50 rhobincu Exp $
//     $Author: rhobincu $
//     $Revision: 1.0 $
//     $Locker:  $
//     $State: Exp $
//     $Date: 2009/05/14 12:35:50 $
//------------------------------------------------------------------------------
package ro.pub.arh.beam.hardware;

//------------------------------------------------------------------------------
public class Opcodes{

    public static final short move     = (short)0x00;
    public static final short nop      = (short)0x00; //alias
    public static final short red      = (short)0x00; //alias

    public static final short ldix     = (short)0x01;
    public static final short extget   = (short)0x02;
    public static final short ldext    = (short)0x03;
    public static final short loadPc   = (short)0x04;
    public static final short clri     = (short)0x05;
    public static final short shget    = (short)0x06;
    public static final short load     = (short)0x07;

    public static final short add      = (short)0x08;
    public static final short sub      = (short)0x09;
//    public static final short cradd    = (short)0x0A;
//    public static final short crsub    = (short)0x0B;
    public static final short inc      = (short)0x0C;
    public static final short dec      = (short)0x0D;
//    public static final short crinc    = (short)0x0E;
//    public static final short crdec    = (short)0x0F;


    public static final short shl      = (short)0x10;
    public static final short shr      = (short)0x11;
    public static final short ashr     = (short)0x12;
    public static final short rot      = (short)0x13;
    public static final short ishl     = (short)0x14;
    public static final short ishr     = (short)0x15;
    public static final short iashr    = (short)0x16;
    public static final short irot     = (short)0x17;


    public static final short ixadd    = (short)0x18;
    public static final short ixsub    = (short)0x19;
    public static final short ixcradd  = (short)0x1A;
    public static final short ixcrsub  = (short)0x1B;
    public static final short bwnot    = (short)0x1C;
    public static final short bwand    = (short)0x1D;
    public static final short bwxor    = (short)0x1E;
    public static final short bwor     = (short)0x1F;

    public static final short abs      = (short)0x20;
    public static final short div      = (short)0x21;
    //public static final short modulo = (short)0x22;
    public static final short mult4    = (short)0x23;
    public static final short mult0    = (short)0x24;
    public static final short mult1    = (short)0x25;
    public static final short mult2    = (short)0x26;
    public static final short mult3    = (short)0x27;

    public static final short write    = (short)0x28;
    public static final short read     = (short)0x29;
    public static final short cas   = (short)0x2A;
    public static final short loadm    = (short)0x2B;
    public static final short byterd   = (short)0x2C;
    public static final short bytewr   = (short)0x2D;
    public static final short srtrd    = (short)0x2E;
    public static final short srtwr    = (short)0x2F;

    //public static final short unused   = (short)0x30;
    //public static final short unused   = (short)0x31;
    //public static final short unused    = (short)0x32;
    //public static final short unused    = (short)0x33;

    //public static final short unused  = (short)0x34;
    //public static final short unused  = (short)0x35;
    //public static final short unused   = (short)0x36;
    //public static final short unused  = (short)0x37;
    public static final short irqret  = (short)0x38;
    public static final short shwait   = (short)0x39;
    public static final short endSub   = (short)0x3A;
    public static final short vwait    = (short)0x3B;

    public static final short remTh     = (short)0x3C;
    public static final short swi     = (short)0x3D;
    public static final short rjmp      = (short)0x3E;
    public static final short ajmp      = (short)0x3F;

    public static final short srcreg   = (short)0x40;
    public static final short swiret    = (short)0x41;
    //public static final short srcscal   = (short)0x42;
    //public static final short land     = (short)0x43;
    public static final short lnot    = (short)0x44;
    public static final short land     = (short)0x45;
    //public static final short unused    = (short)0x46;
    public static final short lor    = (short)0x47;

    //public static final short unused      = (short)0x48;
    //public static final short unused      = (short)0x49;
    //public static final short unused    = (short)0x4A;
    //public static final short unused    = (short)0x4B;
    //public static final short unused    = (short)0x4C;
    //public static final short unused    = (short)0x4D;
    //public static final short unused    = (short)0x4E;
    //public static final short unused     = (short)0x4F;

//    public static final short radd      = (short)0x50;
//    public static final short rsub      = (short)0x51;
//    public static final short rcradd    = (short)0x52;
//    public static final short rcrsub    = (short)0x53;
//    public static final short rbwnot    = (short)0x54;
//    public static final short rbwand    = (short)0x55;
//    public static final short rbwxor    = (short)0x56;
//    public static final short rbwor     = (short)0x57;
//
//    public static final short madd      = (short)0x58;
//    public static final short msub      = (short)0x59;
//    public static final short mcradd    = (short)0x5A;
//    public static final short mcrsub    = (short)0x5B;
//    public static final short mbwnot    = (short)0x5C;
//    public static final short mbwand    = (short)0x5D;
//    public static final short mbwxor    = (short)0x5E;
//    public static final short mbwor     = (short)0x5F;

    public static final short insert    = (short)0x60;
    //public static final short unused    = (short)0x61;
    public static final short delete    = (short)0x62;
    //public static final short addsh    = (short)0x63;

//    public static final short lrot      = (short)0x64;
//    public static final short rrot      = (short)0x65;
//    public static final short memlrot   = (short)0x66;
//    public static final short memrrot   = (short)0x67;

    public static final short lrot      = (short)0x68;
    public static final short rrot      = (short)0x69;
    public static final short lshz      = (short)0x6A;
    public static final short rshz      = (short)0x6B;
    public static final short lshr      = (short)0x6C;
    public static final short rshr      = (short)0x6D;
//    public static final short memlshr   = (short)0x6E;
//    public static final short memrshr   = (short)0x6F;

    public static final short cry        = (short)0x70;
    public static final short brw        = (short)0x71;
    public static final short eq        = (short)0x72;
    //public static final short unused       = (short)0x73;
    public static final short ult       = (short)0x74;
    public static final short ugt       = (short)0x75;
    public static final short lt        = (short)0x76;
    public static final short gt        = (short)0x77;
    //public static final short unused    = (short)0x78;
    //public static final short unused      = (short)0x79;
    //public static final short unused      = (short)0x7A;

    //this is the value instruction; considering them on 9 bits as well, so they don't overlap (last 3 bits always 0)
    public static final short vload      = (short)0x100;
    public static final short vlload     = (short)0x108;
    public static final short insval    = (short)0x110;
    public static final short srcval    = (short)0x118;
    public static final short csrcval    = (short)0x120;
    public static final short mrc    = (short)0x128;
    public static final short mcr    = (short)0x130;
    //public static final short mcr     = (short)0x138;

    public static final short iadd     = (short)0x140;
    //public static final short unused     = (short)0x148;
    public static final short rirhi   = (short)0x150;
    public static final short rirlo   = (short)0x158;
//    public static final short unused   = (short)0x160;
    public static final short iand   = (short)0x168;
    public static final short ixor   = (short)0x170;
    public static final short ior    = (short)0x178;

    public static final short call      = (short)0x180;
    public static final short jmp       = (short)0x188;
    public static final short jz        = (short)0x190;
    public static final short jnz       = (short)0x198;

    public static final short ird    = (short)0x1A0;
    public static final short iwr    = (short)0x1A8;

    //public static final short spird     = (short)0x1B0;
    //public static final short spiwr     = (short)0x1B8;

//    public static final short lrot   = (short)0x1C0;
//    public static final short rrot   = (short)0x1C8;
//    public static final short lshz   = (short)0x1D0;
//    public static final short rshz   = (short)0x1D8;
//    public static final short lshr   = (short)0x1E0;
//    public static final short rshr   = (short)0x1E8;

    //public static final short lshr   = (short)0x1F0;
    //public static final short rshr   = (short)0x1F8;

    //--------------------------------------------------------------------------
    //boolean insns
    //--------------------------------------------------------------------------
    public static final byte bnop	= 0x00;
    public static final byte rst	= 0x01;
    public static final byte endwhere	= 0x02;
    public static final byte elsew	= 0x03;
    public static final byte src	= 0x04;
    public static final byte csrc	= 0x05;
    public static final byte ins	= 0x06;
    public static final byte loop	= 0x07;
    public static final byte rdback	= 0x08;
    public static final byte rdfwd	= 0x09;
    public static final byte clrfrt	= 0x0A;
    public static final byte restore    = 0x0B;
    

    public static final byte movefl	= 0x10;
    public static final byte savesel	= 0x20;
    //public static final byte cfnzc	= 0x30;
    //public static final byte cfzc 	= 0x40;
    //public static final byte cfnzcp	= 0x50;
    public static final byte wherenz 	= 0x60;
    public static final byte wherez 	= 0x70;
    

    private static String[] booleanOpCodes;
    private static String[] opCodes;
    private static String[] format;

//------------------------------------------------------------------------------
static{
    opCodes = new String[512];

    opCodes[move]="move|nop|red";
    opCodes[ldix]="ldix";
    opCodes[extget]="extget";
    opCodes[ldext]="ldext";
    opCodes[loadPc]="loadPc";
    opCodes[clri]="clri";
    
    
    opCodes[shget]="shget";
    opCodes[load]="load";

    opCodes[add]="add";
    opCodes[sub]="sub";
    opCodes[inc]="inc";
    opCodes[dec]="dec";


    opCodes[shl]="shl";
    opCodes[shr]="shr";
    opCodes[ashr]="ashr";
    opCodes[rot]="rot";
    opCodes[ishl]="ishl";
    opCodes[ishr]="ishr";
    opCodes[iashr]="iashr";
    opCodes[irot]="irot";


    opCodes[ixadd]="ixadd";
    opCodes[ixsub]="ixsub";
    opCodes[ixcradd]="ixcradd";
    opCodes[ixcrsub]="ixcrsub";
    opCodes[bwnot]="bwnot";
    opCodes[bwand]="bwand";
    opCodes[bwxor]="bwxor";
    opCodes[bwor]="bwor";

    opCodes[abs]="abs";
    opCodes[div]="div";

    opCodes[mult4]="mult4";
    opCodes[mult0]="mult0";
    opCodes[mult1]="mult1";
    opCodes[mult2]="mult2";
    opCodes[mult3]="mult3";

    opCodes[write]="write";
    opCodes[read]="read";
    opCodes[cas]="cas";
    opCodes[loadm]="loadm";
    opCodes[byterd]="byterd";
    opCodes[bytewr]="bytewr";
    opCodes[srtrd]="srtrd";
    opCodes[srtwr]="srtwr";

    opCodes[irqret]="irqret";
    opCodes[shwait]="shwait";
    opCodes[endSub]="endSub";
    opCodes[vwait]="vwait";

    opCodes[remTh]="remTh";
    opCodes[swi]="swi";
    opCodes[rjmp]="rjmp";
    opCodes[ajmp]="ajmp";

    opCodes[srcreg]="srcreg";
    opCodes[swiret]="swiret";

    opCodes[land]="land";
    opCodes[lor]="lor";
    opCodes[lnot]="lnot";


    opCodes[insert]="insert";
    opCodes[delete]="delete";


    opCodes[cry]="cry";
    opCodes[brw]="brw";    
    opCodes[eq]="eq";  
    opCodes[ult]="ult";
    opCodes[ugt]="ugt";
    opCodes[lt]="lt";
    opCodes[gt]="gt";

    //this is the value instruction; considering them on 9 bits as well, so they don't overlap (last 3 bits always 0)
    opCodes[vload]="vload";
    opCodes[vlload]="vlload";
    opCodes[insval]="insval";
    opCodes[srcval]="srcval";
    opCodes[csrcval]="csrcval";
    opCodes[mrc]="mrc";
    opCodes[mcr]="mcr";


    opCodes[iadd]="iadd";
    opCodes[rirhi]="rirhi";
    opCodes[rirlo]="rirlo";

    opCodes[iand]="iand";
    opCodes[ixor]="ixor";
    opCodes[ior]="ior";

    opCodes[call]="call";
    opCodes[jmp]="jmp";
    opCodes[jz]="jz";
    opCodes[jnz]="jnz";

    opCodes[ird]="ird";
    opCodes[iwr]="iwr";

    opCodes[lrot]="lrot";
    opCodes[rrot]="rrot";
    opCodes[lshz]="lshz";
    opCodes[rshz]="rshz";
    opCodes[lshr]="lshr";
    opCodes[rshr]="rshr";

    
    //types can be:
    //rID - register ID;
    //val - immediate value, as decimal or hex number;
    //label - value as label
    //opCode - operation (instruction code) code 
    //---
    /* each format is composed of one or more fields
     * A field has the following format:
     * [field_size]field_type->[chunk1_start_from_bit][bit_chunk_1][chunk2_start_from_bit][bit_chunk2]
     * For example, [16]opcode->[23,16] translates as:
     *   - a 16 bit opcode which is split in 1 chunk (so actually not split) of 16 bits, which is placed at bit 23
     * [16]value->[23,8][0,8] translates as:
     *   - a 16 bit numerical value which is split in 2 chunks, first one (LS) having 8 bits, being placed
     * at bit 23 (so it occupies bits [30-23]), and second one (MS) having 8 bits, and placed at bit 0, thus 
     * occupying bits [7-0].
     * 
     */
    format = new String[512];


    format[move]        ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[10,5]|[16]opcode->[23,16]|[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[10,5] [5]value->[5,5]";
    format[ldix]        ="[16]opcode->[23,16] [5]rID->[0,5]";
    format[extget]      ="[16]opcode->[23,16] [5]rID->[0,5]";
    format[ldext]       ="[16]opcode->[23,16] [5]rID->[5,5]";
    format[loadPc]      ="[16]opcode->[23,16] [5]rID->[0,5]";
    format[clri]        ="[16]opcode->[23,16] [5]rID->[5,5]";
    
    format[shget]       ="[16]opcode->[23,16] [5]rID->[0,5]";
    format[load]        ="[16]opcode->[23,16] [5]rID->[0,5]";

    format[add]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[sub]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";

    format[inc]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5]";
    format[dec]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5]";


    format[shl]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[shr]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[ashr]        ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[rot]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[ishl]        ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]value->[10,5]";
    format[ishr]        ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]value->[10,5]";
    format[iashr]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]value->[10,5]";
    format[irot]        ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]value->[10,5]";


    format[ixadd]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[10,5]";
    format[ixsub]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[10,5]";
    format[ixcradd]     ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[10,5]";
    format[ixcrsub]     ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[10,5]";
    format[bwnot]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5]";
    format[bwand]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[bwxor]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[bwor]        ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";

    format[abs]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5]";
    format[div]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[mult4]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[mult0]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[mult1]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[mult2]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[mult3]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";

    format[write]       ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[10,5]";
    format[read]        ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[0,5]";
    format[cas]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[loadm]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[byterd]      ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5]";
    format[bytewr]      ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[10,5]";
    format[srtrd]       ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5]";
    format[srtwr]       ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[10,5]";

    format[irqret]      ="[16]opcode->[23,16] [5]rID->[5,5]";
    format[shwait]      ="[16]opcode->[23,16]";
    format[endSub]      ="[16]opcode->[23,16]";
    format[vwait]       ="[16]opcode->[23,16]";

    format[remTh]       ="[16]opcode->[23,16]";
    format[swi]         ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[10,5]";
    format[rjmp]        ="[16]opcode->[23,16] [5]rID->[5,5]";
    format[ajmp]        ="[16]opcode->[23,16] [5]rID->[5,5]";

    format[srcreg]     ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[10,5]";
    format[swiret]     ="[16]opcode->[23,16] [5]rID->[5,5]";
    
    format[land]        ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[lor]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[lnot]        ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5]";

    format[insert]      ="[16]opcode->[23,16]";
    format[delete]      ="[16]opcode->[23,16]";

    format[lrot]        ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[10,5]";
    format[rrot]        ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[10,5]";
    format[lshz]        ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[10,5]";
    format[rshz]        ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[10,5]";
    format[lshr]        ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[10,5]";
    format[rshr]        ="[16]opcode->[23,16] [5]rID->[5,5] [5]rID->[10,5]";

    format[cry]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[brw]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[eq]          ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[ult]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[ugt]         ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[lt]          ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    format[gt]          ="[16]opcode->[23,16] [5]rID->[0,5] [5]rID->[5,5] [5]rID->[10,5]";
    
    //this is the value instruction; considering them on 9 bits as well, so they don't overlap (last 3 bits always 0)
    format[vload]       ="[13]opcode->[26,13] [5]rID->[0,5] [16]lvalue->[10,16]";
    format[vlload]      ="[13]opcode->[26,13] [5]rID->[0,5] [5]rID->[5,5] [16]lvalue->[10,16]";
    format[insval]      ="[13]opcode->[26,13] [5]rID->[5,5] [16]value->[10,16]";
    format[srcval]      ="[13]opcode->[26,13] [5]rID->[5,5] [16]value->[10,16]";
    format[csrcval]      ="[13]opcode->[26,13] [5]rID->[5,5] [16]value->[10,16]";
    format[mrc]         ="[13]opcode->[26,13] [5]rID->[0,5] [5]rID->[5,5] [16]lvalue->[10,16]";
    format[mcr]         ="[13]opcode->[26,13] [5]rID->[0,5] [16]lvalue->[10,16]";

    format[iadd]        ="[13]opcode->[26,13] [5]rID->[0,5] [5]rID->[5,5] [16]value->[10,16]";
    format[rirhi]       ="[13]opcode->[26,13] [5]rID->[0,5] [16]value->[10,16]";
    format[rirlo]       ="[13]opcode->[26,13] [5]rID->[0,5] [16]value->[10,16]";
    
    format[iand]        ="[13]opcode->[26,13] [5]rID->[0,5] [5]rID->[5,5] [16]value->[10,16]";
    format[ixor]        ="[13]opcode->[26,13] [5]rID->[0,5] [5]rID->[5,5] [16]value->[10,16]";
    format[ior]         ="[13]opcode->[26,13] [5]rID->[0,5] [5]rID->[5,5] [16]value->[10,16]";

    format[call]        ="[13]opcode->[26,13] [5]rID->[5,5]";
    format[jmp]         ="[13]opcode->[26,13] [16]label->[10,16]";
    format[jz]          ="[13]opcode->[26,13] [5]rID->[5,5] [16]label->[10,16]";
    format[jnz]         ="[13]opcode->[26,13] [5]rID->[5,5] [16]label->[10,16]";

    format[ird]        ="[13]opcode->[26,13] [5]rID->[0,5] [5]rID->[5,5] [16]value->[10,16]";
    format[iwr]        ="[13]opcode->[26,13] [5]rID->[0,5] [5]rID->[5,5] [16]value->[10,16]";


    //--------------------------------------------------------------------------
    booleanOpCodes = new String[256];


    booleanOpCodes[bnop]	= "bnop";
    booleanOpCodes[rst]         = "rst";
    booleanOpCodes[endwhere]    = "endwhere";
    booleanOpCodes[elsew]       = "elsew";
    booleanOpCodes[src]         = "src";
    booleanOpCodes[csrc]	= "csrc";
    booleanOpCodes[ins]         = "ins";
    booleanOpCodes[loop]	= "loop";
    booleanOpCodes[rdback]	= "rdback";
    booleanOpCodes[rdfwd]	= "rdfwd";
    booleanOpCodes[clrfrt]	= "clrfrt";
    booleanOpCodes[restore]     = "restore";
    

    booleanOpCodes[movefl]	= "movefl";
    booleanOpCodes[savesel]	= "savesel";
    booleanOpCodes[wherenz]	= "wherenz";
    booleanOpCodes[wherez] 	= "wherez";
    
}

//------------------------------------------------------------------------------
private Opcodes(){
}

//------------------------------------------------------------------------------
public static String getNameByBooleanOpcode(byte _opcode){
String _opcodeMenomnic;
//--
    _opcodeMenomnic = booleanOpCodes[(int)_opcode & 0xff];
    if(_opcodeMenomnic == null){
        throw new RuntimeException("Unknown boolean opcode: " + _opcode);
    }

    return _opcodeMenomnic;
}

//------------------------------------------------------------------------------
public static byte getBooleanOpcodeByName(String _name){
    for(int i=0; i<booleanOpCodes.length; i++){
        if(_name.equals(booleanOpCodes[i])){
            return (byte)i;
        }
    }

    throw new RuntimeException("Unknown boolean opcode: " + _name);
}

//------------------------------------------------------------------------------
public static String[] getFormat(int _opcode, int _variant){
    try{
        if(format[_opcode].length() == 0){
            return new String[0];
        }else{
            String[] _variants = format[_opcode].split("\\|");
            return _variants[_variant].split(" ");
        }
    }catch(NullPointerException _npe){
        throw new RuntimeException("Unknown opcode: " + Integer.toHexString(_opcode));
    }
}

//------------------------------------------------------------------------------
public static String[] getFormatByName(String _opcode){
    return getFormat(getOpcodeByName(_opcode) & 0x1ff, getOpcodeVariantByName(_opcode));
}

//------------------------------------------------------------------------------
public static String getNameByOpcode(short _opcode){
String _opcodeMenomnic;
//--
    _opcodeMenomnic = opCodes[_opcode];
    if(_opcodeMenomnic == null){
        throw new RuntimeException("Unknown opcode: " + _opcode);
    }

    return _opcodeMenomnic.indexOf('|') >= 0 ? _opcodeMenomnic.split("\\|")[0] : _opcodeMenomnic;
}

//------------------------------------------------------------------------------
public static short getOpcodeByName(String _name){
    for(int i=0; i<opCodes.length; i++){
        if(opCodes[i] == null){
            continue;
        }
        String[] _names = opCodes[i].split("\\|", -1);
        for(int j=0; j<_names.length; j++){
            if(_name.equals(_names[j])){
                return (short)i;
            }
        }
    }

    throw new RuntimeException("Unknown opcode: " + _name);
}

//------------------------------------------------------------------------------
private static short getOpcodeVariantByName(String _name){
    for(int i=0; i<opCodes.length; i++){
        if(opCodes[i] == null){
            continue;
        }
        String[] _names = opCodes[i].split("\\|", -1);
        for(int j=0; j<_names.length; j++){
            if(_name.equals(_names[j])){
                return (short)j;
            }
        }
    }

    throw new RuntimeException("Unknown opcode: " + _name);
}

//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
//      Change History:
//      $Log: Opcodes.java,java $
