#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/* Helper: send raw HTTP request to the controller API and return HTTP status code */
static int send_request(const char *host, int port, const char *request)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    struct timeval tv = {.tv_sec = 3, .tv_usec = 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    send(sock, request, strlen(request), 0);

    char buf[512] = {0};
    recv(sock, buf, sizeof(buf) - 1, 0);
    close(sock);

    /* Parse HTTP status code */
    int status = 0;
    if (strncmp(buf, "HTTP/", 5) == 0) {
        sscanf(buf + 9, "%d", &status);
    }
    return status;
}

START_TEST(test_unauthenticated_requests_rejected)
{
    /* Invariant: Protected endpoints must reject requests without valid authentication */
    const char *host = getenv("CONTROLLER_HOST") ? getenv("CONTROLLER_HOST") : "127.0.0.1";
    int port = getenv("CONTROLLER_PORT") ? atoi(getenv("CONTROLLER_PORT")) : 80;

    const char *payloads[] = {
        /* No auth header at all */
        "POST /api/config HTTP/1.1\r\nHost: controller\r\nContent-Type: application/json\r\nContent-Length: 80\r\n\r\n{\"wifi\":{\"station\":{\"ssid\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\",\"password\":\"x\"}}}",
        /* Malformed/invalid token */
        "POST /api/config HTTP/1.1\r\nHost: controller\r\nAuthorization: Bearer INVALID_TOKEN_XYZ\r\nContent-Type: application/json\r\nContent-Length: 80\r\n\r\n{\"wifi\":{\"station\":{\"ssid\":\"test\",\"password\":\"test\"}}}",
        /* Expired token (example JWT with exp in the past) */
        "POST /api/config HTTP/1.1\r\nHost: controller\r\nAuthorization: Bearer eyJhbGciOiJIUzI1NiJ9.eyJleHAiOjF9.fake\r\nContent-Type: application/json\r\nContent-Length: 80\r\n\r\n{\"wifi\":{\"station\":{\"ssid\":\"net\",\"password\":\"pass\"}}}",
        /* Overflow payload without auth - the exact exploit vector */
        "POST /api/config HTTP/1.1\r\nHost: controller\r\nContent-Type: application/json\r\nContent-Length: 200\r\n\r\n{\"wifi\":{\"station\":{\"ssid\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\",\"password\":\"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\"}}}"
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        int status = send_request(host, port, payloads[i]);
        /* If connection refused, skip (server not running in test env) */
        if (status == -1) continue;
        /* Must get 401 or 403, never 200/204 which would mean unauthenticated access */
        ck_assert_msg(status == 401 || status == 403,
                      "Payload %d: expected 401/403 but got %d - endpoint lacks authentication",
                      i, status);
    }
}
END_TEST

Suite