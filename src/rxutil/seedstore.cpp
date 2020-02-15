// Copyright (c) 2020 The Lux developers/barrystyle
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "seedstore.h"

#include "init.h"
#include "main.h"
#include "util.h"
#include "chainparams.h"

std::string strCurrentSeed = "";
const std::string seedDB = "seedstore";
std::vector<std::pair<int, uint256>> seedGenerationHeights;
std::string currentSeed = "0000000000000000000000000000000000000000000000000000000000000000";

std::string getSeedHash()
{
    return currentSeed;
}

void setSeedHash(std::string& thisSeed)
{
    currentSeed = thisSeed;
    LogPrintf("setSeedHash to %s\n", thisSeed);
}

leveldb::DB* seedStoreDb()
{
    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;

    //! open seedstore db
    boost::filesystem::path seedStoreHnd = GetDataDir() / seedDB;
    leveldb::Status status = leveldb::DB::Open(options, seedStoreHnd.c_str(), &db);
    if (!status.ok()) {
       delete db;
       printf("%s - Can't open %s, exiting..\n", __func__, seedStoreHnd.c_str());
       StartShutdown();
    }

    return db;
}

void seedStoreInit()
{
    leveldb::DB* db = seedStoreDb();
    if (!db) return;

    //! read and see what we've got
    int height = 0;
    uint256 seedHash = uint256();
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        height = atoi(it->key().ToString());
        seedHash = uint256S(it->value().ToString());
        if (height > 0) {
            seedGenerationHeights.push_back(make_pair(height, seedHash));
            printf("(r) height %08d [%s]\n", height, seedHash.ToString().c_str());
        }
    }
    delete it;
    delete db;
}

void seedStoreWrite(int height, uint256 seedHash)
{
    leveldb::DB* db = seedStoreDb();
    if (!db) return;

    //! write the pair
    leveldb::WriteOptions writeOptions;
    db->Put(writeOptions, to_string(height), seedHash.ToString());
    printf("(w) height %08d [%s]\n", height, seedHash.ToString().c_str());
    delete db;
}

void barrysPreposterouslyNamedSeedHashFunction(int nHeight, std::string& thisSeed)
{
    const int epochHeight = Params().GetConsensus().nEpochHeight;
    const int epochInterval = Params().GetConsensus().nEpochInterval;

    //! return thisSeed as zero
    if (nHeight < epochHeight) {
        setSeed("0000000000000000000000000000000000000000000000000000000000000000");
        return;
    }

    //! fetch seed for appropriate epoch
    if (seedGenerationHeights.size() > 0) {
       for (const auto &l : seedGenerationHeights) {
          int lastHeight = l.first;
          uint256 lastSeed = l.second;
          //! in case of rewinding chain
          if (lastHeight > nHeight)
              continue;
          //! otherwise business as usual
          if ((nHeight - lastHeight) < epochInterval) {
              // LogPrintf("%s - using valid seed %s from height %d\n", __func__, lastSeed.ToString().c_str(), lastHeight);
              setSeed(lastSeed.ToString());
              return;
          }
       }
    }

    //! find last epoch height
    uint256 seedHash{0};
    char mixHash[64] = {0};
    int nPrevHeight = nHeight;
    bool is_epoch = (nHeight % epochInterval) == 0;
    if (!is_epoch) {
        while (nPrevHeight > epochInterval) {
           nPrevHeight--;
           if (nPrevHeight % epochInterval == 0)
               break;
        }
    }
    int nLowestBounds = nPrevHeight - epochInterval;
    LogPrintf("%s - last epoch was %d, lowest inputdata at %d\n", __func__, nPrevHeight, nLowestBounds);

    //! generate seed for last epoch
    for (int i=nLowestBounds; i<nPrevHeight; i++) {
         const uint256 *prevHash = chainActive[i]->phashBlock;
         LogPrintf("height %08d [%s]\n", i, prevHash->ToString().c_str());
         memcpy(mixHash+32,prevHash,32);
         SHA256((const unsigned char*)mixHash,64,(unsigned char*)&seedHash);
    }
    setSeedHash(seedHash.ToString());
    seedGenerationHeights.push_back(make_pair(nPrevHeight, seedHash));
    seedStoreWrite(nPrevHeight, thisSeed);
    LogPrintf("%s - new   seedhash (height %08d / %s)\n", __func__, nPrevHeight, seedHash.ToString().c_str());
}

