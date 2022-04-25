#pragma once

#include <string>
#include <vector>

/*
 * hex_str_to_secret_key
 * convert a std::string to a crypto::secret_key
 * @param key - the key to convert in string form
 * @return the key reinterpreted as a crypto::secret_key
 * 
 */
crypto::secret_key hex_str_to_secret_key(const std::string &key) {
  cryptonote::blobdata buff;
  if(!epee::string_tools::parse_hexstr_to_binbuff(key, buff))
      std::cerr << "failed to parse " << key << " to type crypto::secret_key" << std::endl; 
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
  } catch (const std::exception &e) {
    return false;
  }
  return true;
}

/* isValidHost
 * check if this looks sorta like a proper host name with port
 * @param host name with port of monero node to connect to (eg 127.0.0.1:18081)
 * @return True conforms to convention (has ':' and '.') else False
 */
bool isValidHost(std::string host) {
  return host.find('.') != std::string::npos && host.find(':') != std::string::npos;
}