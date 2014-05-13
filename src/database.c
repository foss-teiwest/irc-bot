#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sqlite3.h>
#include "init.h"
#include "common.h"
#include "database.h"

static sqlite3 *db;

STATIC sqlite3 *open_database(const char *db_name) {

	sqlite3 *newdb;

	if (sqlite3_open_v2(db_name, &newdb, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
				SQLITE_OPEN_FULLMUTEX, NULL) == SQLITE_OK)
		return newdb;

	fprintf(stderr, "%s\n", sqlite3_errmsg(db));
	return NULL;
}

STATIC bool sql_exec(const char *sql) {

	if (sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK)
		return true;

	fprintf(stderr, "%s\n", sqlite3_errmsg(db));
	return false;
}

STATIC bool merge_config_access_list(void) {

	sqlite3_stmt *stmt;
	const char *sql;

	sql_exec("BEGIN TRANSACTION");
	sql = "INSERT OR IGNORE INTO users(name) VALUES(?1)";
	if (sqlite3_prepare_v2(db, sql, strlen(sql) + 1, &stmt, NULL) != SQLITE_OK) {
		fprintf(stderr, "%s\n", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return false;
	}
	for (int i = 0; i < cfg.access_list_count; i++) {
		sqlite3_bind_text(stmt, 1, cfg.access_list[i], strlen(cfg.access_list[i]), SQLITE_STATIC);
		if (sqlite3_step(stmt) != SQLITE_DONE)
			fprintf(stderr, "%s\n", sqlite3_errmsg(db));

		sqlite3_reset(stmt);
	}
	sql_exec("COMMIT TRANSACTION");
	sqlite3_finalize(stmt);
	return true;
}

bool user_in_access_list(const char *user) {

	int status;
	sqlite3_stmt *stmt;

	const char *sql = "SELECT user_id FROM users WHERE name == ?1";
	status = sqlite3_prepare_v2(db, sql, strlen(sql) + 1, &stmt, NULL);
	if (status != SQLITE_OK) {
		fprintf(stderr, "%s\n", sqlite3_errstr(status));
		sqlite3_finalize(stmt);
		return false;
	}
	sqlite3_bind_text(stmt, 1, user, strlen(user), SQLITE_STATIC);
	status = sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	return status == SQLITE_ROW;
}

bool add_user(const char *user) {

	int status;
	sqlite3_stmt *stmt;

	const char *sql = "INSERT INTO users(name) VALUES(?1)";
	status = sqlite3_prepare_v2(db, sql, strlen(sql) + 1, &stmt, NULL);
	if (status != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return status;
	}
	sqlite3_bind_text(stmt, 1, user, strlen(user), SQLITE_STATIC);
	status = sqlite3_step(stmt);
	if (status != SQLITE_DONE) {
		fprintf(stderr, "%s\n", sqlite3_errstr(status));
		return false;
	}
	sqlite3_finalize(stmt);
	return true;
}

char *random_quote(void) {

	int status;
	char *quote = NULL;
	sqlite3_stmt *stmt;

	const char *sql = "SELECT quote FROM quotes ORDER BY random() LIMIT 1";
	status = sqlite3_prepare_v2(db, sql, strlen(sql) + 1, &stmt, NULL);
	if (status != SQLITE_OK) {
		fprintf(stderr, "%s\n", sqlite3_errstr(status));
		sqlite3_finalize(stmt);
		return NULL;
	}
	status = sqlite3_step(stmt);
	if (status == SQLITE_ROW)
		quote = strdup((char *) sqlite3_column_text(stmt, 0));
	else if (status == SQLITE_DONE)
		fprintf(stderr, "no quotes in the database\n");
	else
		fprintf(stderr, "%s\n", sqlite3_errstr(status));

	sqlite3_finalize(stmt);
	return quote;
}

int add_quote(const char *quote, const char *user) {

	int status;
	sqlite3_stmt *stmt;

	const char *sql = "INSERT INTO quotes(quote, user_id) VALUES(?1, (SELECT user_id FROM users WHERE name == ?2))";
	status = sqlite3_prepare_v2(db, sql, strlen(sql) + 1, &stmt, NULL);
	if (status != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return status;
	}
	sqlite3_bind_text(stmt, 1, quote, strlen(quote), SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, user,  strlen(user),  SQLITE_STATIC);
	status = sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	return status;
}

STATIC bool last_quote_modifiable(int *quote_id) {

	int status;
	sqlite3_stmt *stmt;
	bool eligible = false;

	const char *sql = "SELECT quote_id, timestamp FROM quotes ORDER BY timestamp DESC LIMIT 1";
	status = sqlite3_prepare_v2(db, sql, strlen(sql) + 1, &stmt, NULL);
	if (status != SQLITE_OK) {
		fprintf(stderr, "%s\n", sqlite3_errstr(status));
		sqlite3_finalize(stmt);
		return false;
	}
	status = sqlite3_step(stmt);
	if (status == SQLITE_ROW) {
		*quote_id = sqlite3_column_int(stmt, 0);
		eligible = (time(NULL) - sqlite3_column_int(stmt, 1)) < QUOTE_MODIFY_PERIOD;
	} else if (status == SQLITE_DONE)
		fprintf(stderr, "no quotes in the database\n");
	else
		fprintf(stderr, "%s\n", sqlite3_errstr(status));

	sqlite3_finalize(stmt);
	return eligible;
}

int modify_last_quote(const char *quote) {

	int status, quote_id;
	sqlite3_stmt *stmt;
	const char *sql;

	if (!last_quote_modifiable(&quote_id))
		return SQLITE_READONLY;

	sql = "UPDATE quotes SET quote = ?1 WHERE quote_id = ?2";
	status = sqlite3_prepare_v2(db, sql, strlen(sql) + 1, &stmt, NULL);
	if (status != SQLITE_OK) {
		sqlite3_finalize(stmt);
		return status;
	}
	sqlite3_bind_text(stmt, 1, quote, strlen(quote), SQLITE_STATIC);
	sqlite3_bind_int(stmt, 2, quote_id);
	status = sqlite3_step(stmt);

	sqlite3_finalize(stmt);
	return status;
}

bool setup_database(void) {

	db = open_database(cfg.db_name);
	if (!db)
		goto cleanup;

	sqlite3_extended_result_codes(db, 1);
	sqlite3_busy_timeout(db, 3 * MILLISECS);
	sql_exec("PRAGMA foreign_keys=on");

	if (!sql_exec("CREATE TABLE IF NOT EXISTS users(user_id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE)"))
		goto cleanup;

	if (!sql_exec("CREATE TABLE IF NOT EXISTS quotes(quote_id INTEGER PRIMARY KEY, quote TEXT NOT NULL UNIQUE, "
			"timestamp INTEGER DEFAULT (strftime('%s', 'now')), user_id INTEGER NOT NULL REFERENCES users)"))
		goto cleanup;

	if (!merge_config_access_list())
		goto cleanup;

	return true;

cleanup:
	sqlite3_close(db);
	return false;
}

void close_database(void) {

	sqlite3_close(db);
}
