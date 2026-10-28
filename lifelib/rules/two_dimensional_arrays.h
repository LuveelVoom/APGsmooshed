
    /*
    * This is a convenience file to enable you to define a function with
    * the signature:
    *
    * void iterate_2d_grid(uint8_t (&inarr)[32][32], uint8_t (&outarr)[16][16])
    *
    * instead of the more low-level signature:
    * 
    * void iterate_var_grid(uint8_t *ingrid, uint8_t *outgrid)
    *
    * Note that the first coordinate is y and the second coordinate is x.
    */

    void iterate_var_grid(uint8_t *ingrid, uint8_t *outgrid) {

        uint8_t  (&inarr)[32][32] = *reinterpret_cast<uint8_t (*)[32][32]>(ingrid);
        uint8_t (&outarr)[16][16] = *reinterpret_cast<uint8_t (*)[16][16]>(outgrid);
        iterate_2d_grid(inarr, outarr);

    }
