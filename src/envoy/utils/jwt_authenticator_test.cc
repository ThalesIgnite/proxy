/* Copyright 2017 Istio Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/envoy/utils/jwt_authenticator.h"
#include "common/http/message_impl.h"
#include "common/json/json_loader.h"
#include "gtest/gtest.h"
#include "test/mocks/upstream/mocks.h"
#include "test/test_common/utility.h"

using ::envoy::config::filter::http::common::v1alpha::JwtVerificationRule;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::_;

namespace Envoy {
namespace Utils {
namespace Jwt {
namespace {

// RS256 private key
//-----BEGIN PRIVATE KEY-----
//    MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQC6n3u6qsX0xY49
//    o+TBJoF64A8s6v0UpxpYZ1UQbNDh/dmrlYpVmjDH1MIHGYiY0nWqZSLXekHyi3Az
//    +XmV9jUAUEzFVtAJRee0ui+ENqJK9injAYOMXNCJgD6lSryHoxRkGeGV5iuRTteU
//    IHA1XI3yo0ySksDsoVljP7jzoadXY0gknH/gEZrcd0rBAbGLa2O5CxC9qjlbjGZJ
//    VpoRaikHAzLZCaWFIVC49SlNrLBOpRxSr/pJ8AeFnggNr8XER3ZzbPyAUa1+y31x
//    jeVFh/5z9l1uhjeao31K7f6PfPmvZIdaWEH8s0CPJaUEay9sY+VOoPOJhDBk3hoa
//    ypUpBv1XAgMBAAECggEAc5HaJJIm/trsqD17pyV6X6arnyxyx7xn80Eii4ZnoNv8
//    VWbJARP4i3e1JIJqdgE3PutctUYP2u0A8h7XbcfHsMcJk9ecA3IX+HKohF71CCkD
//    bYH9fgnoVo5lvSTYNcMHGKpyacrdRiImHKQt+M21VgJMpCRfdurAmVbX6YA9Sj6w
//    SBFrZbWkBHiHg7w++xKr+VeTHW/8fXI5bvSPAm/XB6dDKAcSXYiJJJhIoaVR9cHn
//    1ePRDLpEwfDpBHeepd/S3qR37mIbHmo8SVytDY2xTUaIoaRfXRWGMYSyxl0y4RsZ
//    Vo6Tp9Tj2fyohvB/S+lE34zhxnsHToK2JZvPeoyHCQKBgQDyEcjaUZiPdx7K63CT
//    d57QNYC6DTjtKWnfO2q/vAVyAPwS30NcVuXj3/1yc0L+eExpctn8tcLfvDi1xZPY
//    dW2L3SZKgRJXL+JHTCEkP8To/qNLhBqitcKYwp0gtpoZbUjZdZwn18QJx7Mw/nFC
//    lJhSYRl+FjVolY3qBaS6eD7imwKBgQDFXNmeAV5FFF0FqGRsLYl0hhXTR6Hi/hKQ
//    OyRALBW9LUKbsazwWEFGRlqbEWd1OcOF5SSV4d3u7wLQRTDeNELXUFvivok12GR3
//    gNl9nDJ5KKYGFmqxM0pzfbT5m3Lsrr2FTIq8gM9GBpQAOmzQIkEu62yELtt2rRf0
//    1pTh+UbN9QKBgF88kAEUySjofLzpFElwbpML+bE5MoRcHsMs5Tq6BopryMDEBgR2
//    S8vzfAtjPaBQQ//Yp9q8yAauTsF1Ek2/JXI5d68oSMb0l9nlIcTZMedZB3XWa4RI
//    bl8bciZEsSv/ywGDPASQ5xfR8bX85SKEw8jlWto4cprK/CJuRfj3BgaxAoGAAmQf
//    ltR5aejXP6xMmyrqEWlWdlrV0UQ2wVyWEdj24nXb6rr6V2caU1mi22IYmMj8X3Dp
//    Qo+b+rsWk6Ni9i436RfmJRcd3nMitHfxKp5r1h/x8vzuifsPGdsaCDQj7k4nqafF
//    vobo+/Y0cNREYTkpBQKBLBDNQ+DQ+3xmDV7RxskCgYBCo6u2b/DZWFLoq3VpAm8u
//    1ZgL8qxY/bbyA02IKF84QPFczDM5wiLjDGbGnOcIYYMvTHf1LJU4FozzYkB0GicX
//    Y0tBQIHaaLWbPk1RZdPfR9kAp16iwk8H+V4UVjLfsTP7ocEfNCzZztmds83h8mTL
//    DSwE5aY76Cs8XLcF/GNJRQ==
//-----END PRIVATE KEY-----

// A good public key
const std::string kPublicKey =
    "{\"keys\": [{"
    "  \"kty\": \"RSA\","
    "  \"n\": "
    "\"up97uqrF9MWOPaPkwSaBeuAPLOr9FKcaWGdVEGzQ4f3Zq5WKVZowx9TCBxmImNJ1q"
    "mUi13pB8otwM_l5lfY1AFBMxVbQCUXntLovhDaiSvYp4wGDjFzQiYA-pUq8h6MUZBnhleYrk"
    "U7XlCBwNVyN8qNMkpLA7KFZYz-486GnV2NIJJx_4BGa3HdKwQGxi2tjuQsQvao5W4xmSVaaE"
    "WopBwMy2QmlhSFQuPUpTaywTqUcUq_6SfAHhZ4IDa_FxEd2c2z8gFGtfst9cY3lRYf-c_Zdb"
    "oY3mqN9Su3-j3z5r2SHWlhB_LNAjyWlBGsvbGPlTqDziYQwZN4aGsqVKQb9Vw\","
    "  \"e\": \"AQAB\","
    "  \"alg\": \"RS256\","
    "  \"kid\": \"62a93512c9ee4c7f8067b5a216dade2763d32a47\""
    "},"
    "{"
    "  \"kty\": \"RSA\","
    "  \"n\": "
    "\"up97uqrF9MWOPaPkwSaBeuAPLOr9FKcaWGdVEGzQ4f3Zq5WKVZowx9TCBxmImNJ1q"
    "mUi13pB8otwM_l5lfY1AFBMxVbQCUXntLovhDaiSvYp4wGDjFzQiYA-pUq8h6MUZBnhleYrk"
    "U7XlCBwNVyN8qNMkpLA7KFZYz-486GnV2NIJJx_4BGa3HdKwQGxi2tjuQsQvao5W4xmSVaaE"
    "WopBwMy2QmlhSFQuPUpTaywTqUcUq_6SfAHhZ4IDa_FxEd2c2z8gFGtfst9cY3lRYf-c_Zdb"
    "oY3mqN9Su3-j3z5r2SHWlhB_LNAjyWlBGsvbGPlTqDziYQwZN4aGsqVKQb9Vw\","
    "  \"e\": \"AQAB\","
    "  \"alg\": \"RS256\","
    "  \"kid\": \"b3319a147514df7ee5e4bcdee51350cc890cc89e\""
    "}]}";

// A good JSON config.
const char kExampleConfig[] = R"(
{
     "issuer": "https://example.com",
     "audiences": [
        "example_service",
        "http://example_service1",
        "https://example_service2/"
      ],
      "remote_jwks": {
        "http_uri": {
          "uri": "https://pubkey_server/pubkey_path",
          "cluster": "pubkey_cluster"
        },
        "cache_duration": {
          "seconds": 600
        }
     },
}
)";

// A JSON config for "other_issuer"
const char kOtherIssuerConfig[] = R"(
{
   "issuer": "other_issuer"
}
)";

// expired token
// {"iss":"https://example.com","sub":"test@example.com","aud":"example_service","exp":1205005587}
const std::string kExpiredToken =
    "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJodHRwczovL2V4YW1wbGUu"
    "Y29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIsImV4cCI6MTIwNTAwNTU4NywiY"
    "XVkIjoiZXhhbXBsZV9zZXJ2aWNlIn0.izDa6aHNgbsbeRzucE0baXIP7SXOrgopYQ"
    "ALLFAsKq_N0GvOyqpAZA9nwCAhqCkeKWcL-9gbQe3XJa0KN3FPa2NbW4ChenIjmf2"
    "QYXOuOQaDu9QRTdHEY2Y4mRy6DiTZAsBHWGA71_cLX-rzTSO_8aC8eIqdHo898oJw"
    "3E8ISKdryYjayb9X3wtF6KLgNomoD9_nqtOkliuLElD8grO0qHKI1xQurGZNaoeyi"
    "V1AdwgX_5n3SmQTacVN0WcSgk6YJRZG6VE8PjxZP9bEameBmbSB0810giKRpdTU1-"
    "RJtjq6aCSTD4CYXtW38T5uko4V-S4zifK3BXeituUTebkgoA";

// A token with aud as invalid_service
// Payload:
// {"iss":"https://example.com","sub":"test@example.com","aud":"invalid_service","exp":2001001001}
const std::string kInvalidAudToken =
    "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJodHRwczovL2V4YW1wbGUu"
    "Y29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIsImV4cCI6MjAwMTAwMTAwMSwiY"
    "XVkIjoiaW52YWxpZF9zZXJ2aWNlIn0.B9HuVXpRDVYIvApfNQmE_l5fEMPEiPdi-s"
    "dKbTione8I_UsnYHccKZVegaF6f2uyWhAvaTPgaMosyDlJD6skadEcmZD0V4TzsYK"
    "v7eP5FQga26hZ1Kra7n9hAq4oFfH0J8aZLOvDV3tAgCNRXlh9h7QiBPeDNQlwztqE"
    "csyp1lHI3jdUhsn3InIn-vathdx4PWQWLVb-74vwsP-END-MGlOfu_TY5OZUeY-GB"
    "E4Wr06aOSU2XQjuNr6y2WJGMYFsKKWfF01kHSuyc9hjnq5UI19WrOM8s7LFP4w2iK"
    "WFIPUGmPy3aM0TiF2oFOuuMxdPR3HNdSG7EWWRwoXv7n__jA";

// Payload:
// {"iss":"https://example.com","sub":"test@example.com","aud":"example_service","exp":2001001001}
const std::string kGoodToken =
    "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJodHRwczovL2V4YW1wbGUu"
    "Y29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIsImV4cCI6MjAwMTAwMTAwMSwiY"
    "XVkIjoiZXhhbXBsZV9zZXJ2aWNlIn0.cuui_Syud76B0tqvjESE8IZbX7vzG6xA-M"
    "Daof1qEFNIoCFT_YQPkseLSUSR2Od3TJcNKk-dKjvUEL1JW3kGnyC1dBx4f3-Xxro"
    "yL23UbR2eS8TuxO9ZcNCGkjfvH5O4mDb6cVkFHRDEolGhA7XwNiuVgkGJ5Wkrvshi"
    "h6nqKXcPNaRx9lOaRWg2PkE6ySNoyju7rNfunXYtVxPuUIkl0KMq3WXWRb_cb8a_Z"
    "EprqSZUzi_ZzzYzqBNVhIJujcNWij7JRra2sXXiSAfKjtxHQoxrX8n4V1ySWJ3_1T"
    "H_cJcdfS_RKP7YgXRWC0L16PNF5K7iqRqmjKALNe83ZFnFIw";

// Payload:
// {"iss":"https://example.com","sub":"test@example.com","aud":"http://example_service/","exp":2001001001}
const std::string kGoodTokenAudHasProtocolScheme =
    "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJodHRwczovL2V4YW1wbGUu"
    "Y29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIsImV4cCI6MjAwMTAwMTAwMSwiY"
    "XVkIjoiaHR0cDovL2V4YW1wbGVfc2VydmljZS8ifQ.gHqO8m3hUZZ8m7EajMQy8vB"
    "RL5o3njwU5Pg2NxU4z3AwUP6P_7MoB_ChiByjg_LQ92GjHXbHn1gAQHVOn0hERVwm"
    "VYGmNsZHm4k5pmD6orPcYV1i3DdLqqxEVyw2R1XD8bC9zK7Tc8mKTRIJYC4T1QSo8"
    "mKTzZ8M-EwAuDYa0CsWGhIfA4o3xChXKPLM2hxA4uM1A6s4AQ4ipNQ5FTgLDabgsC"
    "EpfDR3lAXSaug1NE22zX_tm0d9JnC5ZrIk3kwmPJPrnAS2_9RKTQW2e2skpAT8dUV"
    "T5aSpQxJmWIkyp4PKWmH6h4H2INS7hWyASZdX4oW-R0PMy3FAd8D6Y8740A";

// Payload:
// {"iss":"https://example.com","sub":"test@example.com","aud":"https://example_service1/","exp":2001001001}
const std::string kGoodTokenAudService1 =
    "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJodHRwczovL2V4YW1wbGUu"
    "Y29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIsImV4cCI6MjAwMTAwMTAwMSwiY"
    "XVkIjoiaHR0cHM6Ly9leGFtcGxlX3NlcnZpY2UxLyJ9.JJq_-fzbNWykI2npW13hJ"
    "F_2_IK9JAlodt_T_kO_kSCb7ngAJvmbDhnIUKp7PX-UCEx_6sehNnLZzZeazGeDgw"
    "xcjI4zM7E1bzus_sY_Kl7MSYBx7UyW0rgbEvjJOg681Uwn8MkQh9wfQ-SuzPfe07Y"
    "O4bFMuNBiZsxS0j3_agJrbmpEPycNBSIZ0ez3aQpnDyUgZ1ZGBoVOgzXUJDXptb71"
    "nzvwse8DINafa5kOhBmQcrIADiOyTVC1IqcOvaftVcS4MTkTeCyzfsqcNQ-VeNPKY"
    "3e6wTe9brxbii-IPZFNY-1osQNnfCtYpEDjfvMjwHTielF-b55xq_tUwuqaaQ";

// Payload:
// {"iss":"https://example.com","sub":"test@example.com","aud":"http://example_service2","exp":2001001001}
const std::string kGoodTokenAudService2 =
    "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJodHRwczovL2V4YW1wbGUu"
    "Y29tIiwic3ViIjoidGVzdEBleGFtcGxlLmNvbSIsImV4cCI6MjAwMTAwMTAwMSwiY"
    "XVkIjoiaHR0cDovL2V4YW1wbGVfc2VydmljZTIifQ.XFPQHdA5A2rpoQgMMcCBRcW"
    "t8QrwVJAhdTgNqBjga_ebnoWZdzj9C6t-8mYYoCQ6t7bulLFbPzO8iJREo7zxN7Rn"
    "F0-15ur16LV7AYeDnH0istAiti9uy3POW3telcN374hbBVdA6sBafGqzeQ8cDpb4o"
    "0T_BIy6-kaz3ne4-UEdl8kLrR7UaA_LYrdXGomYKqwH3Q4q4mnV7mpE0YUm98AyI6"
    "Thwt7f3DTmHOMBeO_3xrLOOZgNtuXipqupkp9sb-DcCRdSokoFpGSTibvV_8RwkQo"
    "W2fdqw_ZD7WOe4sTcK27Uma9exclisHVxzJJbQOW82WdPQGicYaR_EajYzA";

}  // namespace

class MockJwtAuthenticatorCallbacks : public JwtAuthenticator::Callbacks {
 public:
  MOCK_METHOD1(onError, void(Status status));
  MOCK_METHOD2(onSuccess, void(const Jwt *jwt, const Http::LowerCaseString *header));
};

class JwtAuthenticatorTest : public ::testing::Test {
 public:
  void SetUp() { SetupConfig(kExampleConfig); }

  void SetupConfig(const std::string &json_str) {
    rules_.clear();
    JwtVerificationRule rule;
    google::protobuf::util::Status status =
        ::google::protobuf::util::JsonStringToMessage(json_str, &rule);
    ASSERT_TRUE(status.ok());
    rules_.push_back(rule);
    store_.reset(new JwtAuthStore(rules_));
    auth_.reset(new JwtAuthenticatorImpl(mock_cm_, *store_));
  }

  std::vector<JwtVerificationRule> rules_;
  std::unique_ptr<JwtAuthStore> store_;
  std::unique_ptr<JwtAuthenticator> auth_;
  NiceMock<Upstream::MockClusterManager> mock_cm_;
  MockJwtAuthenticatorCallbacks mock_cb_;
};

// A mock HTTP upstream with response body.
class MockUpstream {
 public:
  MockUpstream(Upstream::MockClusterManager &mock_cm,
               const std::string &response_body)
      : request_(&mock_cm.async_client_), response_body_(response_body) {
    ON_CALL(mock_cm.async_client_, send_(_, _, _))
        .WillByDefault(
            Invoke([this](Http::MessagePtr &, Http::AsyncClient::Callbacks &cb,
                          const absl::optional<std::chrono::milliseconds> &)
                       -> Http::AsyncClient::Request * {
              Http::MessagePtr response_message(new Http::ResponseMessageImpl(
                Http::HeaderMapPtr{new Http::TestHeaderMapImpl{{":status", "200"}}}));
              response_message->body().reset(
                  new Buffer::OwnedImpl(response_body_));
              cb.onSuccess(std::move(response_message));
              called_count_++;
              return &request_;
            }));
  }

  int called_count() const { return called_count_; }

 private:
  Http::MockAsyncClientRequest request_;
  std::string response_body_;
  int called_count_{};
};

TEST_F(JwtAuthenticatorTest, TestOkJWTandCache) {
  MockUpstream mock_pubkey(mock_cm_, kPublicKey);

  // Test OK pubkey and its cache
  for (int i = 0; i < 10; i++) {
    auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kGoodToken}};

    MockJwtAuthenticatorCallbacks mock_cb;
    EXPECT_CALL(mock_cb, onError(_)).Times(0);
    EXPECT_CALL(mock_cb, onSuccess(_,_)).WillOnce(Invoke([](const Jwt *jwt, const Http::LowerCaseString *header) {
      ASSERT_NE(jwt, nullptr);
      ASSERT_NE(header, nullptr);
      ASSERT_EQ(*header, Http::LowerCaseString("Authorization"));
    }));

    auth_->Verify(headers, &mock_cb);
  }

  EXPECT_EQ(mock_pubkey.called_count(), 1);
}

TEST_F(JwtAuthenticatorTest, TestOkJWTPubkeyNoAlg) {
  // Test OK pubkey with no "alg" claim.
  std::string alg_claim = "  \"alg\": \"RS256\",";
  std::string pubkey_no_alg = kPublicKey;
  std::size_t alg_pos = pubkey_no_alg.find(alg_claim);
  while (alg_pos != std::string::npos) {
    pubkey_no_alg.erase(alg_pos, alg_claim.length());
    alg_pos = pubkey_no_alg.find(alg_claim);
  }
  MockUpstream mock_pubkey(mock_cm_, pubkey_no_alg);

  auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kGoodToken}};

  MockJwtAuthenticatorCallbacks mock_cb;
  EXPECT_CALL(mock_cb, onError(_)).Times(0);
  EXPECT_CALL(mock_cb, onSuccess(_,_)).WillOnce(Invoke([](const Jwt *jwt, const Http::LowerCaseString *header) {
    ASSERT_NE(jwt, nullptr);
    ASSERT_NE(header, nullptr);
    ASSERT_EQ(*header, Http::LowerCaseString("Authorization"));
  }));

  auth_->Verify(headers, &mock_cb);

  EXPECT_EQ(mock_pubkey.called_count(), 1);
}

TEST_F(JwtAuthenticatorTest, TestOkJWTPubkeyNoKid) {
  // Test OK pubkey with no "kid" claim.
  std::string kid_claim1 =
      ",  \"kid\": \"62a93512c9ee4c7f8067b5a216dade2763d32a47\"";
  std::string kid_claim2 =
      ",  \"kid\": \"b3319a147514df7ee5e4bcdee51350cc890cc89e\"";
  std::string pubkey_no_kid = kPublicKey;
  std::size_t kid_pos = pubkey_no_kid.find(kid_claim1);
  pubkey_no_kid.erase(kid_pos, kid_claim1.length());
  kid_pos = pubkey_no_kid.find(kid_claim2);
  pubkey_no_kid.erase(kid_pos, kid_claim2.length());

  MockUpstream mock_pubkey(mock_cm_, pubkey_no_kid);

  auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kGoodToken}};

  MockJwtAuthenticatorCallbacks mock_cb;
  EXPECT_CALL(mock_cb, onError(_)).Times(0);
  EXPECT_CALL(mock_cb, onSuccess(_,_)).WillOnce(Invoke([](const Jwt *jwt, const Http::LowerCaseString *header) {
    ASSERT_NE(jwt, nullptr);
    ASSERT_NE(header, nullptr);
  }));

  auth_->Verify(headers, &mock_cb);

  EXPECT_EQ(mock_pubkey.called_count(), 1);
}

// Verifies that a JWT with aud: http://example_service/ is matched to
// example_service in config.
TEST_F(JwtAuthenticatorTest, TestOkJWTAudService) {
  MockUpstream mock_pubkey(mock_cm_, kPublicKey);

  // Test OK pubkey and its cache
  auto headers = Http::TestHeaderMapImpl{
      {"Authorization", "Bearer " + kGoodTokenAudHasProtocolScheme}};

  MockJwtAuthenticatorCallbacks mock_cb;
  EXPECT_CALL(mock_cb, onError(_)).Times(0);
  EXPECT_CALL(mock_cb, onSuccess(_,_)).WillOnce(Invoke([](const Jwt *jwt, const Http::LowerCaseString *header) {
    ASSERT_NE(jwt, nullptr);
    ASSERT_NE(header, nullptr);
  }));

  auth_->Verify(headers, &mock_cb);

  EXPECT_EQ(mock_pubkey.called_count(), 1);
}

// Verifies that a JWT with aud: https://example_service1/ is matched to
// a JWT with aud: http://example_service1 in config.
TEST_F(JwtAuthenticatorTest, TestOkJWTAudService1) {
  MockUpstream mock_pubkey(mock_cm_, kPublicKey);

  // Test OK pubkey and its cache
  auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kGoodTokenAudService1}};

  MockJwtAuthenticatorCallbacks mock_cb;
  EXPECT_CALL(mock_cb, onError(_)).Times(0);
  EXPECT_CALL(mock_cb, onSuccess(_,_)).WillOnce(Invoke([](const Jwt *jwt, const Http::LowerCaseString *header) {
    ASSERT_NE(jwt, nullptr);
    ASSERT_NE(header, nullptr);
  }));

  auth_->Verify(headers, &mock_cb);

  EXPECT_EQ(mock_pubkey.called_count(), 1);
}

// Verifies that a JWT with aud: http://example_service2 is matched to
// a JWT with aud: https://example_service2/ in config.
TEST_F(JwtAuthenticatorTest, TestOkJWTAudService2) {
  MockUpstream mock_pubkey(mock_cm_, kPublicKey);

  // Test OK pubkey and its cache
  auto headers =
    Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kGoodTokenAudService2}};

  MockJwtAuthenticatorCallbacks mock_cb;
  EXPECT_CALL(mock_cb, onError(_)).Times(0);
  EXPECT_CALL(mock_cb, onSuccess(_,_)).WillOnce(Invoke([](const Jwt *jwt, const Http::LowerCaseString *header) {
    ASSERT_NE(jwt, nullptr);
    ASSERT_NE(header, nullptr);
  }));

  auth_->Verify(headers, &mock_cb);

  EXPECT_EQ(mock_pubkey.called_count(), 1);
}

TEST_F(JwtAuthenticatorTest, TestMissedJWT) {
  EXPECT_CALL(mock_cm_, httpAsyncClientForCluster(_)).Times(0);
  EXPECT_CALL(mock_cb_, onError(_)).WillOnce(Invoke([](Status status) {
    ASSERT_EQ(status, Status::JWT_MISSED);
  }));
  EXPECT_CALL(mock_cb_, onSuccess(_,_)).Times(0);

  // Empty headers.
  auto headers = Http::TestHeaderMapImpl{};
  auth_->Verify(headers, &mock_cb_);
}

TEST_F(JwtAuthenticatorTest, TestInvalidJWT) {
  EXPECT_CALL(mock_cm_, httpAsyncClientForCluster(_)).Times(0);
  EXPECT_CALL(mock_cb_, onError(_)).WillOnce(Invoke([](Status status) {
    ASSERT_EQ(status, Status::JWT_BAD_FORMAT);
  }));
  EXPECT_CALL(mock_cb_, onSuccess(_,_)).Times(0);

  std::string token = "invalidToken";
  auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer " + token}};
  auth_->Verify(headers, &mock_cb_);
}

TEST_F(JwtAuthenticatorTest, TestInvalidPrefix) {
  EXPECT_CALL(mock_cm_, httpAsyncClientForCluster(_)).Times(0);
  EXPECT_CALL(mock_cb_, onError(_)).WillOnce(Invoke([](Status status) {
    ASSERT_EQ(status, Status::JWT_MISSED);
  }));
  EXPECT_CALL(mock_cb_, onSuccess(_,_)).Times(0);

  auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer-invalid"}};
  auth_->Verify(headers, &mock_cb_);
}

TEST_F(JwtAuthenticatorTest, TestExpiredJWT) {
  EXPECT_CALL(mock_cm_, httpAsyncClientForCluster(_)).Times(0);
  EXPECT_CALL(mock_cb_, onError(_)).WillOnce(Invoke([](Status status) {
    ASSERT_EQ(status, Status::JWT_EXPIRED);
  }));
  EXPECT_CALL(mock_cb_, onSuccess(_,_)).Times(0);

  auto headers =
    Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kExpiredToken}};
  auth_->Verify(headers, &mock_cb_);
}

TEST_F(JwtAuthenticatorTest, TestNonMatchAudJWT) {
  EXPECT_CALL(mock_cm_, httpAsyncClientForCluster(_)).Times(0);
  EXPECT_CALL(mock_cb_, onError(_)).WillOnce(Invoke([](Status status) {
    ASSERT_EQ(status, Status::AUDIENCE_NOT_ALLOWED);
  }));
  EXPECT_CALL(mock_cb_, onSuccess(_,_)).Times(0);

  auto headers =
    Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kInvalidAudToken}};
  auth_->Verify(headers, &mock_cb_);
}

TEST_F(JwtAuthenticatorTest, TestWrongCluster) {
  // Get returns nullptr
  EXPECT_CALL(mock_cm_, get(_))
      .WillOnce(Invoke(
          [](const std::string &cluster) -> Upstream::ThreadLocalCluster * {
            EXPECT_EQ(cluster, "pubkey_cluster");
            return nullptr;
          }));

  EXPECT_CALL(mock_cm_, httpAsyncClientForCluster(_)).Times(0);
  EXPECT_CALL(mock_cb_, onError(_)).WillOnce(Invoke([](Status status) {
    ASSERT_EQ(status, Status::FAILED_FETCH_PUBKEY);
  }));
  EXPECT_CALL(mock_cb_, onSuccess(_,_)).Times(0);

  auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kGoodToken}};
  auth_->Verify(headers, &mock_cb_);
}

TEST_F(JwtAuthenticatorTest, TestIssuerNotFound) {
  // Create a config with an other issuer.
  SetupConfig(kOtherIssuerConfig);

  EXPECT_CALL(mock_cm_, httpAsyncClientForCluster(_)).Times(0);
  EXPECT_CALL(mock_cb_, onError(_)).WillOnce(Invoke([](Status status) {
    ASSERT_EQ(status, Status::JWT_UNKNOWN_ISSUER);
  }));
  EXPECT_CALL(mock_cb_, onSuccess(_,_)).Times(0);

  auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kGoodToken}};
  auth_->Verify(headers, &mock_cb_);
}

TEST_F(JwtAuthenticatorTest, TestPubkeyFetchFail) {
  NiceMock<Http::MockAsyncClient> async_client;
  EXPECT_CALL(mock_cm_, httpAsyncClientForCluster(_))
      .WillOnce(Invoke([&](const std::string &cluster) -> Http::AsyncClient & {
        EXPECT_EQ(cluster, "pubkey_cluster");
        return async_client;
      }));

  Http::MockAsyncClientRequest request(&async_client);
  Http::AsyncClient::Callbacks *callbacks;
  EXPECT_CALL(async_client, send_(_, _, _))
      .WillOnce(Invoke([&](Http::MessagePtr &message, Http::AsyncClient::Callbacks &cb,
                           const absl::optional<std::chrono::milliseconds> &)
                           -> Http::AsyncClient::Request * {
        EXPECT_EQ((Http::TestHeaderMapImpl{
                      {":method", "GET"},
                      {":path", "/pubkey_path"},
                      {":authority", "pubkey_server"},
                  }),
                  message->headers());
        callbacks = &cb;
        return &request;
      }));

  EXPECT_CALL(mock_cb_, onError(_)).WillOnce(Invoke([](Status status) {
    ASSERT_EQ(status, Status::FAILED_FETCH_PUBKEY);
  }));
  EXPECT_CALL(mock_cb_, onSuccess(_,_)).Times(0);

  auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kGoodToken}};
  auth_->Verify(headers, &mock_cb_);

  Http::MessagePtr response_message(new Http::ResponseMessageImpl(
    Http::HeaderMapPtr{new Http::TestHeaderMapImpl{{":status", "401"}}}));
  callbacks->onSuccess(std::move(response_message));
}

TEST_F(JwtAuthenticatorTest, TestInvalidPubkey) {
  NiceMock<Http::MockAsyncClient> async_client;
  EXPECT_CALL(mock_cm_, httpAsyncClientForCluster(_))
      .WillOnce(Invoke([&](const std::string &cluster) -> Http::AsyncClient & {
        EXPECT_EQ(cluster, "pubkey_cluster");
        return async_client;
      }));

  Http::MockAsyncClientRequest request(&async_client);
  Http::AsyncClient::Callbacks *callbacks;
  EXPECT_CALL(async_client, send_(_, _, _))
      .WillOnce(Invoke([&](Http::MessagePtr &message, Http::AsyncClient::Callbacks &cb,
                           const absl::optional<std::chrono::milliseconds> &)
                           -> Http::AsyncClient::Request * {
        EXPECT_EQ((Http::TestHeaderMapImpl{
                      {":method", "GET"},
                      {":path", "/pubkey_path"},
                      {":authority", "pubkey_server"},
                  }),
                  message->headers());
        callbacks = &cb;
        return &request;
      }));

  EXPECT_CALL(mock_cb_, onError(_)).WillOnce(Invoke([](Status status) {
    ASSERT_EQ(status, Status::JWK_PARSE_ERROR);
  }));
  EXPECT_CALL(mock_cb_, onSuccess(_,_)).Times(0);

  auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kGoodToken}};
  auth_->Verify(headers, &mock_cb_);

  Http::MessagePtr response_message(new Http::ResponseMessageImpl(
    Http::HeaderMapPtr{new Http::TestHeaderMapImpl{{":status", "200"}}}));
  response_message->body().reset(new Buffer::OwnedImpl("invalid publik key"));
  callbacks->onSuccess(std::move(response_message));
}

TEST_F(JwtAuthenticatorTest, TestOnDestroy) {
  NiceMock<Http::MockAsyncClient> async_client;
  EXPECT_CALL(mock_cm_, httpAsyncClientForCluster(_))
      .WillOnce(Invoke([&](const std::string &cluster) -> Http::AsyncClient & {
        EXPECT_EQ(cluster, "pubkey_cluster");
        return async_client;
      }));

  Http::MockAsyncClientRequest request(&async_client);
  Http::AsyncClient::Callbacks *callbacks;
  EXPECT_CALL(async_client, send_(_, _, _))
      .WillOnce(Invoke([&](Http::MessagePtr &message, Http::AsyncClient::Callbacks &cb,
                           const absl::optional<std::chrono::milliseconds> &)
                           -> Http::AsyncClient::Request * {
        EXPECT_EQ((Http::TestHeaderMapImpl{
                      {":method", "GET"},
                      {":path", "/pubkey_path"},
                      {":authority", "pubkey_server"},
                  }),
                  message->headers());
        callbacks = &cb;
        return &request;
      }));

  // Cancel is called once.
  EXPECT_CALL(request, cancel()).Times(1);

  // onxxx() should not be called.
  EXPECT_CALL(mock_cb_, onError(_)).Times(0);
  EXPECT_CALL(mock_cb_, onSuccess(_,_)).Times(0);

  auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kGoodToken}};
  auth_->Verify(headers, &mock_cb_);

  // Destroy the authenticating process.
  auth_->onDestroy();
}

TEST_F(JwtAuthenticatorTest, TestInlineJwks) {
  // Change the config to use local_jwks.inline_string
  rules_[0].clear_remote_jwks();
  auto local_jwks = rules_[0].mutable_local_jwks();
  local_jwks->set_inline_string(kPublicKey);

  // recreate store and auth with modified config.
  store_.reset(new JwtAuthStore(rules_));
  auth_.reset(new JwtAuthenticatorImpl(mock_cm_, *store_));

  MockUpstream mock_pubkey(mock_cm_, "");
  auto headers = Http::TestHeaderMapImpl{{"Authorization", "Bearer " + kGoodToken}};

  MockJwtAuthenticatorCallbacks mock_cb;
  EXPECT_CALL(mock_cb, onError(_)).Times(0);
  EXPECT_CALL(mock_cb, onSuccess(_,_)).Times(1);

  auth_->Verify(headers, &mock_cb);
  EXPECT_EQ(mock_pubkey.called_count(), 0);
}

}  // namespace Jwt
}  // namespace Utils
}  // namespace Envoy
