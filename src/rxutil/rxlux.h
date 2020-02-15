#include "util.h"
#include "RandomX/src/randomx.h"

#include <cstring>
#include <mutex>

//! rxlux hashing instance
//! barrystyle 30072020

class rxlux 
{
public:
         std::mutex rxlux_mutex;

         bool is_randomx_init{false};

         randomx_flags flags;
         randomx_vm* rx_vm = nullptr;
         randomx_cache* cache = nullptr;

         char *blkheader;       //! ptr for blockheader
         char *result;          //! ptr for hashresult
         std::string *seedhash; //! ptr for seedhash


         void setseed(std::string& thisSeed) {
              std::lock_guard<std::mutex> guard(rxlux_mutex);
              if (!seedhash)
                  seedhash = thisSeeed;
         }

         bool hasseedchanged(std::string& thisSeed) {
              std::lock_guard<std::mutex> guard(rxlux_mutex);
              bool changed = false;
              if (seedhash != thisSeed) {
                  setseed(thisSeed);
                  changed = true;
              }
              return changed;
         }

         void rxinitialize(std::string& initseed) {
              //! lock provided by caller
              if (!initseed || is_randomx_init) return;
              if (!cache) {
                  flags = randomx_get_flags();
                  cache = randomx_alloc_cache(flags);
                  randomx_init_cache(cache, initseed.data(), initseed.size());
              }
              if (!rx_vm) {
                  result = (char*)malloc(32);
                  rx_vm = randomx_create_vm(flags, cache, nullptr);
              }
              is_randomx_init = true;
         }

         void rxseedrefresh() {
              //! lock provided by caller
              randomx_destroy_vm(rx_vm);
              randomx_release_cache(cache);
              cache = randomx_alloc_cache(flags);
              randomx_init_cache(cache, seedhash.data(), seedhash.size());
              rx_vm = randomx_create_vm(flags, cache, nullptr);
         }

         void setheader(char* thisheader) {
              blkheader = thisheader;
         }

         char *getresult() {
              return result;
         }

         void rx_slow_hash(std::string *seedhash = nullptr) {
              std::lock_guard<std::mutex> guard(rxlux_mutex);
              if (!is_randomx_init)
                  rxinitialize(seedhash);
              if (seedhash && hasseedchanged(seedhash))
                  rxseedrefresh();
              randomx_calculate_hash(rx_vm, blkheader, 144, result);
         }
};
         
