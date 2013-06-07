#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "curl.h"
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

char *shorten_url(const char *long_url) {

	CURL *curl;
	CURLcode code;
	char *temp, *url_formatted, *short_url = NULL;
	struct mem_buffer mem = {0};
	struct curl_slist *headers = NULL;

	// Set the Content-type and url format as required by Google API for the POST request
	headers = curl_slist_append(headers, "Content-Type: application/json");
	url_formatted = malloc_w(URLLEN);
	snprintf(url_formatted, URLLEN, "{\"longUrl\": \"%s\"}", long_url);

	curl = curl_easy_init();
	if (curl == NULL)
		goto cleanup;

#ifdef TEST
	curl_easy_setopt(curl, CURLOPT_URL, "file:///home/free/programming/c/git/irc-bot/test-files/url-shorten.txt");
#else
	curl_easy_setopt(curl, CURLOPT_URL, "https://www.googleapis.com/urlshortener/v1/url"); // Set API url
#endif
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, url_formatted); // Send the formatted POST
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Allow redirects
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Use our modified header

	// By default curl_easy_perform output the result in stdout, so we provide own function and data struct,
	// so we can save the output in a string
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

	code = curl_easy_perform(curl); // Do the job!
	if (code != CURLE_OK)
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));

	// Find the short url in the reply and null terminate it
	short_url = strstr(mem.buffer, "http");
	if (short_url == NULL)
		goto cleanup2;

	temp = strchr(short_url, '"');
	if (temp == NULL)
		goto cleanup2;
	*temp = '\0';

	// short_url must be freed to avoid memory leak
	short_url = strndup(short_url, 25);

cleanup2:
	free(mem.buffer);
cleanup:
	free(url_formatted);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return short_url;
}

char *fetch_mumble_users(void) {

	CURL *curl;
	CURLcode code;
	struct mem_buffer mem = {0};

	curl = curl_easy_init();
	if (curl == NULL)
		return NULL;

#ifdef TEST
	curl_easy_setopt(curl, CURLOPT_URL, "file:///home/free/programming/c/git/irc-bot/test-files/mumble.txt");
#else
	curl_easy_setopt(curl, CURLOPT_URL, "https://foss.tesyd.teimes.gr/weblist-bot.php"); // Set mumble users list url
#endif
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

	code = curl_easy_perform(curl);
	if (code != CURLE_OK)
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));

	curl_easy_cleanup(curl);
	return mem.buffer;
}

Github *fetch_github_commits(char *repo, int *commits, struct mem_buffer *mem) {

	CURL *curl;
	CURLcode code;
	Github *commit;
	int max_commits, i;
	char *API_URL, *temp, *temp2;

	memset(mem, 0, sizeof(*mem));
	max_commits = *commits;
	API_URL = malloc_w(URLLEN);
	snprintf(API_URL, URLLEN, "https://api.github.com/repos/%s/commits?per_page=%d", repo, max_commits);

	curl = curl_easy_init();
	if (curl == NULL)
		return NULL;

#ifdef TEST
	curl_easy_setopt(curl, CURLOPT_URL, "file:///home/free/programming/c/git/irc-bot/test-files/github-commit.txt");
#else
	curl_easy_setopt(curl, CURLOPT_URL, API_URL);
#endif
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "irc-bot"); // Github requires a user-agent
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, mem);

	code = curl_easy_perform(curl);
	if (code != CURLE_OK)
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));

	*commits = 0;
	commit = malloc_w(max_commits * sizeof(Github));
	temp = mem->buffer;

	for (i = 0; i < max_commits; i++) {
		temp = strstr(temp + 1, "sha");
		if (temp == NULL)
			break;
		commit[i].sha = temp + 6;
		commit[i].sha[7] = '\0';

		temp = strstr(commit[i].sha + 8, "name");
		if (temp == NULL)
			break;
		commit[i].author = temp + 7;
		temp = strchr(commit[i].author, '"');
		*temp = '\0';

		temp = strstr(temp + 1, "message");
		if (temp == NULL)
			break;
		commit[i].msg = temp + 10;
		temp = strchr(commit[i].msg, '"');
		*temp = '\0';

		// Cut commit message at newline character if present
		temp2 = strstr(commit[i].msg, "\\n");
		if (temp2 != NULL)
			*temp2 = '\0';
		if (strlen(commit[i].msg) > COMMITLEN) { // Truncate message if too long
			commit[i].msg[COMMITLEN] = commit[i].msg[COMMITLEN + 1] = commit[i].msg[COMMITLEN + 2] = '.';
			commit[i].msg[COMMITLEN + 3] = '\0';
		}
		temp = strstr(temp + 1, "html");
		if (temp == NULL)
			break;
		commit[i].url = temp + 11;
		temp = strchr(commit[i].url, '"');
		*temp = '\0';

		(*commits)++;
	}
	free(API_URL);
	curl_easy_cleanup(curl);
	return commit;
}

char *get_url_title(const char *url) {

	CURL *curl;
	CURLcode code;
	struct mem_buffer mem = {0};
	char *temp, *url_title = NULL;

	curl = curl_easy_init();
	if (curl == NULL)
		return NULL;

#ifdef TEST
	curl_easy_setopt(curl, CURLOPT_URL, "file:///home/free/programming/c/git/irc-bot/test-files/url-title.txt");
#else
	curl_easy_setopt(curl, CURLOPT_URL, url);
#endif
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT , 3L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

	code = curl_easy_perform(curl);
	if (code != CURLE_OK) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));
		goto cleanup;
	}
	temp = strstr(mem.buffer, "<title>");
	if (temp == NULL)
		goto cleanup;
	url_title = temp + 7;

	temp = strstr(url_title, "</title>");
	if (temp == NULL)
		goto cleanup;
	*temp = '\0';

	// Replace all newline characters with spaces
	temp = url_title;
	while (*temp != '\0') {
		if (*temp == '\n')
			*temp = ' ';
		temp++;
	}
	// url_title must be freed to avoid memory leak
	url_title = strndup(url_title, TITLELEN);

cleanup:
	free(mem.buffer);
	curl_easy_cleanup(curl);
	return url_title;
}