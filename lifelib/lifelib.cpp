#include "pattern2.h"
#include "streamlife.h"
#include "soup/hashsoup.h"
#include "ssplit.h"
#include <new>

extern "C"
{

    void* CreateLifetree(int maxmem, int nlayers) {
        /*
        * Since we don't know at compile-time how many layers our lifetree
        * has, we have explicit template instantiations for powers of two
        * and round up to the next one.
        *
        * Negative integers are used to denote 'special' algorithms such
        * as streamlife.
        */

        if (nlayers == -1) {
            return new(std::nothrow) apg::streamtree<uint32_t, 1>(maxmem);
        } else if (nlayers > 32) {
            return new(std::nothrow) apg::lifetree<uint32_t, 64>(maxmem);
        } else if (nlayers > 16) {
            return new(std::nothrow) apg::lifetree<uint32_t, 32>(maxmem);
        } else if (nlayers > 8) {
            return new(std::nothrow) apg::lifetree<uint32_t, 16>(maxmem);
        } else if (nlayers > 4) {
            return new(std::nothrow) apg::lifetree<uint32_t, 8>(maxmem);
        } else if (nlayers > 2) {
            return new(std::nothrow) apg::lifetree<uint32_t, 4>(maxmem);
        } else if (nlayers > 1) {
            return new(std::nothrow) apg::lifetree<uint32_t, 2>(maxmem);
        } else {
            return new(std::nothrow) apg::lifetree<uint32_t, 1>(maxmem);
        }
    }

    void DeleteLifetree(void *ptr, int nlayers) {
        if (nlayers == -1) {
            delete reinterpret_cast<apg::streamtree<uint32_t, 1>*>(ptr);
        } else if (nlayers > 32) {
            delete reinterpret_cast<apg::lifetree<uint32_t, 64>*>(ptr);
        } else if (nlayers > 16) {
            delete reinterpret_cast<apg::lifetree<uint32_t, 32>*>(ptr);
        } else if (nlayers > 8) {
            delete reinterpret_cast<apg::lifetree<uint32_t, 16>*>(ptr);
        } else if (nlayers > 4) {
            delete reinterpret_cast<apg::lifetree<uint32_t, 8>*>(ptr);
        } else if (nlayers > 2) {
            delete reinterpret_cast<apg::lifetree<uint32_t, 4>*>(ptr);
        } else if (nlayers > 1) {
            delete reinterpret_cast<apg::lifetree<uint32_t, 2>*>(ptr);
        } else {
            delete reinterpret_cast<apg::lifetree<uint32_t, 1>*>(ptr);
        }
    }

    int GetDiameterOfPattern(void *ptr) {
        auto ppat = reinterpret_cast<apg::pattern*>(ptr);
        return ppat->logdiam();
    }

    void DeletePattern(void *ptr) {
        delete reinterpret_cast<apg::pattern*>(ptr);
    }

    void SavePatternRLE(void *ptr, const char *filename, const char *header, const char *footer) {

        std::ofstream out(filename);
        auto ppat = reinterpret_cast<apg::pattern*>(ptr);
        out << header;
        ppat->write_rle(out);
        out << footer << std::endl;
    }

    void SavePatternMC(void *ptr, const char *filename, const char *header, const char *footer) {

        std::ofstream out(filename);
        auto ppat = reinterpret_cast<apg::pattern*>(ptr);
        auto lab = ppat->getlab();
        lab->write_macrocell_header(out);
        out << header;
        lab->write_macrocell_headerless(out, ppat->gethnode(), ppat->getrule());
        out << footer << std::endl;
    }

    void LoadTimelineMC(void* lt, const char *filename, int maxframes, void* ptr) {
        auto lab = reinterpret_cast<apg::lifetree_abstract<uint32_t>*>(lt);
        std::vector<apg::hypernode<uint32_t> > hvec;
        std::string rule = "";
        lab->load_macrocell(filename, rule, &hvec);
        apg::pattern** patarray = reinterpret_cast<apg::pattern**>(ptr);

        int elf = 0;
        for (auto it = hvec.begin(); it != hvec.end(); ++it) {
            if (elf == maxframes) { break; }
            if (it->index != 0) {
                patarray[elf] = new(std::nothrow) apg::pattern(lab, (*it), rule);
                elf += 1;
            }
        }

        while (elf < maxframes) {
            patarray[elf] = 0; elf += 1;
        }
    }

    void SaveTimelineMC(void* lt, const char *filename, const char *header, const char* footer, void *ptr, int exponent) {

        auto lab = reinterpret_cast<apg::lifetree_abstract<uint32_t>*>(lt);

        apg::pattern** patarray = reinterpret_cast<apg::pattern**>(ptr);

        apg::pattern emptypat(lab, "", patarray[0]->getrule());

        std::vector<apg::hypernode<uint32_t> > hnodes;

        uint64_t i = 0;
        while (patarray[i] != 0) {
            hnodes.push_back(emptypat.coerce(*(patarray[i])));
            i += 1;
        }
        uint64_t startgen = (reinterpret_cast<uint64_t*>(ptr))[i+1];

        std::ofstream out(filename);
        lab->write_macrocell_header(out);
        out << header;
        lab->write_macrocell_headerless(out, hnodes, emptypat.getrule(), 2, exponent, startgen);
        out << footer << std::endl;
    }

    void* BooleanPatternImmutable(void* pat1, void* pat2, int op) {
        auto ppat1 = reinterpret_cast<apg::pattern*>(pat1);
        auto ppat2 = reinterpret_cast<apg::pattern*>(pat2);
        auto lab = ppat1->getlab();
        return new(std::nothrow) apg::pattern(lab, lab->boolean_universe(ppat1->gethnode(), ppat1->coerce(*ppat2), op), ppat1->getrule());
    }

    void BooleanPatternMutable(void* pat1, void* pat2, int op) {
        auto ppat1 = reinterpret_cast<apg::pattern*>(pat1);
        auto ppat2 = reinterpret_cast<apg::pattern*>(pat2);
        auto lab = ppat1->getlab();
        ppat1->changehnode(lab->boolean_universe(ppat1->gethnode(), ppat1->coerce(*ppat2), op));
    }

    void* CreatePatternFromFile(void* lt, const char *filename) {
        auto lab = reinterpret_cast<apg::lifetree_abstract<uint32_t>*>(lt);
        return new(std::nothrow) apg::pattern(lab, std::string(filename));
    }

    void* CreatePatternFromFileContents(void* lt, const char *contents) {
        auto lab = reinterpret_cast<apg::lifetree_abstract<uint32_t>*>(lt);
        std::string rule = "b3s23";
        std::stringstream ss(contents);
        apg::hypernode<uint32_t> hnode = lab->read_macrocell(ss, rule, 0);
        return new(std::nothrow) apg::pattern(lab, hnode, std::string(rule));
    }

    void* CreateRectangle(void* lt, int x, int y, int width, int height, const char *rule) {
        auto lab = reinterpret_cast<apg::lifetree_abstract<uint32_t>*>(lt);
        auto hnode = lab->rectangle(x, y, width, height);
        return new(std::nothrow) apg::pattern(lab, hnode, std::string(rule));
    }

    void* CreatePatternFromRLE(void* lt, const char *rle, const char *rule) {
        auto lab = reinterpret_cast<apg::lifetree_abstract<uint32_t>*>(lt);
        return new(std::nothrow) apg::pattern(lab, std::string(rle), std::string(rule));
    }

    void* Hashsoup(void* lt, const char *rule, const char *symmetry, const char *seed) {
        auto lab = reinterpret_cast<apg::lifetree_abstract<uint32_t>*>(lt);
        std::vector<apg::bitworld> vbw = apg::hashsoup(std::string(seed), std::string(symmetry));
        return new(std::nothrow) apg::pattern(lab, vbw, std::string(rule));
    }

    void* AdvancePattern(void* pat, int numgens, uint32_t exponent) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return new(std::nothrow) apg::pattern(ppat->advance2(numgens, exponent));
    }

    void* GetSolidForPattern(void* pat, uint32_t state, uint32_t exponent) {
        uint64_t truestate = ((uint64_t) state) << exponent;
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        auto lab = ppat->getlab();
        uint64_t depth = ppat->gethnode().depth;
        return new(std::nothrow) apg::pattern(lab, lab->solid(depth, truestate), ppat->getrule());
    }

    void* GetSemisolidForPattern(void* pat, int flags) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        auto lab = ppat->getlab();
        uint64_t depth = ppat->gethnode().depth;
        return new(std::nothrow) apg::pattern(lab, lab->semisolid(depth, flags), ppat->getrule());
    }

    void* BitshiftPattern(void* pat, int shift) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return new(std::nothrow) apg::pattern(ppat->bitshift(shift));
    }

    void* ShiftPattern(void* pat, int x, int y, uint32_t exponent) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return new(std::nothrow) apg::pattern(ppat->shift(x, y, exponent));
    }

    void* TransformPattern(void* pat, const char *tfm) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return new(std::nothrow) apg::pattern(ppat->transform(std::string(tfm), 0, 0));
    }

    void* MakeSpaceshipStream(void* pat, const char *infile) {
        std::vector<int64_t> gstream;

        if (infile[0] == '[') {
            std::istringstream in(infile);
            apg::onlyints(gstream, in);
        } else {
            std::ifstream in(infile);
            apg::onlyints(gstream, in);
        }
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return new(std::nothrow) apg::pattern(ppat->stream(gstream));
    }

    void* FindConnectedComponent(void* v_seed, void* v_backdrop, void* v_corona) {

        auto p_seed = reinterpret_cast<apg::pattern*>(v_seed);
        auto p_backdrop = reinterpret_cast<apg::pattern*>(v_backdrop);
        auto p_corona = reinterpret_cast<apg::pattern*>(v_corona);

        apg::pattern agglom = (*p_seed) & (*p_backdrop);
        apg::pattern cluster = agglom;

        while (cluster.nonempty()) {
            cluster = cluster.convolve(*p_corona);
            cluster &= (*p_backdrop);
            cluster -= agglom;
            agglom += cluster;
        }

        return new(std::nothrow) apg::pattern(agglom);

    }

    void* GetOneCell(void* pat) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return new(std::nothrow) apg::pattern(ppat->onecell());
    }

    void* MatchLive(void* pat, void* pat1) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        auto ppat1 = reinterpret_cast<apg::pattern*>(pat1);
        return new(std::nothrow) apg::pattern(ppat->match(*ppat1));
    }

    void* CopyPattern(void* pat) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return new(std::nothrow) apg::pattern(*ppat);
    }

    void* MatchLiveAndDead(void* pat, void* pat1, void* pat0) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        auto ppat1 = reinterpret_cast<apg::pattern*>(pat1);
        auto ppat0 = reinterpret_cast<apg::pattern*>(pat0);
        return new(std::nothrow) apg::pattern(ppat->match(*ppat1, *ppat0));
    }

    void* FindPeriodOrAdvance(void* pat, int exponent) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        apg::pattern x = ppat->pdetect(1ull << exponent);

        if (ppat->dt != 0) {
            ppat->ascertain_period();
            return nullptr;
        } else {
            return new(std::nothrow) apg::pattern(x);
        }
    }

    int GetPopulationOfPattern(void* pat, int modprime) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return ppat->popcount(modprime);
    }

    void GetPatternBox(void* pat, int64_t* bbox) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        ppat->getrect(bbox);
    }

    uint64_t GetPatternDigest(void* pat) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        if (ppat->empty()) { return 0; }
        int64_t bbox[4];
        ppat->getrect(bbox);
        return ppat->shift(0 - bbox[0], 0 - bbox[1]).digest();
    }

    uint64_t GetPatternOctodigest(void* pat) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        if (ppat->empty()) { return 0; }

        uint64_t digests[8];
        uint8_t perms[8] = {27, 177, 78, 216, 39, 114, 141, 228};

        for (int i = 0; i < 8; i++) {

            auto lab = ppat->getlab();

            apg::pattern tp(lab, lab->transform_recurse(ppat->gethnode(), perms[i]), ppat->getrule());

            int64_t bbox[4];
            tp.getrect(bbox);
            digests[i] = tp.shift(0 - bbox[0], 0 - bbox[1]).digest();
        }

        for (int i = 7; i > 0; i--) {
            for (int j = 0; j < i; j++) {
                if (digests[j] > digests[j+1]) {
                    uint64_t c = digests[j];
                    digests[j] = digests[j+1];
                    digests[j+1] = c;
                }
            }
        }

        uint64_t acc = digests[0];
        uint64_t idx = 1;

        for (int i = 1; i < 8; i++) {
            if (digests[i] != digests[i-1]) {
                idx += 2;
                acc += digests[i] * idx;
            }
        }

        return acc;
    }

    void GetRuleOfPattern(void* pat, char* buffer) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        std::string s = ppat->getrule();
        s.copy(buffer, 2048);
    }

    void GetWechslerOfPattern(void* pat, char* buffer, int buflen) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        std::string s = ppat->phase_wechsler(999999999);
        apg::try_copy(s, buffer, buflen);
    }

    void GetRleOfPattern(void* pat, char* buffer, int buflen) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        std::ostringstream ss;
        ppat->write_rle(ss);
        std::string s = ss.str();
        apg::try_copy(s, buffer, buflen);
    }

    void GetApgcodeOfPattern(void* pat, char* buffer, int buflen) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        std::string s = ppat->apgcode(999999999, 999999999);
        apg::try_copy(s, buffer, buflen);
    }

    void GetCompiledVersion(char* buffer) {
        std::string s(LIFELIB_VERSION);
        s.copy(buffer, 2048);
    }

    void GetCoords(void* pat, int64_t* coords) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        ppat->get_coords(coords);
    }

    void GetCells(void* pat, int ncells, int64_t* coords, uint64_t* states) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        for (int i = 0; i < ncells; i++) {
            states[i] = ppat->getcell(coords[2*i], coords[2*i+1]);
        }
    }

    void GetSubpops(void* pat, int n, int pixelsize, uint64_t* pops) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        auto hnode = ppat->gethnode();
        auto lab = ppat->getlab();
        uint32_t target_depth = n + pixelsize - 4;
        while (hnode.depth < target_depth) { hnode = lab->pyramid_up(hnode); }

        uint64_t dimension = (1ull << n);
        for (uint64_t y = 0; y < dimension; y++) {
            for (uint64_t x = 0; x < dimension; x++) {
                pops[(y << n) + x] = lab->getpop_recurse(lab->subnode(hnode, x, y, n), 1073750017, ((uint64_t) -1));
            }
        }
    }

    void SetCells(void* pat, int ncells, int64_t* coords, uint64_t* states) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        auto lab = ppat->getlab();
        auto newnode = lab->bror_recurse(lab->fromcells(ncells, coords, 0));
        ppat->changehnode(lab->boolean_universe(ppat->gethnode(), newnode, 3));
        auto newnode2 = lab->fromcells(ncells, coords, states);
        ppat->changehnode(lab->boolean_universe(ppat->gethnode(), newnode2, 1));
    }

    uint64_t GetPatternBound(void* pat, int direction, int pixelsize) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        auto lab = ppat->getlab();
        return lab->bound_recurse(ppat->gethnode(), direction, pixelsize);
    }

    int64_t GetDXOfPattern(void* pat) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return ppat->dx;
    }

    int64_t GetDYOfPattern(void* pat) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return ppat->dy;
    }

    int64_t GetDTOfPattern(void* pat) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return ppat->dt;
    }

    uint64_t GetOriginState(void* pat) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return ppat->getcell(0, 0);
    }

    uint64_t GetBeszelIndex(void* pat) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return ppat->gethnode().index;
    }

    uint64_t GetUlqomaIndex(void* pat) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return ppat->gethnode().index2;
    }

    bool PatternNonempty(void* pat) {
        auto ppat = reinterpret_cast<apg::pattern*>(pat);
        return ppat->gethnode().nonempty();
    }

    bool PatternEquality(void* pat1, void* pat2) {
        auto ppat1 = reinterpret_cast<apg::pattern*>(pat1);
        auto ppat2 = reinterpret_cast<apg::pattern*>(pat2);
        return ((*ppat1) == (*ppat2));
    }
}
