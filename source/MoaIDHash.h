#pragma once
#include <unordered_map>

#include "mmtypes.h"

namespace std {
    template<> struct hash<MoaID> {
        size_t operator()(const MoaID &moaID) const noexcept {
            std::hash<uint64_t> uint64Hash;

            uint64_t* key = (uint64_t*)&moaID;

            if (key) {
                return uint64Hash(key[0]) ^ uint64Hash(key[1]);
            }
            return uint64Hash(0);
        }
    };
}

/*
struct MoaIDComparer {
    bool operator()(const MoaID &moaID, const MoaID &moaID2) const {
        const size_t MOA_ID_SIZE = sizeof(MoaID);
        return memcmp(&moaID, &moaID2, MOA_ID_SIZE) == -1;
    }
};
*/