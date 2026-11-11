#include "HandlerTest.h"

#include <limits>

#include "FileStorage.h"

namespace {

std::string makeData(const size_t size, const int seed = 7) {
    std::string data(size, '\0');
    for (size_t i = 0; i < size; ++i)
        data[i] = static_cast<char>((i * 131 + seed) & 0xFF);
    return data;
}

}

class HandlerFilesTest : public HandlerTest {
protected:
    void SetUp() override {
        HandlerTest::SetUp();
        if (IsSkipped() || HasFailure())
            return;
        ivanId_ = signUp(ivan_, "ivan");
        mashaId_ = signUp(masha_, "masha");
        petyaId_ = signUp(petya_, "petya");
        dialogId_ = openDialog(ivan_, mashaId_);
    }

    nlohmann::json init(TestClient& client, const std::string& data, const std::string& filename = "photo.jpg",
                        const MessageType kind = MessageType::Image, const std::string& sha = "") {
        send(client, protocol::fileUploadInitRequest(dialogId_, filename, "image/jpeg", kind, data.size(),
                                                     sha.empty() ? FileStorage::sha256Hex(data) : sha));
        return client.receiveOne();
    }

    nlohmann::json chunk(TestClient& client, const int64_t fileId, const uint64_t offset, const std::string_view bytes) {
        send(client, protocol::fileChunk(fileId, offset, bytes));
        return client.receiveOne();
    }

    // Полная загрузка по протоколу, возвращает последний fileChunkAck
    nlohmann::json upload(TestClient& client, const int64_t fileId, const std::string& data, uint64_t offset = 0) {
        nlohmann::json ack;
        while (offset < data.size()) {
            const auto size = std::min<size_t>(protocol::FILE_CHUNK_SIZE, data.size() - offset);
            ack = chunk(client, fileId, offset, std::string_view(data).substr(offset, size));
            EXPECT_TRUE(ack["success"].get<bool>()) << ack.dump();
            if (!ack["success"].get<bool>())
                break;
            offset = ack["offset"].get<uint64_t>();
        }
        return ack;
    }

    int64_t uploadComplete(TestClient& client, const std::string& data, const MessageType kind = MessageType::Image) {
        const auto response = init(client, data, "photo.jpg", kind);
        EXPECT_TRUE(response["success"].get<bool>()) << response.dump();
        const auto fileId = response["file_id"].get<int64_t>();
        EXPECT_TRUE(upload(client, fileId, data)["completed"].get<bool>());
        return fileId;
    }

    std::string download(TestClient& client, const int64_t fileId) {
        std::string result;
        while (true) {
            send(client, protocol::fileDownloadRequest(fileId, result.size()));
            const auto response = client.receiveOne();
            EXPECT_TRUE(response["success"].get<bool>()) << response.dump();
            if (!response["success"].get<bool>())
                return result;
            EXPECT_EQ(response["offset"].get<uint64_t>(), result.size());
            result += base64::decode(response["data"].get<std::string>()).value();
            if (result.size() >= response["size_bytes"].get<uint64_t>())
                return result;
        }
    }

    static std::string fileMessage(const int64_t dialogId, const int senderId, const int64_t fileId,
                                   const MessageType type = MessageType::Image) {
        Message m;
        m.dialog_id = static_cast<int>(dialogId);
        m.sender_id = senderId;
        m.type = type;
        if (type == MessageType::Voice)
            m.payload = VoiceContent{.duration_sec = 3, .file_id = fileId};
        else
            m.payload = MediaContent{.caption = "смотри", .filename = "fake.exe", .size_bytes = 1, .file_id = fileId};
        return protocol::sendMessage(m);
    }

    TestClient ivan_;
    TestClient masha_;
    TestClient petya_;
    int ivanId_ = 0;
    int mashaId_ = 0;
    int petyaId_ = 0;
    int64_t dialogId_ = 0;
};

TEST_F(HandlerFilesTest, UploadsInChunksAndVerifiesHash) {
    const auto data = makeData(protocol::FILE_CHUNK_SIZE * 2 + 1000);

    const auto response = init(ivan_, data);
    ASSERT_TRUE(response["success"].get<bool>()) << response.dump();
    EXPECT_EQ(response["offset"], 0);
    EXPECT_EQ(response["chunk_size"], protocol::FILE_CHUNK_SIZE);
    const auto fileId = response["file_id"].get<int64_t>();

    const auto first = chunk(ivan_, fileId, 0, std::string_view(data).substr(0, protocol::FILE_CHUNK_SIZE));
    EXPECT_TRUE(first["success"].get<bool>());
    EXPECT_EQ(first["offset"], protocol::FILE_CHUNK_SIZE);
    EXPECT_FALSE(first["completed"].get<bool>());

    const auto last = upload(ivan_, fileId, data, protocol::FILE_CHUNK_SIZE);
    EXPECT_TRUE(last["completed"].get<bool>());
    EXPECT_EQ(last["offset"], data.size());

    const auto row = exec("SELECT status, uploaded_bytes, storage_name FROM files WHERE id = " + std::to_string(fileId));
    EXPECT_EQ(row[0][0].as<std::string>(), "complete");
    EXPECT_EQ(row[0][1].as<uint64_t>(), data.size());

    // На диске лежит файл со случайным именем, а не с именем от клиента
    const auto stored = storage_.path() / row[0][2].as<std::string>();
    EXPECT_TRUE(std::filesystem::exists(stored));
    EXPECT_EQ(std::filesystem::file_size(stored), data.size());
}

