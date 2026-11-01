//////////////////////////////////////////////////////////////////////////////////////////
// Linux test harness: the libpq functions of std/dbms/postgres as ms_abi wrappers, like shim.c.
// run.sh links this (and -lpq) only into programs that call them. Needs libpq-dev
//////////////////////////////////////////////////////////////////////////////////////////
#define MS __attribute__((ms_abi))
#include <libpq-fe.h>

MS PGconn     *ms_PQconnectdb(const char *info) { return PQconnectdb(info); }
MS int         ms_PQstatus(const PGconn *c) { return (int) PQstatus(c); }
MS char       *ms_PQerrorMessage(const PGconn *c) { return PQerrorMessage(c); }
MS void        ms_PQfinish(PGconn *c) { PQfinish(c); }
MS PGresult   *ms_PQexecParams(PGconn *c, const char *command, int n, const Oid *types, const char *const *values, const int *lengths, const int *formats, int resultFormat) {
    return PQexecParams(c, command, n, types, values, lengths, formats, resultFormat);
}
MS int         ms_PQresultStatus(const PGresult *r) { return (int) PQresultStatus(r); }
MS char       *ms_PQresultErrorMessage(const PGresult *r) { return PQresultErrorMessage(r); }
MS int         ms_PQntuples(const PGresult *r) { return PQntuples(r); }
MS int         ms_PQnfields(const PGresult *r) { return PQnfields(r); }
MS char       *ms_PQfname(const PGresult *r, int i) { return PQfname(r, i); }
MS char       *ms_PQgetvalue(const PGresult *r, int row, int col) { return PQgetvalue(r, row, col); }
MS int         ms_PQgetisnull(const PGresult *r, int row, int col) { return PQgetisnull(r, row, col); }
MS char       *ms_PQcmdTuples(PGresult *r) { return PQcmdTuples(r); }
MS void        ms_PQclear(PGresult *r) { PQclear(r); }
