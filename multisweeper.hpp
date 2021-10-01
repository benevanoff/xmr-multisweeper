#ifndef MULTISWEEPER_HPP
#define MULTISWEEPER_HPP

#include <string>
#include <vector>

#include "wallet2.h"
#include "wallet/monero_wallet_full.h"
#include "daemon/monero_daemon_model.h"
#include "utils/monero_utils.h"


bool remove_comma(std::string& str);
bool isValidHost(std::string host);

std::vector<crypto::secret_key> get_keys_from_args(const std::vector<std::string>& args);
bool process_args(std::vector<std::string>& args, std::string& host, std::string& receiving_addr);

#endif
