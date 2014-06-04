#ifndef DATABASE_H
#define DATABASE_H

/**
 * @file database.h
 * All functions needed to interact with the database (sqlite)
 */

#include <stdbool.h>

#define QUOTE_MODIFY_PERIOD 600

/** Open database, create tables, merge config access list and more */
bool setup_database(void);

/** Close database */
void close_database(void);

/** Check if the user is the access list. Records from config are included */
bool user_in_access_list(const char *user);

/** Add a user to the access list */
bool add_user(const char *user);

/** Returns a random quote from the database. Don't forget to free the pointer */
char *random_quote(void);

/** Returns the last quote in the database. Free the memory when done as always */
char *last_quote(void);

/** Add quote to the database. user should be the one that adds the quote and must be in the access list */
int add_quote(const char *quote, const char *user);

/** Attempt to modify the latest quote added. After QUOTE_MODIFY_PERIOD elapses, the quote cannot be modified anymore */
int modify_last_quote(const char *quote);

#endif

