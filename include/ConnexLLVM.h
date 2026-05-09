//===-- Connex.h - Top-level interface for Connex representation ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_CONNEX_CONNEX_H
#define LLVM_LIB_TARGET_CONNEX_CONNEX_H

//#include "MCTargetDesc/ConnexMCTargetDesc.h"
//#include "llvm/Target/TargetMachine.h"


/* I am defining the reserved register(s) of Connex, which I use for:
    - VSELECT code generation
    - handling COPY instructions in WHERE blocks
     (see ConnexTargetMachine.cpp and ConnexISelLowering.cpp), etc
*/
//#define CONNEX_RESERVED_REGISTER_01 Connex::Wh30
//#define CONNEX_RESERVED_REGISTER_01 Connex::Wh31
// For test only (I checked to see LLVM is NO longer using R0 in its code): #define CONNEX_RESERVED_REGISTER_01 Connex::Wh0

#define COPY_REGISTER_IMPLEMENTED_WITH_ORV_H

/*
// These lines have to be commented to work in the Opincaa library.
namespace llvm {
    class ConnexTargetMachine;
    FunctionPass *createConnexISelDag(ConnexTargetMachine &TM);
}
*/

#endif
