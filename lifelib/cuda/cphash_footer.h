
    // Save into multiverse:
    multiverse[threadnum + 64] = b;

    // Zero remaining array:
    ZERO_SIX_TILES; ZERO_SIX_TILES; ZERO_SIX_TILES;

    if (usize == 0) { return; } // 19 tiles

    ZERO_SIX_TILES; ZERO_SIX_TILES; ZERO_SIX_TILES;

    if (usize == 1) { return; } // 37 tiles

    ZERO_SIX_TILES; ZERO_SIX_TILES; ZERO_SIX_TILES;
    ZERO_SIX_TILES; ZERO_SIX_TILES; ZERO_SIX_TILES;
    ZERO_SIX_TILES; ZERO_SIX_TILES; ZERO_SIX_TILES;

    if (usize == 2) { return; } // 91 tiles

    ZERO_SIX_TILES; ZERO_SIX_TILES; ZERO_SIX_TILES;
    ZERO_SIX_TILES; ZERO_SIX_TILES; ZERO_SIX_TILES;
