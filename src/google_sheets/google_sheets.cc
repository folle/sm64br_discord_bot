#include "google_sheets/google_sheets.h"

#include <fstream>
#include <nlohmann/json.hpp>


GoogleSheets::GoogleSheets(const std::string& client_secret_file_path) {
  std::ifstream client_secret_file(client_secret_file_path);
  const auto client_secret_json = nlohmann::json::parse(client_secret_file);

  const auto installed_application = client_secret_json["installed"];
  const auto client_id = installed_application["client_id"].template get<std::string>();
  const auto client_secret = installed_application["client_secret"].template get<std::string>();
  const auto auth_uri = installed_application["auth_uri"].template get<std::string>();
  const auto token_uri = installed_application["token_uri"].template get<std::string>();
  std::vector<std::string> redirect_uris = installed_application["redirect_uris"].template get<std::vector<std::string>>();
}