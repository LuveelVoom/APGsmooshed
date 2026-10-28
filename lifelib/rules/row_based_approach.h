
    void iterate_var_grid(uint8_t *ingrid, uint8_t *outgrid) {

        for (int i = 0; i < 16; i++) {
            iterate_var_row(ingrid + (32 * i), outgrid + (16 * i));
        }

    }