TEST_F(HandlerFilesTest, HashMismatchFailsUploadAndRemovesFile) {
    const auto data = makeData(1000);
    const auto response = init(ivan_, data, "a.jpg", MessageType::Image, FileStorage::sha256Hex("другие данные"));
    const auto fileId = response["file_id"].get<int64_t>();

    const auto ack = chunk(ivan_, fileId, 0, data);

    EXPECT_FALSE(ack["success"].get<bool>());
    EXPECT_EQ(ack["error"], "Hash mismatch");
    EXPECT_FALSE(ack["completed"].get<bool>());

    const auto row = exec("SELECT status, storage_name FROM files WHERE id = " + std::to_string(fileId));
    EXPECT_EQ(row[0][0].as<std::string>(), "failed");
    EXPECT_FALSE(std::filesystem::exists(storage_.path() / row[0][1].as<std::string>()));
}

TEST_F(HandlerFilesTest, ResumesAfterReconnectFromServerOffset) {
    const auto data = makeData(protocol::FILE_CHUNK_SIZE * 3);
    const auto fileId = init(ivan_, data)["file_id"].get<int64_t>();
    ASSERT_TRUE(chunk(ivan_, fileId, 0, std::string_view(data).substr(0, protocol::FILE_CHUNK_SIZE))["success"].get<bool>());

    // Обрыв связи: клиент переподключился и потерял file_id, поэтому начинает с init заново
    TestClient reconnected;
    send(reconnected, protocol::resumeConnectionRequest(ivan_.session->getTempToken()));
    ASSERT_TRUE(reconnected.receiveOne()["success"].get<bool>());

    const auto resumed = init(reconnected, data);
    ASSERT_TRUE(resumed["success"].get<bool>());
    EXPECT_EQ(resumed["file_id"], fileId);
    EXPECT_EQ(resumed["offset"], protocol::FILE_CHUNK_SIZE);

    send(reconnected, protocol::fileUploadStatusRequest(fileId));
    const auto status = reconnected.receiveOne();
    EXPECT_EQ(status["offset"], protocol::FILE_CHUNK_SIZE);
    EXPECT_EQ(status["status"], "uploading");

    EXPECT_TRUE(upload(reconnected, fileId, data, protocol::FILE_CHUNK_SIZE)["completed"].get<bool>());
    EXPECT_EQ(download(masha_, fileId), data);
}

TEST_F(HandlerFilesTest, WrongOffsetIsRejectedWithServerOffset) {
    const auto data = makeData(protocol::FILE_CHUNK_SIZE * 2);
    const auto fileId = init(ivan_, data)["file_id"].get<int64_t>();
    ASSERT_TRUE(chunk(ivan_, fileId, 0, std::string_view(data).substr(0, protocol::FILE_CHUNK_SIZE))["success"].get<bool>());

    const auto repeated = chunk(ivan_, fileId, 0, std::string_view(data).substr(0, protocol::FILE_CHUNK_SIZE));
    EXPECT_FALSE(repeated["success"].get<bool>());
    EXPECT_EQ(repeated["error"], "Offset mismatch");
    EXPECT_EQ(repeated["offset"], protocol::FILE_CHUNK_SIZE);

    const auto skipped = chunk(ivan_, fileId, protocol::FILE_CHUNK_SIZE + 10, "x");
    EXPECT_FALSE(skipped["success"].get<bool>());
    EXPECT_EQ(skipped["offset"], protocol::FILE_CHUNK_SIZE);
}

