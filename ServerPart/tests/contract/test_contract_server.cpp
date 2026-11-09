#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <set>

#include "Validator.h"
#include "protocol.h"

using nlohmann::json;

namespace {

json fixture(const std::string& name) {
    std::ifstream file(std::string(CONTRACT_DIR) + "/" + name + ".json");
    if (!file)
        throw std::runtime_error("Нет эталонного пакета: " + name);
    return json::parse(file);
}

std::set<std::string> fixturesIn(const std::string& dir) {
    std::set<std::string> names;
    for (const auto& entry : std::filesystem::directory_iterator(std::string(CONTRACT_DIR) + "/" + dir))
        names.insert(dir + "/" + entry.path().stem().string());
    return names;
}

std::vector<Message> messages(const json& j) {
    return j.get<std::vector<Message>>();
}

}

TEST(ServerContract, EveryFixtureIsCovered) {
    const std::set<std::string> covered = {
        "server_to_client/login_response_success", "server_to_client/login_response_failure",
        "server_to_client/register_response_success", "server_to_client/register_response_failure",
        "server_to_client/search_user_response", "server_to_client/history_response",
        "server_to_client/get_dialogs_response", "server_to_client/resume_connection_response",
        "server_to_client/resume_session_response", "server_to_client/open_dialog_response_success",
        "server_to_client/open_dialog_response_failure", "server_to_client/send_message",
        "server_to_client/error",
        "client_to_server/login_request", "client_to_server/register_request",
        "client_to_server/search_user_request", "client_to_server/history_request",
        "client_to_server/get_dialogs_request", "client_to_server/ping",
        "client_to_server/resume_connection_request", "client_to_server/resume_session_request",
        "client_to_server/open_dialog_request", "client_to_server/send_message",
        "server_to_client/file_upload_init_response", "server_to_client/file_upload_status_response",
        "server_to_client/file_chunk_ack", "server_to_client/file_download_response",
        "server_to_client/send_file_message",
        "client_to_server/file_upload_init_request", "client_to_server/file_upload_status_request",
        "client_to_server/file_chunk", "client_to_server/file_download_request",
        "client_to_server/send_file_message",
    };

    auto all = fixturesIn("server_to_client");
    all.merge(fixturesIn("client_to_server"));
    EXPECT_EQ(all, covered);
}

TEST(ServerContract, LoginAndRegisterResponses) {
    for (const auto* name : {"login_response_success", "login_response_failure"}) {
        const auto f = fixture(std::string("server_to_client/") + name);
        EXPECT_EQ(json::parse(protocol::loginResponse(f["success"], f["user_id"], f["username"], f["connectToken"], f["sessionToken"], f["error"])), f) << name;
    }
    for (const auto* name : {"register_response_success", "register_response_failure"}) {
        const auto f = fixture(std::string("server_to_client/") + name);
        EXPECT_EQ(json::parse(protocol::registerResponse(f["success"], f["user_id"], f["username"], f["connectToken"], f["sessionToken"], f["error"])), f) << name;
    }
}

TEST(ServerContract, SearchUserResponse) {
    const auto f = fixture("server_to_client/search_user_response");
    EXPECT_EQ(json::parse(protocol::searchUserResponse(f["result"].get<std::vector<User>>())), f);
}

TEST(ServerContract, HistoryResponse) {
    const auto f = fixture("server_to_client/history_response");
    EXPECT_EQ(json::parse(protocol::historyResponse(f["success"], f["dialog_id"], messages(f["messages"]), f["error"])), f);
}

TEST(ServerContract, GetDialogsResponse) {
    const auto f = fixture("server_to_client/get_dialogs_response");
    const auto dialogs = f["dialogs"].get<std::vector<MetaDialog_Client>>();
    EXPECT_EQ(json::parse(protocol::getDialogsResponse(f["success"], dialogs, f["error"])), f);
}

TEST(ServerContract, ResumeResponses) {
    const auto connection = fixture("server_to_client/resume_connection_response");
    EXPECT_EQ(json::parse(protocol::resumeConnectionResponse(connection["success"])), connection);

    const auto session = fixture("server_to_client/resume_session_response");
    EXPECT_EQ(json::parse(protocol::resumeSessionResponse(session["success"], session["token"])), session);
}

TEST(ServerContract, OpenDialogResponses) {
    const auto ok = fixture("server_to_client/open_dialog_response_success");
    EXPECT_EQ(json::parse(protocol::openDialogResponse(true, ok["dialog_id"], ok["peer"].get<User>(), messages(ok["messages"]))), ok);

    const auto fail = fixture("server_to_client/open_dialog_response_failure");
    EXPECT_EQ(json::parse(protocol::openDialogResponse(false, 0, User{}, {}, fail["error"])), fail);
}

TEST(ServerContract, IncomingMessageAndError) {
    const auto message = fixture("server_to_client/send_message");
    EXPECT_EQ(json::parse(protocol::sendMessage(message["data"].get<Message>())), message);

    const auto error = fixture("server_to_client/error");
    EXPECT_EQ(json::parse(protocol::errorMessage(error["message"])), error);
}

TEST(ServerContract, ReadsCredentialRequests) {
    for (const auto* name : {"login_request", "register_request"}) {
        const auto f = fixture(std::string("client_to_server/") + name);
        std::string error;
        EXPECT_TRUE(Validator::valid_string_field(f, "username", Validator::username, error)) << name << ": " << error;
        EXPECT_TRUE(Validator::valid_string_field(f, "password", Validator::password, error)) << name << ": " << error;
    }
}

