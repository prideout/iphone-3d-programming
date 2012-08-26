struct Bone {
    unsigned short A;
    unsigned short B;
    char IsBlended;
};

static const Bone StickFigureBones[] = {
    
    // Head
    0,  1,  0, // 0
    1,  2,  1, // 1
    2,  3,  1, // 2
    
    // Hero's Left Arm
    4,  5,  0, // 3
    5,  6,  1, // 4
    6,  7,  1, // 5
    
    // Hero's Right Arm
    8,  9,  0, // 6
    9,  10, 1, // 7
    10, 11, 1, // 8
    
    // Unskinned
    2,  4,  0, // Left Shoulder
    2,  8,  0, // Right Shoulder

    // Hero's Left Leg
    0,  12, 0, // 9
    12, 13, 1, // 10
    13, 14, 1, // 11
    
    // Hero's Right Leg
    0,  15, 0, // 12
    15, 16, 1, // 13
    16, 17, 1, // 14
};
