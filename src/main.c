#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "irc.h"
#include "wrappers.h"

int main(void) {

    Irc freenode;
    char *line = calloc_wrap(BUFSIZE * sizeof(char) + 1); // Space for null char
    Parsed_data pdata = calloc_wrap(sizeof(*pdata));

    freenode = select_server(Freenode);
    if (connect_server(freenode) < 0)
        exit_msg("Irc connection failed");

    // Keep running as long connection is active
    while (get_line(freenode, line) > 0) {
        if (line[0] == 'P') // Check first character
            ping_reply(freenode, line);
        else if (line[0] == ':' && parse_line(line, pdata)) // Parsing succeeded
            // Check for nick mentions in chat and reply to sender
            if (!strcmp(pdata->command, "PRIVMSG") && strstr(pdata->message, get_nick(freenode)))
                send_message(freenode, pdata->target, "sup %s?", pdata->nick);
    }
    free(pdata);
    free(line);
    quit_server(freenode);
    return 0;
}