TEST_F(HandlerFilesTest, RejectsOversizedAndOverflowingChunks) {
    const auto data = makeData(protocol::FILE_CHUNK_SIZE + 10);
    const auto fileId = init(ivan_, data)["file_id"].get<int64_t>();

    EXPECT_EQ(chunk(ivan_, fileId, 0, data)["error"], "Chunk too large");

    const auto small = makeData(10);
    const auto smallId = init(ivan_, small, "small.jpg")["file_id"].get<int64_t>();
    EXPECT_EQ(chunk(ivan_, smallId, 0, makeData(11))["error"], "Chunk exceeds declared file size");

    send(ivan_, R"({"type":"fileChunk","file_id":)" + std::to_string(smallId) + R"(,"offset":0,"data":"@@@@"})");
    EXPECT_EQ(ivan_.receiveOne()["error"], "Invalid base64 data");
}

TEST_F(HandlerFilesTest, ValidatesUploadMetadata) {
    const auto data = makeData(10);
    const auto sha = FileStorage::sha256Hex(data);

    send(ivan_, protocol::fileUploadInitRequest(dialogId_, "../../etc/passwd", "text/plain", MessageType::File, 10, sha));
    EXPECT_FALSE(ivan_.receiveOne()["success"].get<bool>());

    send(ivan_, protocol::fileUploadInitRequest(dialogId_, "a.txt", "not a mime", MessageType::File, 10, sha));
    EXPECT_FALSE(ivan_.receiveOne()["success"].get<bool>());

    send(ivan_, protocol::fileUploadInitRequest(dialogId_, "a.txt", "text/plain", MessageType::Text, 10, sha));
    EXPECT_EQ(ivan_.receiveOne()["error"], "Unsupported file kind");

    send(ivan_, protocol::fileUploadInitRequest(dialogId_, "a.txt", "text/plain", MessageType::File, 0, sha));
    EXPECT_EQ(ivan_.receiveOne()["error"], "File is empty");

    send(ivan_, protocol::fileUploadInitRequest(dialogId_, "a.txt", "text/plain", MessageType::File, protocol::FILE_MAX_SIZE + 1, sha));
    EXPECT_EQ(ivan_.receiveOne()["error"], "File too large (max 100MB)");

    send(ivan_, protocol::fileUploadInitRequest(dialogId_, "a.txt", "text/plain", MessageType::File, 10, "abc"));
    EXPECT_FALSE(ivan_.receiveOne()["success"].get<bool>());

    EXPECT_EQ(exec("SELECT count(*) FROM files")[0][0].as<int>(), 0);
}

TEST_F(HandlerFilesTest, OutsidersCannotUploadOrDownload) {
    const auto data = makeData(100);

    const auto denied = init(petya_, data);
    EXPECT_FALSE(denied["success"].get<bool>());
    EXPECT_EQ(denied["error"], "You are not a participant of this dialog");

    const auto fileId = uploadComplete(ivan_, data);

    send(petya_, protocol::fileDownloadRequest(fileId, 0));
    const auto response = petya_.receiveOne();
    EXPECT_FALSE(response["success"].get<bool>());
    EXPECT_EQ(response["error"], "File not found");
    EXPECT_EQ(response["data"], "");

    // Чужую загрузку нельзя ни продолжить, ни узнать её статус
    const auto other = init(ivan_, makeData(50, 1), "other.jpg");
    const auto otherId = other["file_id"].get<int64_t>();
    EXPECT_EQ(chunk(masha_, otherId, 0, makeData(50, 1))["error"], "File not found");
    send(masha_, protocol::fileUploadStatusRequest(otherId));
    EXPECT_FALSE(masha_.receiveOne()["success"].get<bool>());
}

TEST_F(HandlerFilesTest, CannotDownloadUnfinishedUpload) {
    const auto data = makeData(protocol::FILE_CHUNK_SIZE + 1);
    const auto fileId = init(ivan_, data)["file_id"].get<int64_t>();
    chunk(ivan_, fileId, 0, std::string_view(data).substr(0, protocol::FILE_CHUNK_SIZE));

    send(masha_, protocol::fileDownloadRequest(fileId, 0));
    EXPECT_EQ(masha_.receiveOne()["error"], "File not found");
}

TEST_F(HandlerFilesTest, DownloadReturnsMetadataAndSupportsOffset) {
    const auto data = makeData(protocol::FILE_CHUNK_SIZE + 500);
    const auto fileId = uploadComplete(ivan_, data);

    send(masha_, protocol::fileDownloadRequest(fileId, protocol::FILE_CHUNK_SIZE));
    const auto tail = masha_.receiveOne();
    ASSERT_TRUE(tail["success"].get<bool>()) << tail.dump();
    EXPECT_EQ(tail["filename"], "photo.jpg");
    EXPECT_EQ(tail["mime_type"], "image/jpeg");
    EXPECT_EQ(tail["kind"], static_cast<int>(MessageType::Image));
    EXPECT_EQ(tail["size_bytes"], data.size());
    EXPECT_EQ(tail["sha256"], FileStorage::sha256Hex(data));
    EXPECT_EQ(base64::decode(tail["data"].get<std::string>()), data.substr(protocol::FILE_CHUNK_SIZE));

    send(masha_, protocol::fileDownloadRequest(fileId, data.size()));
    EXPECT_EQ(masha_.receiveOne()["error"], "Offset out of range");

    EXPECT_EQ(download(masha_, fileId), data);
    EXPECT_EQ(download(ivan_, fileId), data);
}

