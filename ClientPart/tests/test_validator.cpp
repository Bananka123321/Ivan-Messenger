#include <QtTest>
#include "../../common/Validator.h"
#include "../../common/PacketIO.h"
#include "../../common/Message.h"
#include "../App/DialogManager.h"
#include "../App/AppState.h"
#include "../App/Handler.h"

// ===== Validator =====
class TestValidator : public QObject {
    Q_OBJECT
private slots:
    void testValidUsername() {
        std::string error;
        QVERIFY(Validator::valid_string_field({{"username", "ivan"}}, "username", Validator::username, error));
        QVERIFY(error.empty());
    }

    void testInvalidUsernameTooShort() {
        std::string error;
        QVERIFY(!Validator::valid_string_field({{"username", "iv"}}, "username", Validator::username, error));
        QVERIFY(!error.empty());
    }

    void testValidPassword() {
        std::string error;
        QVERIFY(Validator::valid_string_field({{"password", "123456"}}, "password", Validator::password, error));
    }

    void testInvalidPasswordEmpty() {
        std::string error;
        QVERIFY(!Validator::valid_string_field({{"password", ""}}, "password", Validator::password, error));
    }
};

// ===== PacketIO =====
class TestPacketIO : public QObject {
    Q_OBJECT
private slots:
    void testPacketConstants() {
        QVERIFY(MAX_PACKET_SIZE > 0);
        QVERIFY(MAX_PACKET_SIZE <= 10 * 1024 * 1024);
    }

    void testPacketLength() {
        std::string data = "Hello, TCP Chat!";
        uint32_t original_len = static_cast<uint32_t>(data.size());
        uint32_t network_len = htonl(original_len);
        uint32_t decoded_len = ntohl(network_len);
        QCOMPARE(decoded_len, original_len);
    }
};

// ===== DialogManager=====
class TestDialogManager : public QObject {
    Q_OBJECT
private slots:
    void testAddMessageNoDuplicate() {
        Handler h;
        AppState s;
        DialogManager dm(&h, &s);

        Message msg{1, 100, 200, "Test message", 1000};
        dm.addMessage(200, msg);
        dm.addMessage(200, msg);

        const auto* msgs = dm.getMessages(200);
        QVERIFY(msgs != nullptr);
        QCOMPARE(msgs->size(), 1);
    }

    void testPrependHistoryOrder() {
        Handler h;
        AppState s;
        DialogManager dm(&h, &s);

        Message newMsg{2, 100, 200, "New", 2000};
        Message oldMsg{1, 100, 200, "Old", 1000};

        dm.addMessage(200, newMsg);
        dm.prependHistory(200, {oldMsg});

        const auto* msgs = dm.getMessages(200);
        QCOMPARE(msgs->size(), 2);
        QCOMPARE(msgs->at(0).msgId, 1);
        QCOMPARE(msgs->at(1).msgId, 2);
    }
};

int main(int argc, char *argv[]) {
    int status = 0;

    auto runTest = [&argc, &argv, &status](QObject* test) {
        status |= QTest::qExec(test, argc, argv);
        delete test;
    };

    runTest(new TestValidator);
    runTest(new TestPacketIO);
    runTest(new TestDialogManager);

    return status;
}

#include "test_validator.moc"
