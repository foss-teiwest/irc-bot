#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <curl/curl.h>
#include <yajl/yajl_tree.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include "twitter.h"
#include "curl.h"
#include "common.h"
#include "init.h"

STATIC char base64_encode_char(unsigned char u) {

	if (u < 26)
		return 'A' + u;
	if (u < 52)
		return 'a' + (u - 26);
	if (u < 62)
		return '0' + (u - 52);
	if (u == 62)
		return '+';

	return '/';
}

STATIC char *base64_encode(const unsigned char *src, int size) {

	int i;
	char *out, *p;
	unsigned char b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0;

	if (!src)
		return NULL;
	if (!size)
		size = strlen((char *) src);

	out = calloc_w(size * 4 / 3 + 4);
	p = out;

	for (i = 0; i < size; i += 3) {
		b1 = src[i];
		if (i + 1 < size)
			b2 = src[i + 1];

		if (i + 2 < size)
			b3 = src[i + 2];

		b4 = b1 >> 2;
		b5 = ((b1 & 0x3) << 4) | (b2 >> 4);
		b6 = ((b2 & 0xf) << 2) | (b3 >> 6);
		b7 = b3 & 0x3f;

		*p++= base64_encode_char(b4);
		*p++= base64_encode_char(b5);

		if (i + 1 < size)
			*p++= base64_encode_char(b6);
		else
			*p++= '=';

		if (i + 2 < size)
			*p++= base64_encode_char(b7);
		else
			*p++= '=';
	}
	return out;
}

STATIC char *generate_nonce(int len) {

	int i;
	char *nonce;
	unsigned max, seed;
	const char *chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	max = strlen(chars);
	seed = time(NULL) + pthread_self();

	nonce = malloc_w(len + 1);
	for (i = 0; i < len; i++)
		nonce[i] = chars[rand_r(&seed) % max];

	nonce[i] = '\0';
	return nonce;
}

STATIC char *prepare_parameter_string(CURL *curl, char **status_msg, char **oauth_nonce, time_t *timestamp) {

	int len;
	char parameter_string[TWEETLEN];
	char *parameter_string_url_encoded;

	*timestamp = time(NULL);
	*oauth_nonce = generate_nonce(DEFNONCE);
	*status_msg = curl_easy_escape(curl, *status_msg, 0);
	len = snprintf(parameter_string, TWEETLEN, "include_entities=true&oauth_consumer_key=%s&oauth_nonce=%s"
		"&oauth_signature_method=HMAC-SHA1&oauth_timestamp=%lu&oauth_token=%s&oauth_version=1.0&status=%s",
			cfg.oauth_consumer_key, *oauth_nonce, (unsigned long) *timestamp, cfg.oauth_token, *status_msg);

	parameter_string_url_encoded = curl_easy_escape(curl, parameter_string, len);
	return parameter_string_url_encoded;
}

STATIC char *prepare_signature_base_string(CURL *curl, char **resource_url, char *parameter_string) {

	char *signature_base_string;

	*resource_url = curl_easy_escape(curl, *resource_url, 0);
	if (!null_terminate(parameter_string, '6')) // Cut the "include_entities%3Dtrue%26" part
		return NULL;

	parameter_string += strlen(parameter_string) + 1;
	signature_base_string = malloc_w(TWEETLEN);
	snprintf(signature_base_string, TWEETLEN, "POST&%s&%s", *resource_url, parameter_string);

	return signature_base_string;
}

STATIC char *generate_oauth_signature(CURL *curl, const char *signature_base_string) {

	int len;
	unsigned char *oauth_signature;
	char *temp, *signing_key, *oauth_signature_encoded;

	len = strlen(cfg.oauth_consumer_secret) + strlen(cfg.oauth_token_secret) + 2;
	signing_key = malloc_w(len);
	len = snprintf(signing_key, len, "%s&%s", cfg.oauth_consumer_secret, cfg.oauth_token_secret);
	oauth_signature = HMAC(EVP_sha1(), signing_key, len, (unsigned char *) signature_base_string,
			strlen(signature_base_string), NULL, NULL);

	oauth_signature_encoded = base64_encode(oauth_signature, 20);
	temp = oauth_signature_encoded;
	oauth_signature_encoded = curl_easy_escape(curl, oauth_signature_encoded, 0);

	free(temp);
	free(signing_key);
	return oauth_signature_encoded;
}

