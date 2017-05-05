#include "http_helper.h"
#include <sstream>
#include <iostream>
#include <stdio.h>

namespace dronelink {

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

HttpHelper::HttpHelper() :
    curl(std::shared_ptr<CURL>(curl_easy_init(), curl_easy_cleanup))
{
}

HttpHelper::~HttpHelper()
{
}

bool HttpHelper::download(const std::string &url, std::string &content)
{
    std::string readBuffer;

    if (nullptr != curl) {
        CURLcode res;

        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl.get());
        content = readBuffer;

        if (res == CURLcode::CURLE_OK) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool HttpHelper::downloadAndSave(const std::string &url, const std::string &path)
{
    FILE *fp;

    if (nullptr != curl) {
        CURLcode res;

        fp = fopen(path.c_str(), "wb");
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl.get());
        fclose(fp);

        if (res == CURLcode::CURLE_OK) {
            return true;
        } else {
            return false;
        }

    }

    return false;
}


static int curl_ul_progress_update(void *p, double /*dltotal*/, double /*dlnow*/, double ultotal,
                                   double ulnow)
{
    struct dl_up_progress *myp = (struct dl_up_progress *)p;

    if (ultotal == 0 || ulnow == 0 || myp->progress_callback == nullptr) {
        return 0;
    }

    int percentage = 100 / ultotal * ulnow;

    if ((percentage > myp->progress_in_percentage) && (percentage % 5 == 0)) {
        myp->progress_in_percentage = percentage;
        return myp->progress_callback(percentage);
    }

    return 0;
}


bool HttpHelper::uploadFile(const std::string &url, const std::string &path, const
                            progress_callback_t &progress_callback)
{
    CURLcode res;

    if (nullptr != curl) {
        struct dl_up_progress prog;
        prog.progress_callback = progress_callback;

        curl_httppost *post = NULL;
        curl_httppost *last = NULL;

        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk, "Expect:");

        curl_formadd(&post, &last,
                     CURLFORM_COPYNAME, "file",
                     CURLFORM_FILE, path.c_str(),
                     CURLFORM_END);

        curl_easy_setopt(curl.get(), CURLOPT_PROGRESSFUNCTION, curl_ul_progress_update);
        curl_easy_setopt(curl.get(), CURLOPT_PROGRESSDATA, &prog);
        curl_easy_setopt(curl.get(), CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_HTTPPOST, post);
        curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);

        res = curl_easy_perform(curl.get());

        curl_slist_free_all(chunk);

        if (res) {
            return false;
        }

        curl_formfree(post);

        if (res == CURLcode::CURLE_OK) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

static int curl_dl_progress_update(void *p, double dltotal, double dlnow, double /*ultotal*/,
                                   double /*ulnow*/)
{
    struct dl_up_progress *myp = (struct dl_up_progress *)p;

    if (dltotal == 0 || dlnow == 0 || myp->progress_callback == nullptr) {
        return 0;
    }

    int percentage = 100 / dltotal * dlnow;

    if ((percentage > myp->progress_in_percentage) && (percentage % 5 == 0)) {
        myp->progress_in_percentage = percentage;
        return myp->progress_callback(percentage);
    }

    return 0;
}

bool HttpHelper::downloadAndSaveWithProgress(const std::string &url, const std::string &path, const
                                             progress_callback_t &progress_callback)
{
    FILE *fp;

    if (nullptr != curl) {
        CURLcode res;
        struct dl_up_progress prog;
        prog.progress_callback = progress_callback;

        fp = fopen(path.c_str(), "wb");
        curl_easy_setopt(curl.get(), CURLOPT_PROGRESSFUNCTION, curl_dl_progress_update);
        curl_easy_setopt(curl.get(), CURLOPT_PROGRESSDATA, &prog);
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);
        res = curl_easy_perform(curl.get());
        fclose(fp);

        if (res == CURLcode::CURLE_OK) {
            return true;
        } else {
            return false;
        }
    }

    return false;
}

}