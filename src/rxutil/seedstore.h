// Copyright (c) 2020 The Lux developers/barrystyle
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"

class uint256;

//! seedHash store and generation funcs
std::string getSeedHash();
void setSeedHash(std::string& thisSeed);
void seedStoreInit();
void seedStoreWrite(int height, uint256 seedHash);
void barrysPreposterouslyNamedSeedHashFunction(int nHeight, std::string& thisSeed);
