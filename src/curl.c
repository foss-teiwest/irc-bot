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
	Mem_buffer *mem = membuf;
	size_t total_size = size * elements;

	// Our function will be called as many times as needed by curl_easy_perform to complete the operation
	// So we increase the size of our buffer each time to accommodate for it (and null char)
	mem->buffer = realloc(mem->buffer, mem->size + total_size + 1);
	if (mem->buffer == NULL)
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
	char *temp, url_formatted[URLLEN], *short_url = NULL;
	Mem_buffer mem = {NULL, 0};
	struct curl_slist *headers = NULL;

	// Set the Content-type and url format as required by Google API for the POST request
	headers = curl_slist_append(headers, "Content-Type: application/json");
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
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Use our modified header

	// By default curl_easy_perform output the result in stdout, so we provide own function and data struct,
	// so we can save the output in a string
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

	code = curl_easy_perform(curl); // Do the job!
	if (code != CURLE_OK || mem.buffer == NULL) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));
		goto cleanup;
	}
	// Find the short url in the reply and null terminate it
	short_url = strstr(mem.buffer, "http");
	if (short_url == NULL)
		goto cleanup;

	temp = strchr(short_url, '"');
	if (temp == NULL)
		goto cleanup;
	*temp = '\0';

	// short_url must be freed to avoid memory leak
	short_url = strndup(short_url, ADDRLEN);

cleanup:
	free(mem.buffer);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return short_url;
}

char *fetch_mumble_users(void) {

	CURL *curl;
	CURLcode code;
	Mem_buffer mem = {NULL, 0};

	curl = curl_easy_init();
	if (curl == NULL)
		goto cleanup;

#ifdef TEST
	curl_easy_setopt(curl, CURLOPT_URL, "file:///home/free/programming/c/git/irc-bot/test-files/mumble.txt");
#else
	curl_easy_setopt(curl, CURLOPT_URL, "https://foss.tesyd.teimes.gr/weblist-bot.php"); // Set mumble users list url
#endif
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

	code = curl_easy_perform(curl);
	if (code != CURLE_OK || mem.buffer == NULL)
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));

cleanup:
	curl_easy_cleanup(curl);
	return mem.buffer;
}

Github *fetch_github_commits(const char *repo, int *commits, Mem_buffer *mem) {

	CURL *curl;
	CURLcode code;
	Github *commit = NULL;
	int max_commits, i;
	char *temp, *temp2, API_URL[URLLEN];

	// Save the number of commits requested to a new variable and overwrite the original value,
	// with the number of commits we actually read. (a repo might have less)
	max_commits = *commits;
	*commits = 0;
	curl = curl_easy_init();
	if (curl == NULL)
		goto cleanup;

	// Use per_page field to limit json reply to the amount of commits specified
	snprintf(API_URL, URLLEN, "https://api.github.com/repos/%s/commits?per_page=%d", repo, max_commits);

#ifdef TEST
	curl_easy_setopt(curl, CURLOPT_URL, "file:///home/free/programming/c/git/irc-bot/test-files/github-commit.txt");
#else
	curl_easy_setopt(curl, CURLOPT_URL, API_URL);
#endif
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "irc-bot"); // Github requires a user-agent
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, mem);

	code = curl_easy_perform(curl);
	if (code != CURLE_OK || mem->buffer == NULL) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));
		goto cleanup;
	}
	commit = malloc_w(max_commits * sizeof(Github));
	temp = mem->buffer;

	// Find the field we are interested in the json reply, save a reference to it & null terminate
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

		// Truncate message if too long
		if (strlen(commit[i].msg) > COMMITLEN) {
			commit[i].msg[COMMITLEN] = commit[i].msg[COMMITLEN + 1] = commit[i].msg[COMMITLEN + 2] = '.';
			commit[i].msg[COMMITLEN + 3] = '\0';
		}
		// Get long url
		temp = strstr(temp + 1, "html");
		if (temp == NULL)
			break;
		commit[i].url = temp + 11;
		temp = strchr(commit[i].url, '"');
		*temp = '\0';

		// Counter to the number of commits actually processed
		(*commits)++;
	}
cleanup:
	curl_easy_cleanup(curl);
	return commit;
}

char *get_url_title(const char *url) {

	CURL *curl;
	CURLcode code;
	Mem_buffer mem = {NULL, 0};
	char *temp, *url_title = NULL;

	curl = curl_easy_init();
	if (curl == NULL)
		goto cleanup;

#ifdef TEST
	curl_easy_setopt(curl, CURLOPT_URL, "file:///home/free/programming/c/git/irc-bot/test-files/url-title.txt");
#else
	curl_easy_setopt(curl, CURLOPT_URL, url);
#endif
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mem);

	code = curl_easy_perform(curl);
	if (code != CURLE_OK || mem.buffer == NULL) {
		fprintf(stderr, "Error: %s\n", curl_easy_strerror(code));
		goto cleanup;
	}
	temp = strcasestr(mem.buffer, "<title");
	if (temp == NULL)
		goto cleanup;
	url_title = temp + 7; // Point url_title to the first title character

	temp = strcasestr(url_title, "</title");
	if (temp == NULL) {
		url_title = NULL;
		goto cleanup;
	}
	*temp = '\0'; // Terminate title string

	// Replace newline characters with spaces
	for (temp = url_title; *temp != '\0'; temp++)
		if (*temp == '\n')
			*temp = ' ';

	// Return value must be freed to avoid memory leak
	url_title = strndup(url_title, TITLELEN);

cleanup:
	free(mem.buffer);
	curl_easy_cleanup(curl);
	return url_title;
}