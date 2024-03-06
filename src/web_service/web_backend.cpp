// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstdlib>
#include <mutex>
#include <string>
#include <LUrlParser.h>
#include <httplib.h>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/web_result.h"
#include "core/settings.h"
#include "web_service/web_backend.h"

namespace WebService {

constexpr std::array<const char, 1> API_VERSION{'1'};

constexpr u32 HTTP_PORT = 80;
constexpr u32 HTTPS_PORT = 443;

constexpr u32 TIMEOUT_SECONDS = 30;

struct Client::Impl {
    Impl(std::string host, std::string username, std::string token)
        : host{std::move(host)}, username{std::move(username)}, token{std::move(token)} {
        std::lock_guard<std::mutex> lock(jwt_cache.mutex);
        if (this->username == jwt_cache.username && this->token == jwt_cache.token) {
            jwt = jwt_cache.jwt;
        }
    }

    /// A generic function handles POST, GET and DELETE request together
    Common::WebResult GenericJson(const std::string& method, const std::string& path,
                                  const std::string& data, bool allow_anonymous) {
        if (jwt.empty()) {
            UpdateJWT();
        }

        if (jwt.empty() && !allow_anonymous) {
            LOG_ERROR(WebService, "Credentials must be provided for authenticated requests");
            return Common::WebResult{Common::WebResult::Code::CredentialsMissing,
                                     "Credentials needed"};
        }

        auto result = GenericJson(method, path, data, jwt);
        if (result.result_string == "401") {
            // Try again with new JWT
            UpdateJWT();
            result = GenericJson(method, path, data, jwt);
        }

        return result;
    }

    /**
     * A generic function with explicit authentication method specified
     * JWT is used if the jwt parameter is not empty
     * username + token is used if jwt is empty but username and token are not empty
     * anonymous if all of jwt, username and token are empty
     */
    Common::WebResult GenericJson(const std::string& method, const std::string& path,
                                  const std::string& data, const std::string& jwt = "",
                                  const std::string& username = "", const std::string& token = "") {
        if (cli == nullptr) {
            auto parsedUrl = LUrlParser::clParseURL::ParseURL(host);
            int port;
            if (parsedUrl.m_Scheme == "http") {
                if (!parsedUrl.GetPort(&port)) {
                    port = HTTP_PORT;
                }
                cli = std::make_unique<httplib::Client>(parsedUrl.m_Host.c_str(), port,
                                                        TIMEOUT_SECONDS);
            } else if (parsedUrl.m_Scheme == "https") {
                if (!parsedUrl.GetPort(&port)) {
                    port = HTTPS_PORT;
                }
                cli = std::make_unique<httplib::SSLClient>(parsedUrl.m_Host.c_str(), port,
                                                           TIMEOUT_SECONDS);
            } else {
                LOG_ERROR(WebService, "Bad URL scheme {}", parsedUrl.m_Scheme);
                return Common::WebResult{Common::WebResult::Code::InvalidURL, "Bad URL scheme"};
            }
        }
        if (cli == nullptr) {
            LOG_ERROR(WebService, "Invalid URL {}", host + path);
            return Common::WebResult{Common::WebResult::Code::InvalidURL, "Invalid URL"};
        }

        httplib::Headers params;
        if (!jwt.empty()) {
            params = {
                {std::string("Authorization"), fmt::format("Bearer {}", jwt)},
            };
        } else if (!username.empty()) {
            params = {
                {std::string("x-username"), username},
                {std::string("x-token"), token},
            };
        }

        params.emplace(std::string("api-version"),
                       std::string(API_VERSION.begin(), API_VERSION.end()));
        if (method != "GET") {
            params.emplace(std::string("Content-Type"), std::string("application/json"));
        };

        httplib::Request request;
        request.method = method;
        request.path = path;
        request.headers = params;
        request.body = data;

        httplib::Response response;

        if (!cli->send(request, response)) {
            LOG_ERROR(WebService, "{} to {} returned null", method, host + path);
            return Common::WebResult{Common::WebResult::Code::LibError, "Null response"};
        }

        if (response.status >= 400) {
            LOG_ERROR(WebService, "{} to {} returned error status code: {}", method, host + path,
                      response.status);
            return Common::WebResult{Common::WebResult::Code::HttpError,
                                     std::to_string(response.status)};
        }

        auto content_type = response.headers.find("content-type");

        if (content_type == response.headers.end()) {
            LOG_ERROR(WebService, "{} to {} returned no content", method, host + path);
            return Common::WebResult{Common::WebResult::Code::WrongContent, ""};
        }

        if (content_type->second.find("application/json") == std::string::npos &&
            content_type->second.find("text/html; charset=utf-8") == std::string::npos) {
            LOG_ERROR(WebService, "{} to {} returned wrong content: {}", method, host + path,
                      content_type->second);
            return Common::WebResult{Common::WebResult::Code::WrongContent, "Wrong content"};
        }
        return Common::WebResult{Common::WebResult::Code::Success, "", response.body};
    }

    // Retrieve a new JWT from given username and token
    void UpdateJWT() {
        if (username.empty() || token.empty()) {
            return;
        }

        auto result = GenericJson("POST", "/jwt/internal", "", "", username, token);
        if (result.result_code != Common::WebResult::Code::Success) {
            LOG_ERROR(WebService, "UpdateJWT failed");
        } else {
            std::lock_guard<std::mutex> lock(jwt_cache.mutex);
            jwt_cache.username = username;
            jwt_cache.token = token;
            jwt_cache.jwt = jwt = result.returned_data;
        }
    }

    std::string host;
    std::string username;
    std::string token;
    std::string jwt;
    std::unique_ptr<httplib::Client> cli;

    struct JWTCache {
        std::mutex mutex;
        std::string username;
        std::string token;
        std::string jwt;
    };
    static inline JWTCache jwt_cache;
};

Client::Client(std::string host, std::string username, std::string token)
    : impl{std::make_unique<Impl>(std::move(host), std::move(username), std::move(token))} {}

Client::~Client() = default;

Common::WebResult Client::PostJson(const std::string& path, const std::string& data,
                                   bool allow_anonymous) {
    return impl->GenericJson("POST", path, data, allow_anonymous);
}

Common::WebResult Client::GetJson(const std::string& path, bool allow_anonymous) {
    return impl->GenericJson("GET", path, "", allow_anonymous);
}

Common::WebResult Client::DeleteJson(const std::string& path, const std::string& data,
                                     bool allow_anonymous) {
    return impl->GenericJson("DELETE", path, data, allow_anonymous);
}

} // namespace WebService
