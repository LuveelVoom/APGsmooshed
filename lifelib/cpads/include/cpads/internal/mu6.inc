
constexpr uint64_t mu[6] = {0x5555555555555555ull,
                            0x3333333333333333ull,
                            0x0f0f0f0f0f0f0f0full,
                            0x00ff00ff00ff00ffull,
                            0x0000ffff0000ffffull,
                            0x00000000ffffffffull};

constexpr size_t log2_bits_per_word = 3 + hh::constexpr_log2(sizeof(T));

static_assert(log2_bits_per_word <= 6, "word size must be <= 64 bits");
