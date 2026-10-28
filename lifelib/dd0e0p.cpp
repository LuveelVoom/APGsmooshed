/*
* dd0e0p -- write a logical glider stream to a physical pattern file.
*
* Mandatory options:
*
* if=stream.txt           Location of glider stream
* of=filename.mc          Pattern file to which to append
*
* Optional options:
*
* tf=target.mc            If you wish to not overwrite the target
* box=378,-43,3,3         Specify initial glider (rather than detect)
* final                   Remove initial glider afterwards
* absolute=523857         Advance pattern by n generations
* relative=1000           Advance pattern by n generations beyond recipe end
* bs=1048576              Advance by n generations at a time
*/

#include "pattern2.h"
#include "ssplit.h"
#include "streamlife.h"
#include <fstream>
#include <ctime>

void pplus(uint64_t maxmem, std::string targetfile, int64_t *bbox, std::vector<int64_t> &gstream, apg::pattern &z, apg::pattern &h) {

    apg::lifetree<uint32_t, 1> lt(maxmem);
    apg::pattern x(&lt, targetfile);
    if (gstream.size() == 0) {
        z += x;
    } else {
        apg::pattern y = (bbox[3] == 0) ? (x & x[2] & x[4] & x[6] & x[8]) : (x.rmrect(bbox));
        apg::pattern g = x - y;
        if (g.empty()) {
            std::cerr << "Pattern contains no glider." << std::endl;
            exit(1);
        }
        g.getrect(bbox);
        std::cout << "box=" << bbox[0] << "," << bbox[1] << "," << bbox[2] << "," << bbox[3] << std::endl;
        y += g.stream(gstream);
        z += y;
        h += g;
    }

}

int main(int argc, char *argv[]) {

    std::string infile = "";
    std::string outfile = "";
    std::string targetfile = "";
    int64_t bbox[4] = {0};
    bool fin = false;
    int64_t absolute = -1;
    int64_t relative = 0;
    int64_t blocksize = 1048576;
    int64_t backup = 0;
    uint64_t maxmem = 1000;
    uint64_t stepcount = 0;
    uint64_t se = 0;

    for (int i = 1; i < argc; i++) {
        std::string s = argv[i];
        std::string com = s;
        std::string rest = "";

        size_t j = s.find('=');
        if (j != std::string::npos) {
            com = s.substr(0, j);
            rest = s.substr(j+1);
        }

        if (com == "absolute") {
            absolute = std::stoll(rest);
        } else if (com == "relative") {
            relative = std::stoll(rest);
        } else if (com == "final") {
            fin = true;
        } else if (com == "pp") {
            backup = std::stoll(rest);
        } else if (com == "bs") {
            blocksize = std::stoll(rest);
        } else if (com == "se") {
            se = std::stoll(rest);
        } else if (com == "mem") {
            maxmem = std::stoll(rest);
        } else if (com == "box") {
            std::vector<std::string> v = apg::string_split(rest, ',');
            for (uint64_t k = 0; k < v.size(); k++) {
                if (k == 4) { break; }
                bbox[k] = std::stoll(v[k]);
            }
        } else if (com == "is") {
            stepcount = std::stoll(rest);
        } else if (com == "if") {
            infile = rest;
        } else if (com == "of") {
            outfile = rest;
        } else if (com == "tf") {
            targetfile = rest;
        } else {
            std::cerr << "Invalid command: \033[31;1m" << com << "\033[0m";
            if (rest != "") { std::cerr << "=" << rest; }
            std::cerr << std::endl;
            exit(1);
        }

    }

    if (outfile == "") {
        std::cerr << "Usage: ./dd0e0p if=recipe.txt of=filename.mc" << std::endl;
        exit(1);
    }
    if (targetfile == "") { targetfile = outfile; }

    std::vector<int64_t> gstream;
    if (infile != "") {
        std::ifstream in(infile);
        apg::onlyints(gstream, in);
    }

    apg::streamtree<uint32_t, 1> st(maxmem);
    apg::pattern y(&st, "", "b3s23");
    apg::pattern g(&st, "", "b3s23");
    pplus(maxmem, targetfile, bbox, gstream, y, g);

    if (absolute < 0) {
        int64_t p = 0 - absolute;
        absolute = relative;
        for (uint64_t i = 0; i < gstream.size(); i++) {
            absolute += gstream[i];
        }
        if ((absolute % p) != 0) {
            absolute += p;
            absolute -= (absolute % p);
        }
    }

    std::cout << "absolute=" << absolute << std::endl;

    uint64_t lastcount = stepcount;
    clock_t lasttime = clock();

    if (blocksize > 0) {
        while (absolute > blocksize) {
            y = y[blocksize];
            absolute -= blocksize;
            std::cerr << absolute << " generations remaining..." << std::endl;

            stepcount += 1;
            clock_t thistime = clock();

            int64_t timespan = (thistime - lasttime) / CLOCKS_PER_SEC;

            if (((backup > 0) && (timespan > backup)) || ((se > 0) && ((stepcount % se) == 0))) {

                int64_t genspassed = (stepcount - lastcount) * blocksize;
                int64_t genspersec = genspassed / timespan;
                std::cerr << genspassed << " generations completed in " << timespan;
                std::cerr << " seconds (" << genspersec << " per second)." << std::endl;

                std::ostringstream ss;
                if (outfile.substr(outfile.size() - 3) == ".mc") {
                    ss << outfile.substr(0, outfile.size() - 3) << "_backup" << stepcount << ".mc";
                } else {
                    ss << outfile << "_backup" << stepcount;
                }
                std::string backupname = ss.str();
                std::ofstream out(backupname);
                y.write_macrocell(out);
                std::cerr << "Progress saved in " << backupname << std::endl;
                lasttime = thistime;
                lastcount = stepcount;
            }
        }
        y = y[absolute];
        if (!fin) {
            y += g;
        }
        std::ofstream out(outfile);
        y.write_macrocell(out);
    }

    return 0;

}
