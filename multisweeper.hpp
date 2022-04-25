#pragma once

#include <string>
#include <vector>

#include "wallet2.h"
#include "wallet/monero_wallet.h"
#include "wallet/monero_wallet_full.h"
#include "daemon/monero_daemon_model.h"
#include "utils/monero_utils.h"

class Sweeper {
    public:
        Sweeper() = delete;
        Sweeper(const std::string &host, const std::string &receiving_addr, const std::vector<crypto::secret_key> &keys, unsigned thread_count = 1) : host_(host), receiving_addr_(receiving_addr), keys_(keys), thread_count_(thread_count) { /* TODO: adjust if unreasonable thread count requested */ }
        void sweep();

    private:
        /* split up the keys into n groups based on the order they are currently stored in
         * @param groups the number of groups you want, effectively result.size()
         * @return a 2d array of keys from keys_ with "groups" number of rows
         */
        std::vector<std::vector<crypto::secret_key>> partition_keyset(size_t groups);

        std::string host_, receiving_addr_;
        std::vector<crypto::secret_key> keys_;
        unsigned thread_count_;
};

bool scan_wallets(const std::string &host, const std::string &rec_addr, const std::vector<crypto::secret_key> &keys);