TEST(ServerContract, ReadsSearchRequest) {
    const auto f = fixture("client_to_server/search_user_request");
    std::string error;
    EXPECT_TRUE(Validator::valid_string_field(f, "username", Validator::search, error)) << error;
}

TEST(ServerContract, ReadsHistoryRequest) {
    const auto f = fixture("client_to_server/history_request");

    EXPECT_NO_THROW(f.at("dialog_id").get<int64_t>());
    EXPECT_EQ(f.at("last_msg_id").get<int64_t>(), 5'000'000'000);
    const int limit = f.at("limit").get<int>();
    EXPECT_GE(limit, 1);
    EXPECT_LE(limit, protocol::HISTORY_MAX_LIMIT);
}

TEST(ServerContract, ReadsTokenAndDialogRequests) {
    EXPECT_FALSE(fixture("client_to_server/resume_connection_request").value("token", "").empty());
    EXPECT_FALSE(fixture("client_to_server/resume_session_request").value("token", "").empty());
    EXPECT_NO_THROW(fixture("client_to_server/open_dialog_request").at("target_user_id").get<int>());
    EXPECT_EQ(fixture("client_to_server/get_dialogs_request")["type"], "getDialogsRequest");
    EXPECT_EQ(fixture("client_to_server/ping")["type"], "ping");
}

TEST(ServerContract, ReadsOutgoingMessage) {
    const auto f = fixture("client_to_server/send_message");

    const auto msg = f.at("data").get<Message>();

    EXPECT_EQ(msg.dialog_id, 15);
    EXPECT_EQ(msg.sender_id, 7);
    EXPECT_EQ(Validator::validateMessage(msg), std::nullopt);
    EXPECT_EQ(Validator::sanitize(std::get<TextContent>(msg.payload).text), "Привет!");
}

TEST(ServerContract, FileUploadResponses) {
    const auto init = fixture("server_to_client/file_upload_init_response");
    EXPECT_EQ(json::parse(protocol::fileUploadInitResponse(init["success"], init["file_id"], init["offset"], init["error"])), init);
    EXPECT_EQ(init["chunk_size"], protocol::FILE_CHUNK_SIZE);

    const auto status = fixture("server_to_client/file_upload_status_response");
    EXPECT_EQ(json::parse(protocol::fileUploadStatusResponse(status["success"], status["file_id"], status["offset"],
                                                             status["status"], status["error"])), status);

    const auto ack = fixture("server_to_client/file_chunk_ack");
    EXPECT_EQ(json::parse(protocol::fileChunkAck(ack["success"], ack["file_id"], ack["offset"], ack["completed"], ack["error"])), ack);
}

TEST(ServerContract, FileDownloadResponse) {
    const auto f = fixture("server_to_client/file_download_response");
    const FileInfo file{
        .file_id = f["file_id"].get<int64_t>(), .filename = f["filename"].get<std::string>(),
        .mime_type = f["mime_type"].get<std::string>(), .kind = static_cast<MessageType>(f["kind"].get<int>()),
        .size_bytes = f["size_bytes"].get<uint64_t>(), .sha256 = f["sha256"].get<std::string>()};
    const auto bytes = base64::decode(f["data"].get<std::string>()).value();

    EXPECT_EQ(json::parse(protocol::fileDownloadResponse(true, file, f["offset"], bytes, f["error"])), f);
}

TEST(ServerContract, IncomingFileMessage) {
    const auto f = fixture("server_to_client/send_file_message");
    EXPECT_EQ(json::parse(protocol::sendMessage(f["data"].get<Message>())), f);
}

TEST(ServerContract, ReadsFileRequests) {
    const auto init = fixture("client_to_server/file_upload_init_request");
    EXPECT_NO_THROW(init.at("dialog_id").get<int64_t>());
    EXPECT_EQ(Validator::filename(init.at("filename").get<std::string>()), std::nullopt);
    EXPECT_EQ(Validator::mimeType(init.at("mime_type").get<std::string>()), std::nullopt);
    EXPECT_TRUE(Validator::isFileKind(static_cast<MessageType>(init.at("kind").get<uint8_t>())));
    EXPECT_LE(init.at("size_bytes").get<uint64_t>(), protocol::FILE_MAX_SIZE);
    EXPECT_EQ(Validator::sha256Hex(init.at("sha256").get<std::string>()), std::nullopt);

    const auto chunk = fixture("client_to_server/file_chunk");
    EXPECT_EQ(chunk.at("file_id").get<int64_t>(), 5'000'000'001);
    EXPECT_EQ(chunk.at("offset").get<uint64_t>(), 262'144u);
    EXPECT_EQ(base64::decode(chunk.at("data").get<std::string>()), "test");

    EXPECT_EQ(fixture("client_to_server/file_upload_status_request").at("file_id").get<int64_t>(), 5'000'000'001);
    EXPECT_EQ(fixture("client_to_server/file_download_request").at("offset").get<uint64_t>(), 262'144u);
}

TEST(ServerContract, ReadsOutgoingFileMessage) {
    const auto msg = fixture("client_to_server/send_file_message").at("data").get<Message>();

    ASSERT_EQ(msg.type, MessageType::Image);
    EXPECT_EQ(std::get<MediaContent>(msg.payload).file_id, 5'000'000'001);
    EXPECT_EQ(Validator::validateMessage(msg), std::nullopt);
}
