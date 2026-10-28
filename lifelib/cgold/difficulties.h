#pragma once

#include <utility>
#include <map>
#include <set>
#include <string>

#define MAX_DIFFICULTY 1.0e80
typedef double difficul_t;
typedef std::pair<difficul_t, std::string> dsentry;

namespace apg {

    class DifficultyHolder {

        std::map<std::string, difficul_t> difficulties;

        public:
        DifficultyHolder() {

            #include "difficulties.inc"

        }

        std::string representative(std::string apgcode) {

            auto it = difficulties.find(apgcode);
            if (it != difficulties.end()) { return apgcode; }
            auto x = apgcode.find('_');
            if (x == std::string::npos) { return apgcode; }
            std::string prefix = apgcode.substr(0, x);
            auto it2 = difficulties.find(prefix);
            if (it2 != difficulties.end()) { return prefix; }
            if (apgcode.substr(0, 2) == "yl") {
                uint64_t period = std::stoll(apgcode.substr(2));
                if ((period == 144) || (period % 96 == 0)) {
                    return "corderpuffer";
                }
            }
            return apgcode;

        }

        difficul_t rep_difficulty(std::string rep) {

            auto it = difficulties.find(rep);
            if (it == difficulties.end()) {
                return MAX_DIFFICULTY;
            } else {
                return it->second;
            }

        }

        difficul_t get_difficulty(std::string apgcode) {

            if ((apgcode[0] != 'm') || (apgcode[1] != 'e') || (apgcode[2] != 't')) {
                // Not a methuselah:
                if ((apgcode[0] != 'y') && ((apgcode[0] != 'x') || (apgcode[1] == 's'))) {
                    // Not an oscillator, spaceship, or linear-growth pattern:
                    return 0;
                }
            }

            return rep_difficulty(representative(apgcode));
        }

        std::map<std::string, int64_t> to_reps(std::map<std::string, int64_t> &occ) {

            std::map<std::string, int64_t> occ2;
            for (auto it = occ.begin(); it != occ.end(); ++it) {
                occ2[representative(it->first)] += it->second;
            }
            return occ2;

        }

        void set_difficulties(std::string standard, std::map<std::string, int64_t> &occ_raw) {
            /*
            * Update difficulty estimates based upon initial segment of blockchain
            * objects. This only affects the difficulties of objects in such a way
            * that it cannot invalidate existing blocks.
            */

            auto occ = to_reps(occ_raw);

            std::set<std::pair<int64_t, std::string> > nocc;
            int64_t totobj = 0;
            for (auto it = occ.begin(); it != occ.end(); ++it) {
                difficul_t di = rep_difficulty(it->first);
                if (di > difficulties[standard]) {
                    nocc.emplace(it->second, it->first);
                    totobj += it->second;
                }
            }

            totobj += occ[standard];

            int64_t cumobj = 0;
            for (auto it = nocc.begin(); it != nocc.end(); ++it) {
                cumobj += it->first;
                difficulties[it->second] = (difficulties[standard] * totobj) / cumobj;
            }
        }
    };
}
