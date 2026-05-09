#ifndef CHECK_DATA_HAZARD_INCLUDE
#define CHECK_DATA_HAZARD_INCLUDE

/* This function should be very efficient since it is (also) called in
     Kernel::append().
     The function is not very heavy - it makes 6 comparisons and can
       emit a debug message. */
inline bool CheckDataHazard(Instruction &instrCrt, Instruction &instrPrev,
                     //bool emitErrorMessage = true,
                     bool emitErrorMessage = false,
                     bool isSimulator = false) {
    bool res = false;
    int instrCrtOpcode = instrCrt.getOpcode();

    /*
    cout << "Entered CheckDataHazard(): instrCrt = "
         << instrCrt.toString()
         << " instrPrev = "
         << instrPrev.toString()
         << endl;
    */

    if (instrCrtOpcode == _READ) {
        //if (instrPrev.getDest() == instrCrt.getDest()) {
        if (instrPrev.getDest() == instrCrt.getRight()) {
            // We read from address previously assigned
            //return true;
            res = true;
        }
    }
    else
    if (instrCrtOpcode == _WRITE) {
        if (instrPrev.getDest() == instrCrt.getLeft() ||
            // We store previously assigned register
            instrPrev.getDest() == instrCrt.getRight()) {
            // We store at address previously assigned - TODO: might NOT be a problem
            //return true;
            res = true;
        }
    }
    else
    if (instrCrtOpcode == _IWRITE) {
        if (instrPrev.getDest() == instrCrt.getLeft()) {
                    // We store previously assigned register
            //return true;
            res = true;
        }
    }
    else
    if (instrCrtOpcode == _WHERE_EQ) {
        if (instrPrev.getOpcode() == _EQ) {
            // We have _EQ just before WHERE_EQ
            //return true;
            res = true;
        }
    }
    else
    if (instrCrtOpcode == _WHERE_LT) {
        if (instrPrev.getOpcode() == _LT ||
            instrPrev.getOpcode() == _ULT) {
            // We have _(U)LT just before WHERE_LT
            //return true;
            res = true;
        }
    }
    else
    if (instrCrtOpcode == _WHERE_CRY) {
        if (instrPrev.getOpcode() == _ADDC ||
            instrPrev.getOpcode() == _SUBC) {
            // We have _ADD/SUBC just before WHERE_CRY
            //return true;
            res = true;
        }
    }
    // MEGA-TODO: find data hazards between CELL_SH* and SHIFT_REG instructions

    if (res == true && emitErrorMessage) {
        cout << (isSimulator ? "Simulator: " : "Opincaa program: ")
             << "CheckDataHazard(): we found a data hazard between instrCrt = "
             //<< instrCrt.toString()
             << instrCrt.dump()
             << " and the previous instruction instrPrev = "
             //<< instrPrev.toString()
             << instrPrev.dump()
             << endl;

        /* Killing the simulator turns to be a bad idea, since the Opincaa
         *   program continues running and expects data from the simulator.
         *
          // assert(0 && "CheckDataHazard(): we found a data hazard");
        */

        FILE *fout;

        if (isSimulator) {
            fout = fopen("Error_ConnexSim_OPINCAA_DataHazards.txt", "wt");
            fprintf(fout, "Data hazard detected!\n"
                     "Please check Opincaa simulator's output for CheckDataHazard().");
        }
        else {
            fout = fopen("Error_Connex_OPINCAA_DataHazards.txt", "wt");
            fprintf(fout, "Data hazard detected!\n"
                     "Please check Opincaa program's output for CheckDataHazard().");
        }
        fclose(fout);
    }

    //instrPrev = instrCrt;

    return res;
}
#endif

