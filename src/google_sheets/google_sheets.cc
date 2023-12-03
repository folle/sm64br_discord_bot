#include "google_sheets/google_sheets.h"

#include <chrono>
#include <boost/algorithm/string.hpp>
#include <cpprest/http_client.h>
#include <jwt-cpp/jwt.h>


GoogleSheets::GoogleSheets(std::shared_ptr<Database> database) :
  database_(std::move(database)) {
}

#ifdef _WIN32
[[nodiscard]] bool GoogleSheets::GetBearerAccessToken(std::wstring& access_token) const noexcept {
#else
[[nodiscard]] bool GoogleSheets::GetBearerAccessToken(std::string & access_token) const noexcept {
#endif
  access_token.clear();

  const auto current_time = std::chrono::system_clock::now();
  constexpr auto kAssertionExpirationLength = std::chrono::seconds(30);
  const auto jwt_token = jwt::create().set_type("JWT")
    .set_key_id(database_->GetGooglePrivateKeyId())
    .set_issuer(database_->GetGoogleClientEmail())
    .set_payload_claim("scope", jwt::claim(std::string("https://www.googleapis.com/auth/spreadsheets")))
    .set_audience("https://oauth2.googleapis.com/token")
    .set_expires_at(current_time + kAssertionExpirationLength)
    .set_issued_at(current_time)
    .sign(jwt::algorithm::rs256("", database_->GetGooglePrivateKey()));

#ifdef _WIN32
  web::json::value jwt_token_post_data;
  jwt_token_post_data[L"grant_type"] = web::json::value::string(L"urn:ietf:params:oauth:grant-type:jwt-bearer");
  jwt_token_post_data[L"assertion"] = web::json::value::string(std::wstring(jwt_token.begin(), jwt_token.end()));

  auto jwt_http_client = web::http::client::http_client(L"https://oauth2.googleapis.com/token");
  const auto jwt_request = jwt_http_client.request(web::http::methods::POST, L"", jwt_token_post_data).get();
  if (web::http::status_codes::OK != jwt_request.status_code()) {
    logger_->error("Access token POST request failed with status code '{}'", jwt_request.status_code());
    return false;
  }

  constexpr auto kExpiresInField = L"expires_in";
  constexpr auto kTokenTypeField = L"token_type";
  constexpr auto kAccessTokenField = L"access_token";
#else
  web::json::value jwt_token_post_data;
  jwt_token_post_data["grant_type"] = web::json::value::string("urn:ietf:params:oauth:grant-type:jwt-bearer");
  jwt_token_post_data["assertion"] = web::json::value::string(jwt_token);

  auto jwt_http_client = web::http::client::http_client("https://oauth2.googleapis.com/token");
  const auto jwt_request = jwt_http_client.request(web::http::methods::POST, "", jwt_token_post_data).get();
  if (web::http::status_codes::OK != jwt_request.status_code()) {
    logger_->error("Access token POST request failed with status code '{}'", jwt_request.status_code());
    return false;
  }

  constexpr auto kExpiresInField = "expires_in";
  constexpr auto kTokenTypeField = "token_type";
  constexpr auto kAccessTokenField = "access_token";
#endif

  const auto access_token_json = jwt_request.extract_json().get();
  if (!access_token_json.has_integer_field(kExpiresInField)) {
    logger_->error("Access token json doesn't have expiration field");
    return false;
  }

  const auto expiration_time_remaining_in_seconds = access_token_json.at(kExpiresInField).as_integer();
  if (0 >= expiration_time_remaining_in_seconds) {
    logger_->error("Access token is already expired");
    return false;
  }

  if (!access_token_json.has_string_field(kTokenTypeField)) {
    logger_->error("Access token json doesn't have token type field");
    return false;
  }

  const auto token_type = access_token_json.at(kTokenTypeField).as_string();
  if (!boost::iequals(L"Bearer", token_type)) {
    logger_->error("Access token type is not Bearer");
    return false;
  }

  if (!access_token_json.has_string_field(kAccessTokenField)) {
    logger_->error("Access token json doesn't have token field");
    return false;
  }

  access_token = access_token_json.at(kAccessTokenField).as_string();
  if (access_token.empty()) {
    logger_->error("Access token is empty");
    return false;
  }

  return true;
}

//{
//  web::http::client::http_client sheets_http_client(L"https://sheets.googleapis.com?access_token=1q");
//  auto s= client2.request(web::http::methods::DEL).get().extract_json().get().to_string();
//  auto b = 2;
//
//   spreadsheet id leader board 1w4N1f4pVFcal7X4AremQua1fuXNcidXFzAPu_Uxp0EA
//}