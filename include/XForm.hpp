#pragma once

#include <string>
#include <unordered_map>

/**
 * @brief Decode a URL string
 *
 * @param src the URL string
 * @return the decoded string
 */
std::string UrlDecode(const std::string &src) {
  std::string ret;
  char ch;
  for (size_t i = 0; i < src.length(); i++) {
    if (src[i] == '%') {
      int ascii;
      sscanf(src.substr(i + 1, 2).c_str(), "%x", &ascii);
      ch = static_cast<char>(ascii);
      ret += ch;
      i = i + 2;
    } else if (src[i] == '+') {
      ret += ' ';
    } else {
      ret += src[i];
    }
  }
  return (ret);
}

/**
 * @brief Docode a x-www-form-urlencoded string
 *
 * @param src the form string
 * @return the form
 */
auto DecodeXWWWFormUrlencoded(std::string &src) {
  std::unordered_map<std::string, std::string> ret;
  auto delimiter = src.find_first_of('=');
  while (delimiter != std::string::npos) {
    auto delimiter_2 = src.find_first_of('&');
    if (delimiter_2 == std::string::npos) {  // the end of the form
      ret[UrlDecode(src.substr(0, delimiter))] =
          UrlDecode(src.substr(delimiter + 1));
      return ret;
    }
    ret[UrlDecode(src.substr(0, delimiter))] =
        UrlDecode(src.substr(delimiter + 1, delimiter_2 - delimiter - 1));
    src = src.substr(delimiter_2 + 1);
    delimiter = src.find_first_of('=');
  }
  return ret;
}
