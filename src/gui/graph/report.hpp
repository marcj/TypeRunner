#pragma once

#ifdef REPORTING

namespace report {

    // network simplex
    namespace simplex {
        inline int iters = 0;
    } 

    //namespace crossing {
        // crossing stuff
        inline int random_runs = -1;
        inline int forgivness = -1;
        inline bool transpose = false; 

        inline int base = 0;
        inline int final = 0;
        inline int iters = 0;
        inline std::vector<int> random_final;
    //}
}

#endif
