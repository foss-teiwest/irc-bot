#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curl/curl.h>
#include <openssl/hmac.h>
#include "twitter.h"
#include "common.h"


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

	out = CALLOC_W(size * 4 / 3 + 4);
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
		else *p++= '=';

		if (i + 2 < size)
			*p++= base64_encode_char(b7);
		else *p++= '=';
	}
	return out;
}

STATIC char *generate_nonce(int len) {

	int i;
	char *nonce;
	unsigned int max;
	const char *chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	max = strlen(chars);
	srand(time(NULL) + getpid());

	nonce = MALLOC_W(len + 1);
	for (i = 0; i < len; i++)
		nonce[i] = chars[rand() % max];

	nonce[i] = '\0';
	return nonce;
}

STATIC char *prepare_parameter_string(CURL *curl, char **status_msg, char **oauth_nonce, time_t *timestamp) {

	int len;
	char parameter_string[TWTLEN];
	char *parameter_string_url_encoded;

	*timestamp = time(NULL);
	*oauth_nonce = generate_nonce(DEFNONCE);
	*status_msg = curl_easy_escape(curl, *status_msg, 0);
	len = snprintf(parameter_string, TWTLEN, "include_entities=true&oauth_consumer_key=%s&oauth_nonce=%s"
		"&oauth_signature_method=HMAC-SHA1&oauth_timestamp=%lu&oauth_token=%s&oauth_version=1.0&status=%s",
			cfg.oauth_consumer_key, *oauth_nonce, (unsigned long) *timestamp, cfg.oauth_token, *status_msg);

	parameter_string_url_encoded = curl_easy_escape(curl, parameter_string, len);
	return parameter_string_url_encoded;
}

STATIC char *prepare_signature_base_string(CURL *curl, char **resource_url, char *parameter_string) {

	char *signature_base_string = MALLOC_W(TWTLEN);

	*resource_url = curl_easy_escape(curl, *resource_url, 0);
	if (!null_terminate(parameter_string, '6')) // Cut the "include_entities%3Dtrue%26" part
		return NULL;

	parameter_string += strlen(parameter_string) + 1;
	snprintf(signature_base_string, TWTLEN, "POST&%s&%s", *resource_url, parameter_string);

	return signature_base_string;
}

STATIC char *generate_oauth_signature(CURL *curl, const char *signature_base_string) {

	int len;
	unsigned char *oauth_signature;
	char *temp, *signing_key, *oauth_signature_encoded;

	len = strlen(cfg.oauth_consumer_secret) + strlen(cfg.oauth_token_secret) + 2;
	signing_key = MALLOC_W(len);
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

STATIC size_t discard_response(char *data, size_t size, size_t elements, void *null) {

	// Silence unused warnings
	(void) data;
	(void) null;
	return size * elements;
}

STATIC struct curl_slist *prepare_http_post_request(CURL *curl, char **status_msg, const char *oauth_signature, const char *oauth_nonce, time_t timestamp) {

	char *temp, buffer[TWTLEN];
	struct curl_slist *headers = NULL;

	snprintf(buffer, TWTLEN, "Authorization: OAuth oauth_consumer_key=\"%s\", oauth_nonce=\"%s\", oauth_signature=\"%s\", "
		"oauth_signature_method=\"HMAC-SHA1\", oauth_timestamp=\"%lu\", oauth_token=\"%s\", oauth_version=\"1.0\"",
			cfg.oauth_consumer_key, oauth_nonce, oauth_signature, (unsigned long) timestamp, cfg.oauth_token);

	headers = curl_slist_append(headers, buffer);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	temp = MALLOC_W(TWTLEN);
	snprintf(temp, TWTLEN, "status=%s", *status_msg);
	free(*status_msg);
	*status_msg = temp;

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, *status_msg);
	curl_easy_setopt(curl, CURLOPT_URL, TWTURL);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_response);

	return headers;
}

long send_tweet(char *status_msg) {

	CURL *curl;
	CURLcode code;
	struct curl_slist *request = NULL;
	char *parameter_string;
	char *signature_base_string;
	char *oauth_signature = NULL;
	char *resource_url;
	char *oauth_nonce;
	time_t timestamp;
	long http_status = 0;

	curl = curl_easy_init();
	if (!curl)
		return 0;

	resource_url = TWTURL;
	parameter_string = prepare_parameter_string(curl, &status_msg, &oauth_nonce, &timestamp);
	signature_base_string = prepare_signature_base_string(curl, &resource_url, parameter_string);
	if (!signature_base_string)
		goto cleanup;

	oauth_signature = generate_oauth_signature(curl, signature_base_string);
	request = prepare_http_post_request(curl, &status_msg, oauth_signature, oauth_nonce, timestamp);
	code = curl_easy_perform(curl);
	if (code != CURLE_OK) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));
		goto cleanup;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);

cleanup:
	free(status_msg);
	free(oauth_nonce);
	free(oauth_signature);
	free(signature_base_string);
	curl_free(resource_url);
	curl_free(parameter_string);
	curl_slist_free_all(request);
	curl_easy_cleanup(curl);
	return http_status;
}

