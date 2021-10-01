#include <iostream>
#include "multisweeper.hpp"
#include "wallet2.h"
#include "mnemonics/electrum-words.h"
#include "wallet/monero_wallet_full.h"
#include "utils/monero_utils.h"

using namespace std;


int main(int argc, const char** argv) {
  std::vector<std::string> args_vect(argv+1, argv + argc);
  std::string host, receiving_addr;
  if (!process_args(args_vect, host, receiving_addr))
    return 1;
  
  std::vector<crypto::secret_key> keys = get_keys_from_args(args_vect);
  
  std::vector<std::string> files_created;
  for (size_t i = 0; i < keys.size(); i++) {
    crypto::secret_key sec_spendkey = keys[i];
    if (sec_spendkey == crypto::null_skey)
      return 1;   
    crypto::public_key pub_spendkey;
    if(!crypto::secret_key_to_public_key(sec_spendkey, pub_spendkey)) {
      cerr << "failed to calculate public spend key from private spend key" << endl;
      return 1;
    }
    
    crypto::secret_key sec_viewkey;
    crypto::hash_to_scalar(&sec_spendkey, sizeof(sec_spendkey), sec_viewkey);
    crypto::public_key pub_viewkey;
    if (!crypto::secret_key_to_public_key(sec_viewkey, pub_viewkey)) {
      cerr << "failed to calculate public view key from derived private view key" << endl;
      return 1;
    }
    
    cryptonote::account_public_address addr {pub_spendkey, pub_viewkey};
    string addr_string = cryptonote::get_account_address_as_str(cryptonote::network_type::MAINNET, false, addr);
    cout << "Loading wallet " << i+1 << endl;
    string wallet_file_name = "wallet" + to_string(i+1);
    files_created.push_back(wallet_file_name);
    // TODO: sync wallets in parrallel
    monero_wallet* wallet = monero_wallet_full::create_wallet_from_keys (
        wallet_file_name,
        "pwd",
        monero_network_type::MAINNET,
        addr_string,
        epee::string_tools::pod_to_hex(sec_viewkey),
        epee::string_tools::pod_to_hex(sec_spendkey), // spend key string
        monero_rpc_connection(            
          host,
          string(""),
          string("")), 0);
    cout << "sucessfully loaded wallet " << addr_string << " from spend key " << argv[i] << endl;
    struct : monero_wallet_listener {
      void on_sync_progress(uint64_t height, uint64_t start_height, uint64_t end_height, double percent_done, const string& message) {
        cout << "height:" << height << " " << percent_done * 100 << " percent done" << endl;
      }
    } listener;
    wallet->sync(listener);
    wallet->remove_listener(listener);
    uint64_t balance = wallet->get_balance();
    if (balance == 0) {
      cout << "No balance found, moving on..." << endl;
      continue;
    }
    cout << "balance found: " << balance  << " sweeping..." << endl;
    monero_tx_config config;
    config.m_address = receiving_addr;
    config.m_relay = true;
    std::vector<std::shared_ptr<monero_tx_wallet>> txs = wallet->sweep_unlocked(config);
  }
  
  for (std::string file_name : files_created) {
    remove(file_name.c_str()); 
  }
  
  return 0;
}
