/* Alex: IMPORTANT: we need to set it to the maximum value possible because the
  static members of ConnexVector get initiated at the very beginning of the
  program. */
int CONNEX_VECTOR_LENGTH = 128;
//int CONNEX_VECTOR_LENGTH = 1024;
//int CONNEX_VECTOR_LENGTH = 16384;

int LOG2_CONNEX_VECTOR_LENGTH;


void ComputeLog2CVL() {
  switch (CONNEX_VECTOR_LENGTH) {
    case 4096:
      LOG2_CONNEX_VECTOR_LENGTH = 12;
      break;
    case 2048:
      LOG2_CONNEX_VECTOR_LENGTH = 11;
      break;
    case 1024:
      LOG2_CONNEX_VECTOR_LENGTH = 10;
      break;
    case 512:
      LOG2_CONNEX_VECTOR_LENGTH = 9;
      break;
    case 256:
      LOG2_CONNEX_VECTOR_LENGTH = 8;
      break;
    case 128:
      LOG2_CONNEX_VECTOR_LENGTH = 7;
      break;
    case 64:
      LOG2_CONNEX_VECTOR_LENGTH = 6;
      break;
    case 32:
      LOG2_CONNEX_VECTOR_LENGTH = 5;
      break;
    case 16:
      LOG2_CONNEX_VECTOR_LENGTH = 4;
      break;
    case 8:
      LOG2_CONNEX_VECTOR_LENGTH = 3;
      break;
    case 4:
      LOG2_CONNEX_VECTOR_LENGTH = 2;
      break;
    case 2:
      LOG2_CONNEX_VECTOR_LENGTH = 1;
      break;
    default:
      LOG2_CONNEX_VECTOR_LENGTH = 0;
  }
}


int CONNEX_MEM_SIZE = 1024;
int CONNEX_REG_COUNT = 32;
int INSTRUCTION_QUEUE_LENGTH = 1024;
