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
#include "wallet2.h"
#include "daemon/monero_daemon_model.h"
#include "wallet/monero_wallet.h"
#include "wallet/monero_wallet_full.h"
#include "utils/monero_utils.h"

using namespace std;

/*
 * hex_str_to_secret_key
 * convert a std::string to a crypto::secret_key
 * @param key - the key to convert in string form
 * @return the key reinterpreted as a crypto::secret_key
 * 
 */
crypto::secret_key hex_str_to_secret_key(std::string key) {
  cryptonote::blobdata buff;
  if(!epee::string_tools::parse_hexstr_to_binbuff(key, buff))
      cerr << "failed to parse " << key << " to type crypto::secret_key" << endl; 
    return *reinterpret_cast<const crypto::secret_key*>(buff.data());
}

/*
 * remove_comma
 * mutate string to remove (the first) comma
 * @param str - the string the modify
 * @return true on success false if fail (including no comma present)
 */
bool remove_comma(std::string& str) {
  std::string::size_type pos = str.find(',');
  if (pos == std::string::npos)
    return false;
  try {
    str.erase(pos);
  } catch (const exception& e) {
    return false;
  }
  return true;
}

/* isValidHost - idk why i like this better than underscores >_<
 * check if this looks sorta like a proper host name with port
 * @param host name with port of monero node to connect to (eg 127.0.0.1:18081)
 * @return True conforms to convention (has ':' and '.') else False
 */
bool isValidHost(std::string host) {
  unsigned int period_count = 0;
  for(size_t pos = host.find('.'); pos != std::string::npos; pos = host.find('.')) {
    period_count++;
    try {
      host.erase(0, pos + 1);	
    } catch (const std::out_of_range& oor) {
      break;
    } catch (const exception e) {
      return false;
    }
  }
  if (period_count == 3)
    return host.find(':') != std::string::npos;
  return true;
}

/* 
 * process_args 
 * extract host and receiving_addr address from main arguments, erases them from argument vector
 * @param args - should be argv from main
 * @param host - the monero node to connect to formatted like 127.0.0.1:18081
 * @param receiving_addr - the address which will collect all the funds
 * @return True on success False if error
 */
bool process_args(std::vector<std::string>& args, std::string& host, std::string& receiving_addr) {
  const string usage = "usage: multisweeper [host], [Receiving address], [privatekey1], (privatekey2), (privatekeyn) ex: ./multisweeper 127.0.0.1 46YCMVGbdnfCoA7PammbanBtyaTzhLzgF3bEJAeybFQZ4aTZ4cu76KHKXe4Mft9yaodUBzs3HQpW23qmiJB2NZ714snYrAV, boss wedge inbound avoid enforce smelting entrance much salads icon yeti volcano nightly seismic gymnast avoid dewdrop deepest shipped nitrogen pizza ointment acoustic adventure adventure, 3dc0c516976cf04e8d71c24ef6a6e012e8489379c1e8b6b7077e9fdd805e6202";
  
  if (args.size() < 3) {
    cout << usage << endl;
    return false;
  }
  
  remove_comma(args[0]);;
  if(!isValidHost(args[0])) {
    cerr << "invalid host" << endl;
    return false;
  }
  args.erase(args.begin());
  remove_comma(args[0]);
  if (!monero_utils::is_valid_address(args[0], monero_network_type::MAINNET)) {
    cerr << "invalid address" << endl;
    cout << usage << endl;
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
