/*
 * 
 * MIT LICENSE
 * Copyright (c) Benjamin Evanoff
 * evanoff3@illinois.edu
 * https://github.com/benevanoff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * Have a bunch of old Monero wallets that may or may not hold a balance? This tool is probably for you
 * This program takes a list of Monero secret spend keys either as a hexadecimal or 25 word mnemonic
 * and will then sweep the balances of each of those wallets to one new wallet of your choice.
*/

#include <iostream>
#include "multisweeper.hpp"
#include "mnemonics/electrum-words.h"

std::vector<std::vector<crypto::secret_key>> Sweeper::partition_keyset(size_t groups) {
  std::vector<std::vector<crypto::secret_key>> result;
  if (groups < 2) {
    result.push_back(keys_);
    return result;
  }

  size_t i = 0;
  while (i < keys_.size()) {
    size_t start = i;
    size_t end = i+(groups-1) < keys_.size() ? i+(groups-1) : keys_.size()-1;
    std::vector<crypto::secret_key> row = std::vector<crypto::secret_key>(keys_.begin()+start, keys_.begin()+end);
    result.push_back(row);
    i = end+1;
  }
  return result;
}

bool scan_wallets(const std::string &host, const std::string &rec_addr, const std::vector<crypto::secret_key> &keys) {
std::vector<std::string> files_created;
  for (size_t i = 0; i < keys.size(); i++) {
    crypto::secret_key sec_spendkey = keys[i];
    if (sec_spendkey == crypto::null_skey)
      return false;   
    crypto::public_key pub_spendkey;
    if(!crypto::secret_key_to_public_key(sec_spendkey, pub_spendkey)) {
      std::cerr << "failed to calculate public spend key from private spend key" << std::endl;
      return false;
    }
    
    crypto::secret_key sec_viewkey;
    crypto::hash_to_scalar(&sec_spendkey, sizeof(sec_spendkey), sec_viewkey);
    crypto::public_key pub_viewkey;
    if (!crypto::secret_key_to_public_key(sec_viewkey, pub_viewkey)) {
      std::cerr << "failed to calculate public view key from derived private view key" << std::endl;
      return false;
    }
    
    cryptonote::account_public_address addr {pub_spendkey, pub_viewkey};
    std::string addr_string = cryptonote::get_account_address_as_str(cryptonote::network_type::MAINNET, false, addr);
    std::cout << "Loading wallet " << i+1 << std::endl;
    std::string wallet_file_name = "wallet" + std::to_string(i+1);
    files_created.push_back(wallet_file_name);
    std::unique_ptr<monero_wallet_full> wallet( monero_wallet_full::create_wallet_from_keys (
        wallet_file_name,
        "pwd",
        monero_network_type::MAINNET,
        addr_string,
        epee::string_tools::pod_to_hex(sec_viewkey),
        epee::string_tools::pod_to_hex(sec_spendkey), // spend key string
        monero_rpc_connection(            
          host,
          std::string(""),
          std::string("")), 0)
    );
    std::cout << "sucessfully loaded wallet " << addr_string << " from spend key " << keys[i] << std::endl;
    struct : monero_wallet_listener {
      void on_sync_progress(uint64_t height, uint64_t start_height, uint64_t end_height, double percent_done, const std::string &message) {
        std::cout << "height:" << height << " " << percent_done * 100 << " percent done" << std::endl;
      }
    } listener;
    wallet->sync(listener);
    wallet->remove_listener(listener);
    uint64_t balance = wallet->get_balance();
    if (balance == 0) {
      std::cout << "No balance found, moving on..." << std::endl;
      continue;
    }
    std::cout << "balance found: " << balance  << " sweeping..." << std::endl;
    monero_tx_config config;
    config.m_address = rec_addr;
    config.m_relay = true;
    std::vector<std::shared_ptr<monero_tx_wallet>> txs = wallet->sweep_unlocked(config);
  }
  
  for (std::string file_name : files_created) {
    remove(file_name.c_str()); 
  }
  return true;
}

void Sweeper::sweep() {
  if (thread_count_ > 1) {
    auto key_groups = partition_keyset(thread_count_);
    std::vector<std::thread*> threads;
    for (size_t i = 0; i < thread_count_; i++) {
      std::thread worker(scan_wallets, host_, receiving_addr_, key_groups[i]);
      threads.push_back(&worker);
    }
    for (std::thread* t : threads) { // wait for threads to finish
      t->join();
    }
  } else {
    scan_wallets(host_, receiving_addr_, keys_);
  }
}
