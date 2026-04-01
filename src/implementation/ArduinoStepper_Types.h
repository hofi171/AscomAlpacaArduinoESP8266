#ifndef E84F3C69_B2E3_4B6C_BB62_788495DB085A
#define E84F3C69_B2E3_4B6C_BB62_788495DB085A


enum eCOMMAND {
  SINGLE_STEP =0,
  STOP = 1,
  TO_POS = 2,
  SET_POS = 3,
  SET_SPEED = 4,
  SET_MODE = 5,
  GET_VCC = 6,
  GET_MODE = 7
};

enum eSTEPMODE {
  FullStep =0,
  HalfStep = 1,
  QuaterStep = 2,
  EighthStep = 3,
  SixteenthStep = 4,
  FullStep2Phase = 5
};

#endif /* E84F3C69_B2E3_4B6C_BB62_788495DB085A */
