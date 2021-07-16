//
// Created by sma11case on 2021/7/15.
//

#include <chrono>
#include "submodule/httplib/httplib.h"

#define SCEConst1K  (1024UL)
#define SCEConst1M  (1024UL * SCEConst1K)
#define SCEConst1G  (1024UL * SCEConst1M)
#define SCEConst1T  (1024UL * SCEConst1G)

static
uint64_t get_timestamp_ms()
{
    auto&& ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    );
    return ms.count();
}

static
std::string speed_to_unit_string(size_t speed)
{
    char tmp1[512];

    if (speed >= SCEConst1G)
    {
        auto len1 = sprintf(tmp1, "%.02lf GB/S", (double)speed / SCEConst1G);
        return std::string(tmp1, len1);
    }

    if (speed >= SCEConst1M)
    {
        auto len1 = sprintf(tmp1, "%.02lf MB/S", (double)speed / SCEConst1M);
        return std::string(tmp1, len1);
    }

    if (speed >= SCEConst1K)
    {
        auto len1 = sprintf(tmp1, "%.02lf KB/S", (double)speed / SCEConst1K);
        return std::string(tmp1, len1);
    }

    auto len1 = sprintf(tmp1, "%lu B/S", speed);
    return std::string(tmp1, len1);
}

uint64_t string_to_size(const std::string& datasize)
{
    auto sz = strtoull(datasize.c_str(), NULL, 10);

    if (datasize.find('M') != std::string::npos) sz *= (1024 * 1024);
    else if (datasize.find('G') != std::string::npos) sz *= (1024 * 1024 * 1024);
    return sz;
}

int server_main(int argc, char **argv)
{
    static uint64_t start_time_ms = 0;
    if (0 == start_time_ms) start_time_ms = get_timestamp_ms();

    httplib::Server svr;
    svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {

        auto id = (uint32_t)(get_timestamp_ms() - start_time_ms);

        size_t sz = 1024 * 1024 * 100; //100M
        if (req.has_param("size"))
        {
            auto&& datasize = req.get_param_value("size");
            std::transform(datasize.begin(), datasize.end(), datasize.begin(), ::toupper);

            sz = string_to_size(datasize);
            printf("new comming, %x, sending size=%zu,%s\n", id, sz, datasize.c_str());
        }
        else printf("new comming, %x, sending size=100M\n", id);

        res.set_content_provider(
            sz, // Content size
            "application/octet-stream", // Content type
            [=](size_t offset, size_t size, httplib::DataSink& sink) {

                try
                {
                    char data[1024];
                    size_t datalen = sizeof(data);

                    size_t sz2 = sz;
                    do
                    {
                        if (sz2 < datalen) sz2 = datalen;
                        sink.write(data, datalen);
                        sz2 -= datalen;
                    } while (sz2);

//                    sink.done(); // No more data
                    printf("finished, %x, sent size=%zu\n", id, sz);
                    return true; // return 'false' if you want to cancel the process.
                }
                catch (const std::exception& e)
                {
                    printf("[ERROR]: %s\n", e.what());
                    return false;
                }
            });
    });

#if 0
    svr.Post("/", [](const httplib::Request& req, httplib::Response& res, const httplib::ContentReader& content_reader) {

        auto id = (uint32_t)(get_timestamp_ms() - start_time_ms);

        try
        {
            size_t datalen = 0;
            if (req.is_multipart_form_data())
            {
                printf("new comming, %x, receive stream ...\n", id);

                httplib::MultipartFormDataItems files;
                content_reader([&](const httplib::MultipartFormData& file) {
                    datalen += file.content.length();
                    return true;
                }, [&](const char *data, size_t data_length) {
                    datalen += data_length;
                    return true;
                });
            }
            else
            {
                datalen = req.content_length;
                printf("new comming, %x, receive data %zu bytes\n", id, datalen);
            }
            printf("finished, %x, receive size=%zu\n", id, datalen);

            char tmp1[256];
            sprintf(tmp1, "received: %zu bytes\n", datalen);
            res.set_content(tmp1, "plain/text");
            return true;
        }
        catch (const std::exception& e)
        {
            printf("[ERROR]: %x, %s\n", id, e.what());
            return false;
        }
    });
#endif

    int port = 1234;
    if (argc >= 3) port = strtod(argv[2], NULL);
    printf("listen at 0.0.0.0:%d\n", port);

    svr.listen("0.0.0.0", port);

    return 0;
}

int client_main(int argc, char **argv)
{
    const char *p1 = argv[2];
    httplib::Client client(p1);

    //download
    {
        uint64_t datalen = 0;
        uint64_t time1 = 0;
        uint64_t time2 = 0;

        std::string t1 = "/?size=300M";
        if (argc >= 4) t1 = std::string("/?size=") + argv[3];

        printf("testing download ......\n");
        auto res = client.Get(t1.c_str(), [&](uint64_t len, uint64_t total) {
                if (0 == time1)
                {
                    datalen = total;
                    time1 = time(NULL);
                }
                time2 = time(NULL);
                if (time2 > time1)
                {
                    auto speed = len / (time2 - time1);
                    printf("%02d%% complete, download speed: %s\r", (int)(len * 100 / total), speed_to_unit_string(speed).c_str());
                }
                return true;
            }
        );
        printf("\n");
        if (res.error()) printf("[ERROR] code: %d\n", res.error());
        if (datalen && time2)
        {
            auto speed = datalen / (time2 - time1);
            printf("finished, download speed: %s\n", speed_to_unit_string(speed).c_str());
        }
    }

    //upload
    {
        uint64_t time1 = 0;
        uint64_t time2 = 0;

        uint64_t datalen = 1024 * 1024 * 300;
        if (argc >= 5) datalen = string_to_size(argv[4]);

        printf("testing uplaod ......\n");
        auto res = client.Post("/", datalen, [&](size_t offset, size_t length, httplib::DataSink& sink) {

            if (0 == time1) time1 = time(NULL);
            time2 = time(NULL);
            if (time2 > time1)
            {
                auto speed = offset / (time2 - time1);
                printf("%02d%% complete, upload speed: %s\r", (int)(offset * 100 / datalen), speed_to_unit_string(speed).c_str());
            }

            char data[1024];
            auto len1 = sizeof(data);
            if (len1 > length) len1 = length;
            if (len1) sink.write(data, len1);
//                if (len1 == length) sink.done();
            return true;
        }, "text/plain");

        printf("\n");
        if (res.error()) printf("[ERROR] code: %d\n", res.error());
        if (datalen && time2)
        {
            auto speed = datalen / (time2 - time1);
            printf("finished, upload speed: %s\n", speed_to_unit_string(speed).c_str());
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
#ifndef DEBUG
    try
    {
#endif
    if (argc >= 3)
    {
        if (0 == strcmp(argv[1], "server")) server_main(argc, argv);
        else if (0 == strcmp(argv[1], "client")) client_main(argc, argv);
    }
    else
    {
        printf("usage: speedtest server <listen-port>\n");
        printf("       speedtest client <server-url> [download-size] [upload-size]\n");
        printf("eg:    speedtest server 8888\n");
        printf("       speedtest client http://127.0.0.1:8888 300M 300M\n");
    }
#ifndef DEBUG
    }
    catch (const std::exception& e)
    {
        printf("[ERROR]: %s\n", e.what());
    }
#endif

    return 0;
}