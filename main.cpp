#include <iostream>
#include "wallet2.h"
#include "wallet/monero_wallet_full.h"
#include "utils/monero_utils.h"

using namespace std;

crypto::secret_key hex_str_to_secret_key(std::string key) {
  cryptonote::blobdata buff;
  if(!epee::string_tools::parse_hexstr_to_binbuff(key, buff))
      cerr << "failed to parse " << key << " to type crypto::secret_key" << endl; 
    return *reinterpret_cast<const crypto::secret_key*>(buff.data());
}

int main(int argc, const char** argv) {
  const string usage = "usage: multisweeper [Receiving address] [privatekey1] (privatekey2) (privatekeyn)";
  if (argc < 2) {
    cout << usage << endl;
    return 1;
  }
  if (!monero_utils::is_valid_address(argv[1], monero_network_type::MAINNET)) {
    cerr << "invalid address" << endl;
    cout << usage << endl;
    return 1;
  }
  string receiving_addr = argv[1];
  
  for (int i = 2; i < argc; i++) {
    crypto::secret_key sec_spendkey = hex_str_to_secret_key(argv[i]);
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
    cout << "Loading wallet " << i-1 << endl;
    string wallet_file_name = "wallet" + to_string(i-1);
    // TODO: sync wallets in parrallel
    monero_wallet* wallet = monero_wallet_full::create_wallet_from_keys (
        wallet_file_name,
        "pwd",
        monero_network_type::MAINNET,
        addr_string,
        epee::string_tools::pod_to_hex(sec_viewkey),
        argv[i], // spend key string
        monero_rpc_connection(            
          string("http://uwillrunanodesoon.moneroworld.com:18089"),
          string(""),
          string("")));
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
  return 0;
}
