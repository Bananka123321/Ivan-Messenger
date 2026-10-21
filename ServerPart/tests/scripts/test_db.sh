#!/usr/bin/env bash
set -euo pipefail

NAME="messenger-test-db"
IMAGE="docker.io/library/postgres:17-alpine"
PORT="${MESSENGER_TEST_DB_PORT:-55432}"
USER="${MESSENGER_TEST_DB_USER:-messenger}"
PASSWORD="${MESSENGER_TEST_DB_PASSWORD:-messenger}"
DB="${MESSENGER_TEST_DB_NAME:-messenger_test}"

if [[ "${MESSENGER_TEST_DB_EXTERNAL:-0}" == "1" ]]; then
    echo "MESSENGER_TEST_DB_EXTERNAL=1: используется внешняя база, контейнер не нужен"
    exit 0
fi

RUNTIME="$(command -v podman || command -v docker || true)"
if [[ -z "$RUNTIME" ]]; then
    echo "Не найден ни podman, ни docker" >&2
    exit 1
fi

case "${1:-}" in
    start)
        "$RUNTIME" rm -f "$NAME" >/dev/null 2>&1 || true
        "$RUNTIME" run -d --rm --name "$NAME" \
            -e POSTGRES_USER="$USER" \
            -e POSTGRES_PASSWORD="$PASSWORD" \
            -e POSTGRES_DB="$DB" \
            -p "127.0.0.1:${PORT}:5432" \
            --tmpfs /var/lib/postgresql/data \
            "$IMAGE" >/dev/null

        for _ in $(seq 1 120); do
            if "$RUNTIME" exec "$NAME" pg_isready -h 127.0.0.1 -U "$USER" -d "$DB" >/dev/null 2>&1; then
                echo "Тестовая база готова: 127.0.0.1:${PORT}/${DB}"
                exit 0
            fi
            sleep 0.5
        done

        echo "PostgreSQL не запустился за 60 секунд" >&2
        "$RUNTIME" logs "$NAME" >&2 || true
        exit 1
        ;;
    stop)
        "$RUNTIME" rm -f "$NAME" >/dev/null 2>&1 || true
        ;;
    *)
        echo "Использование: $0 start|stop" >&2
        exit 2
        ;;
esac
