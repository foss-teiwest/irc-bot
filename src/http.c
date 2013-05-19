#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "http.h"
#include "helper.h"


#ifdef TEST
	size_t curl_write_memory(char *data, size_t size, size_t elements, void *membuf)
#else
	static size_t curl_write_memory(char *data, size_t size, size_t elements, void *membuf)
#endif
{
	void *newdata;
	struct mem_buffer *mem = membuf;
	size_t total_size = size * elements;

	// Our function will be called as many times as needed by curl_easy_perform to complete the operation
	// So we increase the size of our buffer each time to accommodate for it (and null char)
	newdata = realloc(mem->buffer, mem->size + total_size + 1);
	if (newdata == NULL)
		return 0;

	// Our mem_buffer struct keeps the current size so far, so we begin writting to the end of it each time
	mem->buffer = newdata;
	memcpy(&(mem->buffer[mem->size]), data, total_size);
	mem->size += total_size;
	mem->buffer[mem->size] = '\0';

	// if the return value isn't the total number of bytes that was passed in our function,
	// curl_easy_perform will return an error
	return total_size;
}

char *shorten_url(char *long_url) {

	CURL *curl;
	CURLcode code;
	char *short_url, *temp, *url_formatted = malloc_w(URLLEN);
	struct mem_buffer mem = {0};
	struct curl_slist *headers = NULL;

	// Set the Content-type and url format as required by Google API for the POST request
	char *google_api = "https://www.googleapis.com/urlshortener/v1/url";
	headers = curl_slist_append(headers, "Content-Type: application/json");
	snprintf(url_formatted, URLLEN, "{\"longUrl\": \"%s\"}", long_url);

	curl = curl_easy_init();
	if (curl == NULL)
		return 0;

	curl_easy_setopt(curl, CURLOPT_URL, google_api); // Set API url
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, url_formatted); // Send the formatted POST
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Allow redirects
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Use our modified header

	// By default curl_easy_perform output the result in stdout, so we provide own function and data struct,
	// so we can save the output in a string
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

	code = curl_easy_perform(curl); // Do the job!
	if (code != CURLE_OK)
		printf("Error: %s\n", curl_easy_strerror(code));

	// Find the short url in the reply and null terminate it
	short_url = strstr(mem.buffer, "ht");
	if (short_url == NULL)
		return NULL;

	temp = strchr(short_url, '"');
	if (temp == NULL)
		return NULL;
	*temp = '\0';

	// short_url must be freed to avoid memory leak
	short_url = strndup(short_url, 25);

	free(mem.buffer);
	free(url_formatted);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return short_url;
}