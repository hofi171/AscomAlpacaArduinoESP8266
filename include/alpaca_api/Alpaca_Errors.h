#ifndef FAA2112D_262B_4CB1_9038_615ABBCD45DC
#define FAA2112D_262B_4CB1_9038_615ABBCD45DC


enum class AlpacaError {
    Success = 0x0,
    NotImplemented = 0x400,
    InvalidValue = 0x401,       
    ValueNotSet = 0x402,
    NotConnected = 0x407,
    InvalidWhileParked = 0x408,
    InvalidWhileSlaved = 0x409,
    InvalidOperation = 0x40B,
    ActionNotImplemented = 0x40C
};

#endif /* FAA2112D_262B_4CB1_9038_615ABBCD45DC */
