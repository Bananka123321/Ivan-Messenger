#include <gtest/gtest.h>

#include "protocol.h"

using nlohmann::json;

namespace {

json parse(const std::string& packet) {
    return json::parse(packet);
}

}

TEST(ProtocolRequests, LoginAndRegister) {
    const auto login = parse(protocol::loginRequest("ivan", "secret1"));
    EXPECT_EQ(login, json({{"type", "loginRequest"}, {"username", "ivan"}, {"password", "secret1"}}));

    const auto reg = parse(protocol::registerRequest("ivan", "secret1"));
    EXPECT_EQ(reg, json({{"type", "registerRequest"}, {"username", "ivan"}, {"password", "secret1"}}));
}

TEST(ProtocolRequests, SearchHistoryDialogs) {
    EXPECT_EQ(parse(protocol::searchUserRequest("iv")), json({{"type", "searchUserRequest"}, {"username", "iv"}}));

    EXPECT_EQ(parse(protocol::historyRequest(5'000'000'000, 77, 50)),
              json({{"type", "historyRequest"}, {"dialog_id", 5'000'000'000}, {"last_msg_id", 77}, {"limit", 50}}));

    EXPECT_EQ(parse(protocol::getDialogsRequest()), json({{"type", "getDialogsRequest"}}));
    EXPECT_EQ(parse(protocol::ping()), json({{"type", "ping"}}));
}

TEST(ProtocolRequests, TokensAndOpenDialog) {
    EXPECT_EQ(parse(protocol::resumeConnectionRequest("abc")), json({{"type", "resumeConnectionRequest"}, {"token", "abc"}}));
    EXPECT_EQ(parse(protocol::resumeSessionRequest("xyz")), json({{"type", "resumeSessionRequest"}, {"token", "xyz"}}));
    EXPECT_EQ(parse(protocol::openDialogRequest(42)), json({{"type", "openDialogRequest"}, {"target_user_id", 42}}));
}

TEST(ProtocolRequests, EscapesSpecialCharacters) {
    const std::string tricky = "a\"b\\c\n{}";
    const auto j = parse(protocol::loginRequest(tricky, tricky));
    EXPECT_EQ(j["username"], tricky);
    EXPECT_EQ(j["password"], tricky);
}

TEST(ProtocolResponses, SendMessageWrapsMessageInData) {
    Message m;
    m.id = 1;
    m.dialog_id = 2;
    m.sender_id = 3;
    m.created_at_ms = 1760968800123;
    m.payload = TextContent{.text = "hi"};

    const auto j = parse(protocol::sendMessage(m));
    EXPECT_EQ(j["type"], "sendMessage");
    EXPECT_EQ(j["data"], json(m));
}

TEST(ProtocolResponses, LoginResponseCarriesTokensAndError) {
    const auto ok = parse(protocol::loginResponse(true, 7, "ivan", "conn", "sess"));
    EXPECT_EQ(ok, json({{"type", "loginResponse"}, {"success", true}, {"user_id", 7}, {"username", "ivan"},
                        {"connectToken", "conn"}, {"sessionToken", "sess"}, {"error", ""}}));

    const auto fail = parse(protocol::loginResponse(false, -1, "", "", "", "Invalid password"));
    EXPECT_EQ(fail["success"], false);
    EXPECT_EQ(fail["error"], "Invalid password");
}

TEST(ProtocolResponses, RegisterResponseHasSameShapeAsLogin) {
    const auto reg = parse(protocol::registerResponse(true, 7, "ivan", "conn", "sess"));
    auto login = parse(protocol::loginResponse(true, 7, "ivan", "conn", "sess"));
    login["type"] = "registerResponse";
    EXPECT_EQ(reg, login);
}

TEST(ProtocolResponses, SearchUserResponse) {
    const auto j = parse(protocol::searchUserResponse({{.username = "ivan", .user_id = 1}, {.username = "ivanova", .user_id = 2}}));
    EXPECT_EQ(j["type"], "searchUserResponse");
    EXPECT_EQ(j["result"], json::parse(R"([{"user_id":1,"username":"ivan"},{"user_id":2,"username":"ivanova"}])"));

    EXPECT_EQ(parse(protocol::searchUserResponse({}))["result"], json::array());
}

TEST(ProtocolResponses, ErrorMessage) {
    EXPECT_EQ(parse(protocol::errorMessage("Not authenticated")), json({{"type", "error"}, {"message", "Not authenticated"}}));
}

TEST(ProtocolResponses, HistoryResponse) {
    Message m;
    m.id = 10;
    m.payload = TextContent{.text = "x"};

    const auto j = parse(protocol::historyResponse(true, 99, {m}));
    EXPECT_EQ(j["type"], "historyResponse");
    EXPECT_EQ(j["success"], true);
    EXPECT_EQ(j["dialog_id"], 99);
    ASSERT_EQ(j["messages"].size(), 1u);
    EXPECT_EQ(j["messages"][0]["id"], 10);
    EXPECT_EQ(j["error"], "");
}

TEST(ProtocolResponses, GetDialogsResponse) {
    MetaDialog_Client md;
    md.dialog_id = 5;
    md.peer_id = 3;
    md.username = "masha";

    const auto j = parse(protocol::getDialogsResponse(true, {md}));
    EXPECT_EQ(j["type"], "getDialogsResponse");
    ASSERT_EQ(j["dialogs"].size(), 1u);
    EXPECT_EQ(j["dialogs"][0]["username"], "masha");
}

TEST(ProtocolResponses, ResumeResponses) {
    EXPECT_EQ(parse(protocol::resumeConnectionResponse(true)), json({{"type", "resumeConnectionResponse"}, {"success", true}}));
    EXPECT_EQ(parse(protocol::resumeSessionResponse(true, "tok")), json({{"type", "resumeSessionResponse"}, {"success", true}, {"token", "tok"}}));
    EXPECT_EQ(parse(protocol::resumeSessionResponse(false))["token"], "");
}

TEST(ProtocolResponses, OpenDialogResponseSuccessHasDialogData) {
    const auto j = parse(protocol::openDialogResponse(true, 15, {.username = "masha", .user_id = 3}, {}));

    EXPECT_EQ(j["success"], true);
    EXPECT_EQ(j["dialog_id"], 15);
    EXPECT_EQ(j["peer"], json({{"user_id", 3}, {"username", "masha"}}));
    EXPECT_EQ(j["messages"], json::array());
    EXPECT_FALSE(j.contains("error"));
}

TEST(ProtocolResponses, OpenDialogResponseFailureHasOnlyError) {
    const auto j = parse(protocol::openDialogResponse(false, 0, User{}, {}, "User not found"));

    EXPECT_EQ(j["success"], false);
    EXPECT_EQ(j["error"], "User not found");
    EXPECT_FALSE(j.contains("dialog_id"));
    EXPECT_FALSE(j.contains("peer"));
}

TEST(ProtocolFiles, UploadInitRequest) {
    const auto j = parse(protocol::fileUploadInitRequest(15, "отчёт.pdf", "application/pdf", MessageType::File, 1234, std::string(64, 'a')));

    EXPECT_EQ(j["type"], "fileUploadInitRequest");
    EXPECT_EQ(j["dialog_id"], 15);
    EXPECT_EQ(j["filename"], "отчёт.pdf");
    EXPECT_EQ(j["mime_type"], "application/pdf");
    EXPECT_EQ(j["kind"], 3);
    EXPECT_EQ(j["size_bytes"], 1234);
    EXPECT_EQ(j["sha256"], std::string(64, 'a'));
}

TEST(ProtocolFiles, ChunkCarriesBase64OfRawBytes) {
    const std::string bytes("\x00\xFF\x10 binary", 10);
    const auto j = parse(protocol::fileChunk(7, 5'000'000'000ULL, bytes));

    EXPECT_EQ(j["type"], "fileChunk");
    EXPECT_EQ(j["file_id"], 7);
    EXPECT_EQ(j["offset"], 5'000'000'000ULL);
    EXPECT_EQ(base64::decode(j["data"].get<std::string>()), bytes);
}

TEST(ProtocolFiles, FullChunkFitsIntoOnePacket) {
    const std::string bytes(protocol::FILE_CHUNK_SIZE, '\xAB');

    const auto packet = protocol::fileDownloadResponse(true, FileInfo{.file_id = 1, .filename = std::string(255, 'x'),
        .mime_type = std::string(127, 'y'), .size_bytes = protocol::FILE_MAX_SIZE, .sha256 = std::string(64, 'a')}, 0, bytes);

    EXPECT_LT(packet.size(), 1024u * 1024u) << "чанк должен помещаться в MAX_PACKET_SIZE";
}

TEST(ProtocolFiles, StatusAndDownloadRequests) {
    const auto status = parse(protocol::fileUploadStatusRequest(42));
    EXPECT_EQ(status["type"], "fileUploadStatusRequest");
    EXPECT_EQ(status["file_id"], 42);

    const auto download = parse(protocol::fileDownloadRequest(42, 262144));
    EXPECT_EQ(download["type"], "fileDownloadRequest");
    EXPECT_EQ(download["file_id"], 42);
    EXPECT_EQ(download["offset"], 262144);
}

TEST(ProtocolFiles, UploadResponsesReportServerOffset) {
    const auto init = parse(protocol::fileUploadInitResponse(true, 42, 524288));
    EXPECT_EQ(init["type"], "fileUploadInitResponse");
    EXPECT_EQ(init["success"], true);
    EXPECT_EQ(init["file_id"], 42);
    EXPECT_EQ(init["offset"], 524288);
    EXPECT_EQ(init["chunk_size"], protocol::FILE_CHUNK_SIZE);

    const auto ack = parse(protocol::fileChunkAck(false, 42, 262144, false, "Offset mismatch"));
    EXPECT_EQ(ack["type"], "fileChunkAck");
    EXPECT_EQ(ack["success"], false);
    EXPECT_EQ(ack["offset"], 262144);
    EXPECT_EQ(ack["completed"], false);
    EXPECT_EQ(ack["error"], "Offset mismatch");

    const auto status = parse(protocol::fileUploadStatusResponse(true, 42, 100, "uploading"));
    EXPECT_EQ(status["type"], "fileUploadStatusResponse");
    EXPECT_EQ(status["status"], "uploading");
    EXPECT_EQ(status["offset"], 100);
}

TEST(ProtocolFiles, DownloadResponseCarriesMetadataAndData) {
    const FileInfo file{.file_id = 42, .filename = "cat.png", .mime_type = "image/png",
                        .kind = MessageType::Image, .size_bytes = 3, .sha256 = std::string(64, 'b')};

    const auto j = parse(protocol::fileDownloadResponse(true, file, 0, "abc"));

    EXPECT_EQ(j["type"], "fileDownloadResponse");
    EXPECT_EQ(j["success"], true);
    EXPECT_EQ(j["file_id"], 42);
    EXPECT_EQ(j["offset"], 0);
    EXPECT_EQ(j["data"], "YWJj");
    EXPECT_EQ(j["filename"], "cat.png");
    EXPECT_EQ(j["mime_type"], "image/png");
    EXPECT_EQ(j["kind"], 1);
    EXPECT_EQ(j["size_bytes"], 3);
    EXPECT_EQ(j["sha256"], std::string(64, 'b'));
}
