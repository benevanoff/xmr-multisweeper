#include <iostream>
#include <thread>
#include <memory>
#include <stdexcept>
#include "multisweeper.hpp"
#include "sweeper_utils.hpp"
#include "mnemonics/electrum-words.h"

/* 
 * process_args 
 * extract host and receiving_addr address from main arguments, erases them from argument vector
 * @param args - should be argv from main
 * @param host - the monero node to connect to formatted like 127.0.0.1:18081
 * @param receiving_addr - the address which will collect all the funds
 * @param thread_count the number of concurrent threads to use
 * @return True on success False if error
 */
bool process_args(std::vector<std::string>& args, std::string& host, std::string& receiving_addr, size_t &thread_count) {
  const std::string usage = "usage: multisweeper (-j n) [host] [Receiving address] [privatekey1], (privatekey2), (privatekeyn) ex: ./multisweeper 127.0.0.1 46YCMVGbdnfCoA7PammbanBtyaTzhLzgF3bEJAeybFQZ4aTZ4cu76KHKXe4Mft9yaodUBzs3HQpW23qmiJB2NZ714snYrAV boss wedge inbound avoid enforce smelting entrance much salads icon yeti volcano nightly seismic gymnast avoid dewdrop deepest shipped nitrogen pizza ointment acoustic adventure adventure, 3dc0c516976cf04e8d71c24ef6a6e012e8489379c1e8b6b7077e9fdd805e6202";
  
  if (args.size() < 3) {
    std::cout << usage << std::endl;
    return false;
  }
  
  if (args[0] == "-j") {
    args.erase(args.begin());
    thread_count = std::stoi(args[0]);
    args.erase(args.begin());
  }

  remove_comma(args[0]);
  if(!isValidHost(args[0])) {
    std::cerr << "invalid host" << std::endl;
    return false;
  }
  host = args[0];
  args.erase(args.begin());
  remove_comma(args[0]);
  if (!monero_utils::is_valid_address(args[0], monero_network_type::MAINNET)) {
    std::cerr << "invalid address" << std::endl;
    std::cout << usage << std::endl;
    return false;
  }
  receiving_addr = args[0];
  args.erase(args.begin());
  return true;
}

/*
 * get_keys_from_args
 * take vector of strings from main argv which should
 * encode secret spend keys either as hexadecimal or a 25 word mnemonic
 * @param args - is meant to be a subset of argv from main function through which
 * to search for private keys delimited by comma (,)
 * @return vector of secret keys passed as argument but now as crypto::secret_key
 */
std::vector<crypto::secret_key> get_keys_from_args(const std::vector<std::string>& args) {
  std::vector<crypto::secret_key> keys;
  epee::wipeable_string current_mnemonic = "";
  size_t current_mnemonic_word_count = 0;
  for (std::string entry : args) {
    bool delimiter_found = entry.find(',') != std::string::npos;
    size_t entry_size = entry.size();
    if (delimiter_found)
      entry.erase(entry_size - 1);
    
    if (entry.size() == 64) // 64 hex characters and a comma  
	  keys.push_back(hex_str_to_secret_key(entry));
	else { // process 25 word mnemonic seed
      current_mnemonic += entry;
      current_mnemonic_word_count++;
      if (current_mnemonic_word_count < 25)
        current_mnemonic += " ";
      if (delimiter_found || current_mnemonic_word_count == 25) {
        crypto::secret_key key;
        std::string language;
        if (!crypto::ElectrumWords::words_to_bytes(current_mnemonic, key, language))
          throw std::runtime_error("Failed to decode mnemonic seed");
        keys.push_back(key);
      }
	}
  }
  return keys;	
}


int main(int argc, const char** argv) {
  std::vector<std::string> args_vect(argv+1, argv + argc);
  std::string host, receiving_addr;
  size_t thread_count = 1;
  if (!process_args(args_vect, host, receiving_addr, thread_count))
    return 1;

  std::vector<crypto::secret_key> keys = get_keys_from_args(args_vect);
  
  Sweeper sweeper(host, receiving_addr, keys, thread_count);
  sweeper.sweep();
  
  return 0;
}