STATIC struct curl_slist *prepare_http_post_request(CURL *curl, char **status_msg, const char *oauth_signature, const char *oauth_nonce, time_t timestamp) {

	char *temp, buffer[TWEETLEN];
	struct curl_slist *headers = NULL;

	snprintf(buffer, TWEETLEN, "Authorization: OAuth oauth_consumer_key=\"%s\", oauth_nonce=\"%s\", oauth_signature=\"%s\", "
		"oauth_signature_method=\"HMAC-SHA1\", oauth_timestamp=\"%lu\", oauth_token=\"%s\", oauth_version=\"1.0\"",
			cfg.oauth_consumer_key, oauth_nonce, oauth_signature, (unsigned long) timestamp, cfg.oauth_token);

	headers = curl_slist_append(headers, buffer);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	temp = malloc_w(TWEETLEN);
	snprintf(temp, TWEETLEN, "status=%s", *status_msg);
	free(*status_msg);
	*status_msg = temp;

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, *status_msg);
	curl_easy_setopt(curl, CURLOPT_URL, TWTAPI);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);

	return headers;
}

long send_tweet(char *status_msg, char *tweet_url) {

	CURL *curl;
	CURLcode code;
	struct mem_buffer mem = {NULL, 0};
	yajl_val val, root = NULL;
	char errbuf[1024];
	struct curl_slist *request = NULL;
	char *parameter_string;
	char *signature_base_string;
	char *oauth_signature = NULL;
	char *resource_url;
	char *oauth_nonce;
	time_t timestamp;
	char *tweed_id, *twt_profile_url;
	long http_status = 0;

	curl = curl_easy_init();
	if (!curl)
		return 0;

	resource_url = TWTAPI;
	parameter_string = prepare_parameter_string(curl, &status_msg, &oauth_nonce, &timestamp);
	signature_base_string = prepare_signature_base_string(curl, &resource_url, parameter_string);
	if (!signature_base_string)
		goto cleanup;

	oauth_signature = generate_oauth_signature(curl, signature_base_string);
	request = prepare_http_post_request(curl, &status_msg, oauth_signature, oauth_nonce, timestamp);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

	code = curl_easy_perform(curl);
	if (code != CURLE_OK) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));
		goto cleanup;
	}
	if (!mem.buffer) {
		fprintf(stderr, "Error: Body was empty");
		goto cleanup;
	}
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);
	if (http_status != 200)
		goto cleanup;

	root = yajl_tree_parse(mem.buffer, errbuf, sizeof(errbuf));
	if (!root) {
		fprintf(stderr, "%s\n", errbuf);
		goto cleanup;
	}
	free(mem.buffer);
	tweet_url[0] = '\0';

	val = yajl_tree_get(root, CFG("id_str"), yajl_t_string);
	if (!val) goto cleanup;
	tweed_id = YAJL_GET_STRING(val);

	val = yajl_tree_get(root, CFG("user", "screen_name"), yajl_t_string);
	if (!val) goto cleanup;
	twt_profile_url = YAJL_GET_STRING(val);

	snprintf(tweet_url, TWEET_URLLEN, "https://twitter.com/%s/status/%s", twt_profile_url, tweed_id);

cleanup:
	free(status_msg);
	free(oauth_nonce);
	free(oauth_signature);
	free(signature_base_string);
	yajl_tree_free(root);
	curl_free(resource_url);
	curl_free(parameter_string);
	curl_slist_free_all(request);
	curl_easy_cleanup(curl);
	return http_status;
}