TEST_F(HandlerFilesTest, FileMessageIsDeliveredWithServerMetadata) {
    const auto data = makeData(2000);
    const auto fileId = uploadComplete(ivan_, data);

    send(ivan_, fileMessage(dialogId_, ivanId_, fileId));

    const auto delivered = masha_.receiveOne();
    ASSERT_EQ(delivered["type"], "sendMessage");
    const auto msg = delivered["data"].get<Message>();
    EXPECT_EQ(msg.type, MessageType::Image);
    const auto& media = std::get<MediaContent>(msg.payload);
    EXPECT_EQ(media.file_id, fileId);
    EXPECT_EQ(media.filename, "photo.jpg");      // не "fake.exe" из пакета клиента
    EXPECT_EQ(media.size_bytes, data.size());    // не 1 из пакета клиента
    EXPECT_EQ(media.mime_type, "image/jpeg");
    EXPECT_EQ(media.caption, "смотри");

    send(masha_, protocol::historyRequest(dialogId_, std::numeric_limits<int64_t>::max(), 10));
    const auto history = masha_.receiveOne();
    ASSERT_EQ(history["messages"].size(), 1u);
    EXPECT_EQ(history["messages"][0]["payload"]["file_id"], fileId);

    send(masha_, protocol::getDialogsRequest());
    EXPECT_EQ(masha_.receiveOne()["dialogs"][0]["last_msg_preview"], "Файл: photo.jpg");
}

TEST_F(HandlerFilesTest, VoiceMessageCarriesItsFile) {
    const auto fileId = uploadComplete(ivan_, makeData(3000), MessageType::Voice);

    send(ivan_, fileMessage(dialogId_, ivanId_, fileId, MessageType::Voice));

    const auto msg = masha_.receiveOne()["data"].get<Message>();
    ASSERT_EQ(msg.type, MessageType::Voice);
    EXPECT_EQ(std::get<VoiceContent>(msg.payload).file_id, fileId);
    EXPECT_EQ(std::get<VoiceContent>(msg.payload).size_bytes, 3000u);
}

TEST_F(HandlerFilesTest, RejectsMessagesWithForeignOrUnfinishedFiles) {
    const auto data = makeData(protocol::FILE_CHUNK_SIZE + 1);
    const auto unfinished = init(ivan_, data)["file_id"].get<int64_t>();

    send(ivan_, fileMessage(dialogId_, ivanId_, unfinished));
    EXPECT_EQ(ivan_.receiveOne()["message"], "File upload is not completed");

    const auto ivansFile = uploadComplete(ivan_, makeData(100));

    // Маша не может отправить файл, загруженный Иваном
    send(masha_, fileMessage(dialogId_, mashaId_, ivansFile));
    EXPECT_EQ(masha_.receiveOne()["message"], "File not found");

    // Тип сообщения должен совпадать с типом файла
    send(ivan_, fileMessage(dialogId_, ivanId_, ivansFile, MessageType::File));
    EXPECT_EQ(ivan_.receiveOne()["message"], "Message type does not match file kind");

    // Файл, загруженный в один диалог, нельзя переслать в другой
    const auto otherDialog = openDialog(ivan_, petyaId_);
    send(ivan_, fileMessage(otherDialog, ivanId_, ivansFile));
    EXPECT_EQ(ivan_.receiveOne()["message"], "File not found");

    EXPECT_EQ(exec("SELECT count(*) FROM messages")[0][0].as<int>(), 0);
    EXPECT_TRUE(masha_.receive().empty());
}

TEST_F(HandlerFilesTest, RequiresAuthentication) {
    TestClient anonymous;

    send(anonymous, protocol::fileUploadInitRequest(dialogId_, "a.jpg", "image/jpeg", MessageType::Image, 10, std::string(64, 'a')));
    EXPECT_EQ(anonymous.receiveOne()["error"], "Not authenticated");

    send(anonymous, protocol::fileChunk(1, 0, "abc"));
    EXPECT_EQ(anonymous.receiveOne()["error"], "Not authenticated");

    send(anonymous, protocol::fileDownloadRequest(1, 0));
    EXPECT_EQ(anonymous.receiveOne()["error"], "Not authenticated");

    send(anonymous, protocol::fileUploadStatusRequest(1));
    EXPECT_EQ(anonymous.receiveOne()["error"], "Not authenticated");
}
