#pragma once
#include <unordered_map>

#include "mmtypes.h"

namespace std {
    template<> struct hash<MoaID> {
        size_t operator()(const MoaID &moaID) const noexcept {
            std::hash<uint64_t> uint64Hash;

            uint64_t* key = (uint64_t*)&moaID;
            return uint64Hash(key[0]) ^ uint64Hash(key[1]);
        }
    };
}

/*
struct MoaIDComparer {
    bool operator()(const MoaID &moaID, const MoaID &moaID2) const {
        return memcmp(&moaID, &moaID2, sizeof(MoaID)) == -1;
    }
};
*/