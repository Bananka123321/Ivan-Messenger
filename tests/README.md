# Тесты

Тесты написаны на GoogleTest. В клиентских тестах сигналы Qt проверяются через `QSignalSpy`.
Каждая часть проекта собирает свои тесты сама, они включаются опцией `BUILD_TESTS`.

| Вид | Метка | Что проверяет |
|---|---|---|
| Модульные | `unit` | Отдельные классы без сети и базы: `Validator`, сериализация, `PacketIO`, `Config`, `AppState`, `DialogManager`, ViewModel-и |
| Интеграционные | `integration` | Сервер: менеджеры БД и `Handler` на настоящем PostgreSQL. Клиент: `TCPClient`, `Router` и `AppController` против поддельного TLS-сервера |
| Контрактные | `contract` | Клиент и сервер одинаково понимают пакеты из `tests/contract/` |

## Сервер

```bash
cd ServerPart
cmake -S . -B cmake-build-tests -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build cmake-build-tests -j
cd cmake-build-tests
ctest --output-on-failure          # все тесты
ctest -L unit                      # только модульные
ctest -L integration               # только с базой
```

Для интеграционных тестов `ctest` сам поднимает PostgreSQL в контейнере (Podman или Docker)
через `ServerPart/tests/scripts/test_db.sh` и удаляет его после прогона. Данные лежат в tmpfs,
рабочая база не затрагивается. Таблицы создаются из `ServerPart/sql/schema.sql`.

Запуск отдельного файла тестов в обход `ctest` (например, из CLion) требует запущенной базы:

```bash
ServerPart/tests/scripts/test_db.sh start
./tests/test_handler_messaging
ServerPart/tests/scripts/test_db.sh stop
```

Без базы такие тесты пропускаются (SKIPPED) с подсказкой.

Чтобы использовать уже работающий PostgreSQL вместо контейнера:

```bash
export MESSENGER_TEST_DB_EXTERNAL=1
export MESSENGER_TEST_DB_HOST=127.0.0.1 MESSENGER_TEST_DB_PORT=5432
export MESSENGER_TEST_DB_NAME=messenger_test MESSENGER_TEST_DB_USER=... MESSENGER_TEST_DB_PASSWORD=...
```

Тесты очищают все таблицы перед каждым тестом, поэтому указывайте только отдельную тестовую базу.

## Клиент

```bash
cd ClientPart
cmake -S . -B build/tests -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build/tests -j
cd build/tests && ctest --output-on-failure
```

База для клиентских тестов не нужна. `QSettings` перенаправляются во временную папку,
так что сохранённая сессия и адрес сервера приложения не меняются.
Логи приложения в тестах выключены; включить: `MESSENGER_TEST_LOGS=1`.

Для Android тесты не собираются.

## Контракт протокола

`tests/contract/client_to_server/` и `tests/contract/server_to_client/` — эталонные пакеты.
Сервер проверяет, что строит ответы ровно такими и умеет читать запросы.
Клиент проверяет, что строит такие же запросы и правильно понимает ответы.

При изменении протокола:
1. Обновите или добавьте JSON в `tests/contract/`.
2. Обновите проверки в `ServerPart/tests/contract/test_contract_server.cpp`
   и `ClientPart/tests/contract/test_contract_client.cpp`.
3. Тест `EveryFixtureIsCovered` упадёт, если эталон есть, а проверки для него нет.

## Добавление тестов

```cmake
# ServerPart/tests/CMakeLists.txt
add_server_test(test_something unit unit/test_something.cpp)

# ClientPart/tests/CMakeLists.txt
add_client_test(test_something unit unit/test_something.cpp)
```

Вспомогательные классы:
- `TlsPair` (сервер) — TLS-соединение в памяти для `ClientSession` и `PacketIO_Server`;
- `DatabaseTest`, `HandlerTest` (сервер) — фикстуры с чистой базой и настоящим `Handler`;
- `FakeServer` (клиент) — TLS-сервер на 127.0.0.1, отвечающий так, как скажет тест.
