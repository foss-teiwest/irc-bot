#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "curl.h"
#include "common.h"


STATIC size_t curl_write_memory(char *data, size_t size, size_t elements, void *membuf) {

	Mem_buffer *mem = membuf;
	size_t total_size = size * elements;

	// Our function will be called as many times as needed by curl_easy_perform to complete the operation
	// So we increase the size of our buffer each time to accommodate for it (and null char)
	mem->buffer = realloc(mem->buffer, mem->size + total_size + 1);
	if (!mem->buffer)
		return 0;

	// Our mem_buffer struct keeps the current size so far, so we begin writting to the end of it each time
	memcpy(&(mem->buffer[mem->size]), data, total_size);
	mem->size += total_size;
	mem->buffer[mem->size] = '\0';

	// if the return value isn't the total number of bytes that was passed in our function,
	// curl_easy_perform will return an error
	return total_size;
}

char *shorten_url(const char *long_url) {

	CURL *curl;
	CURLcode code;
	char url_formatted[URLLEN], *short_url = NULL;
	Mem_buffer mem = { NULL, 0 };
	struct curl_slist *headers = NULL;

	// Set the Content-type and url format as required by Google API for the POST request
	headers = curl_slist_append(headers, "Content-Type: application/json");
	snprintf(url_formatted, URLLEN, "{\"longUrl\": \"%s\"}", long_url);

	curl = curl_easy_init();
	if (!curl)
		goto cleanup;

#ifdef TEST
	curl_easy_setopt(curl, CURLOPT_URL, TESTDIR "url-shorten.txt");
#else
	curl_easy_setopt(curl, CURLOPT_URL, "https://www.googleapis.com/urlshortener/v1/url"); // Set API url
#endif
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, url_formatted); // Send the formatted POST
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Allow redirects
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Use our modified header

	// By default curl_easy_perform output the result in stdout, so we provide own function and data struct,
	// so we can save the output in a string
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

	code = curl_easy_perform(curl); // Do the job!
	if (code != CURLE_OK || !mem.buffer) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));
		goto cleanup;
	}
	// Find the short url in the reply and null terminate it
	short_url = strstr(mem.buffer, "http");
	if (!null_terminate(short_url, '"'))
		goto cleanup;

	// short_url must be freed to avoid memory leak
	short_url = strndup(short_url, ADDRLEN);

cleanup:
	free(mem.buffer);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return short_url;
}

Github *fetch_github_commits(yajl_val *root, const char *repo, int *commit_count) {

	CURL *curl;
	CURLcode code;
	yajl_val val;
	Github *commits = NULL;
	Mem_buffer mem = { NULL, 0 };
	char API_URL[URLLEN], errbuf[1024];
	int i;

	curl = curl_easy_init();
	if (!curl)
		goto cleanup;

	// Use per_page field to limit json reply to the amount of commits specified
	snprintf(API_URL, URLLEN, "https://api.github.com/repos/%s/commits?per_page=%d", repo, *commit_count);

#ifdef TEST
	curl_easy_setopt(curl, CURLOPT_URL, TESTDIR "github.json");
#else
	curl_easy_setopt(curl, CURLOPT_URL, API_URL);
#endif
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "irc-bot"); // Github requires a user-agent
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

	code = curl_easy_perform(curl);
	if (code != CURLE_OK || !mem.buffer) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));
		goto cleanup;
	}
	*root = yajl_tree_parse(mem.buffer, errbuf, sizeof(errbuf));
	if (!*root) {
		fprintf(stderr, "%s\n", errbuf);
		goto cleanup;
	}
	free(mem.buffer);
	*commit_count = YAJL_IS_ARRAY(*root) ? YAJL_GET_ARRAY(*root)->len : 0;
	commits = MALLOC_W(*commit_count * sizeof(*commits));

	// Find the field we are interested in the json reply, save a reference to it & null terminate
	for (i = 0; i < *commit_count; i++) {
		val = yajl_tree_get(YAJL_GET_ARRAY(*root)->values[i], CFG("sha"),                      yajl_t_string);
		if (!val)
			break;

		commits[i].sha  = YAJL_GET_STRING(val);
		val = yajl_tree_get(YAJL_GET_ARRAY(*root)->values[i], CFG("commit", "author", "name"), yajl_t_string);
		if (!val)
			break;

		commits[i].name = YAJL_GET_STRING(val);
		val = yajl_tree_get(YAJL_GET_ARRAY(*root)->values[i], CFG("commit", "message"),        yajl_t_string);
		if (!val)
			break;

		commits[i].msg  = YAJL_GET_STRING(val);
		val = yajl_tree_get(YAJL_GET_ARRAY(*root)->values[i], CFG("html_url"),                 yajl_t_string);
		if (!val)
			break;

		commits[i].url  = YAJL_GET_STRING(val);

		// Cut commit message at newline character if present
		null_terminate(commits[i].msg, '\n');
	}

cleanup:
	curl_easy_cleanup(curl);
	return commits;
}

char *get_url_title(const char *url) {

	CURL *curl;
	CURLcode code;
	Mem_buffer mem = { NULL, 0 };
	char *temp, *url_title = NULL;
	bool iso = false;

	curl = curl_easy_init();
	if (!curl)
		goto cleanup;

#ifdef TEST
	curl_easy_setopt(curl, CURLOPT_URL, TESTDIR "url-title.txt");
	(void) url; // Silence warning
#else
	curl_easy_setopt(curl, CURLOPT_URL, url);
#endif
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

	code = curl_easy_perform(curl);
	if (code != CURLE_OK || !mem.buffer) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));
		goto cleanup;
	}
	// Search http response in order to determine text encoding
	temp = strcasestr(mem.buffer, "iso-8859-7");
	if (temp) // strlen("charset") = 7
		if (starts_case_with(temp - 8, "charset") || starts_case_with(temp - 7, "charset"))
			iso = true;

	temp = strcasestr(mem.buffer, "<title");
	if (!temp)
		goto cleanup;

	url_title = temp + 7; // Point url_title to the first title character

	temp = strcasestr(url_title, "</title");
	if (!temp) {
		url_title = NULL;
		goto cleanup;
	}
	*temp = '\0'; // Terminate title string

	// Replace newline characters with spaces
	for (temp = url_title; *temp != '\0'; temp++)
		if (*temp == '\n')
			*temp = ' ';

	// If title string uses ISO 8859-7 encoding then convert it to UTF-8
	// Return value must be freed to avoid memory leak
	if (iso)
		url_title = iso8859_7_to_utf8(url_title);
	else
		url_title = strndup(url_title, TITLELEN);

cleanup:
	free(mem.buffer);
	curl_easy_cleanup(curl);
	return url_title;
}
