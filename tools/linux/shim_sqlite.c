//////////////////////////////////////////////////////////////////////////////////////////
// Linux: the SQLite functions of std/dbms/sqlite as ms_abi wrappers, like shim.c.
// link.sh links this (and -lsqlite3, or $SAAD_SQLITE_LIB) only into programs that call them. Needs libsqlite3-dev,
// or SAAD_SQLITE_INCLUDE with the folder of a sqlite3.h
//////////////////////////////////////////////////////////////////////////////////////////
#define MS __attribute__((ms_abi))
#include <sqlite3.h>

MS int         ms_sqlite3_open_v2(const char *f, sqlite3 **db, int flags, const char *vfs) { return sqlite3_open_v2(f, db, flags, vfs); }
MS int         ms_sqlite3_close_v2(sqlite3 *db) { return sqlite3_close_v2(db); }
MS const char *ms_sqlite3_errmsg(sqlite3 *db) { return sqlite3_errmsg(db); }
MS int         ms_sqlite3_busy_timeout(sqlite3 *db, int ms) { return sqlite3_busy_timeout(db, ms); }
MS int         ms_sqlite3_prepare_v2(sqlite3 *db, const char *sql, int n, sqlite3_stmt **s, const char **tail) { return sqlite3_prepare_v2(db, sql, n, s, tail); }
MS int         ms_sqlite3_finalize(sqlite3_stmt *s) { return sqlite3_finalize(s); }
MS int         ms_sqlite3_reset(sqlite3_stmt *s) { return sqlite3_reset(s); }
MS int         ms_sqlite3_clear_bindings(sqlite3_stmt *s) { return sqlite3_clear_bindings(s); }
MS int         ms_sqlite3_step(sqlite3_stmt *s) { return sqlite3_step(s); }
MS int         ms_sqlite3_bind_parameter_count(sqlite3_stmt *s) { return sqlite3_bind_parameter_count(s); }
MS const char *ms_sqlite3_bind_parameter_name(sqlite3_stmt *s, int i) { return sqlite3_bind_parameter_name(s, i); }
MS int         ms_sqlite3_bind_int64(sqlite3_stmt *s, int i, long long v) { return sqlite3_bind_int64(s, i, v); }
MS int         ms_sqlite3_bind_double(sqlite3_stmt *s, int i, double v) { return sqlite3_bind_double(s, i, v); }
MS int         ms_sqlite3_bind_text(sqlite3_stmt *s, int i, const char *t, int n, void (*d)(void *)) { return sqlite3_bind_text(s, i, t, n, d); }
MS int         ms_sqlite3_bind_null(sqlite3_stmt *s, int i) { return sqlite3_bind_null(s, i); }
MS int         ms_sqlite3_column_count(sqlite3_stmt *s) { return sqlite3_column_count(s); }
MS const char *ms_sqlite3_column_name(sqlite3_stmt *s, int i) { return sqlite3_column_name(s, i); }
MS int         ms_sqlite3_column_type(sqlite3_stmt *s, int i) { return sqlite3_column_type(s, i); }
MS const unsigned char *ms_sqlite3_column_text(sqlite3_stmt *s, int i) { return sqlite3_column_text(s, i); }
MS int         ms_sqlite3_column_bytes(sqlite3_stmt *s, int i) { return sqlite3_column_bytes(s, i); }
MS long long   ms_sqlite3_column_int64(sqlite3_stmt *s, int i) { return sqlite3_column_int64(s, i); }
MS long long   ms_sqlite3_changes64(sqlite3 *db) { return sqlite3_changes64(db); }
MS long long   ms_sqlite3_last_insert_rowid(sqlite3 *db) { return sqlite3_last_insert_rowid(db); }
