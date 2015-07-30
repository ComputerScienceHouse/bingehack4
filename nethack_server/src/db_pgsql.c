/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Copyright (c) Daniel Thaler, 2011. */
/* The NetHack server may be freely redistributed under the terms of either:
 *  - the NetHack license
 *  - the GNU General Public license v2 or later
 */

#include "nhserver.h"

#if defined(LIBPQFE_IN_SUBDIR)
# include <postgresql/libpq-fe.h>
#else
# include <libpq-fe.h>
#endif

/* prepared statement names */
#define PREP_AUTH       "auth_user"
#define PREP_REGISTER   "register_user"

/* SQL statements used */
static const char SQL_init_user_table[] =
    "CREATE TABLE users(" "uid SERIAL PRIMARY KEY, "
    "name varchar(50) UNIQUE NOT NULL, " "pwhash text NOT NULL, "
    "email text NOT NULL default '', "
    "can_debug BOOLEAN NOT NULL DEFAULT FALSE, " "ts timestamp NOT NULL, "
    "reg_ts timestamp NOT NULL" ");";

static const char SQL_init_games_table[] =
    "CREATE TABLE games(" "gid SERIAL PRIMARY KEY, " "filename text NOT NULL, "
    "plname text NOT NULL, " "role text NOT NULL, " "race text NOT NULL, "
    "gender text NOT NULL, " "alignment text NOT NULL, "
    "mode integer NOT NULL, " "moves integer NOT NULL, "
    "depth integer NOT NULL, " "level integer NOT NULL, "
    "level_desc text NOT NULL, " "done boolean NOT NULL DEFAULT FALSE, "
    "owner integer NOT NULL REFERENCES users (uid), " "ts timestamp NOT NULL, "
    "start_ts timestamp NOT NULL, " "health integer NOT NULL, " 
    "maxhealth integer NOT NULL, " "energy integer NOT NULL, "
    "maxenergy integer NOT NULL, " "wi integer NOT NULL, "
    "intel integer NOT NULL, " "str integer NOT NULL, "
    "dx integer NOT NULL, " "co integer NOT NULL, "
    "ch integer NOT NULL" ");";
    

static const char SQL_init_options_table[] =
    "CREATE TABLE options(" "uid integer NOT NULL REFERENCES users (uid), "
    "optname text NOT NULL, " "opttype integer NOT NULL, "
    "optvalue text NOT NULL, " "PRIMARY KEY(uid, optname)" ");";

static const char SQL_init_topten_table[] =
    "CREATE TABLE topten(" "gid integer PRIMARY KEY REFERENCES games (gid), "
    "points integer NOT NULL, " "hp integer NOT NULL, "
    "maxhp integer NOT NULL, " "deaths integer NOT NULL, "
    "end_how integer NOT NULL, " "death text NOT NULL, "
    "entrytxt text NOT NULL" ");";

static const char SQL_check_table[] =
    "SELECT 1::integer " "FROM   pg_tables "
    "WHERE  schemaname = 'public' AND tablename = $1::text;";

/* Note: the regprocedure type check only succeeds it the function exists. */
static const char SQL_check_pgcrypto[] =
    "SELECT 'crypt(text,text)'::regprocedure;";

static const char SQL_register_user[] =
    "INSERT INTO users (name, pwhash, email, ts, reg_ts) "
    "VALUES ($1::varchar(50), crypt($2::text, gen_salt('bf', 8)), $3::text, "
            "'now', 'now');";

static const char SQL_last_reg_id[] = "SELECT currval('users_uid_seq');";

static const char SQL_auth_user[] =
    "SELECT uid, pwhash = crypt($2::text, pwhash) AS auth_ok " "FROM   users "
    "WHERE  name = $1::varchar(50);";

static const char SQL_get_user_info[] =
    "SELECT name, can_debug " "FROM   users " "WHERE  uid = $1::bigint";

static const char SQL_update_user_ts[] =
    "UPDATE users " "SET ts = 'now' " "WHERE uid = $1::integer;";

static const char SQL_set_user_email[] =
    "UPDATE users " "SET email = $2::text " "WHERE uid = $1::integer;";

static const char SQL_set_user_password[] =
    "UPDATE users " "SET pwhash = crypt($2::text, gen_salt('bf', 8)) "
    "WHERE uid = $1::integer;";

static const char SQL_add_game[] =
    "INSERT INTO games (filename, role, race, gender, alignment, mode, moves, "
    "depth, level, owner, plname, level_desc, ts, start_ts, health, maxhealth, "
    "energy, maxenergy, wi, intel, str, dx, co, ch) "
    "VALUES ($1::text, $2::text, $3::text, $4::text, $5::text, "
    "$6::integer, 1, 1, 1, $7::integer, $8::text, $9::text, 'now', 'now', "
    "$10::integer, $11::integer, $12::integer, $13::integer, $14::integer, "
    "$15::integer, $16::integer, $17::integer, $18::integer, $19::integer)";

static const char SQL_delete_game[] =
    "DELETE FROM games WHERE owner = $1::integer AND gid = $2::integer;";

static const char SQL_last_game_id[] = "SELECT currval('games_gid_seq');";

static const char SQL_update_game_and_stats[] =
    "UPDATE games "
    "SET ts = 'now', moves = $2::integer, depth = $3::integer, "
             "level = $4::integer, level_desc = $5::text, health = $6::integer, "
             "maxhealth = $7::integer, energy = $8::integer, "
             "maxenergy = $9::integer, wi = $10::integer, "
             "intel = $11::integer, str = $12::integer, dx = $13::integer, "
             "co = $14::integer, ch = $15::integer "
    "WHERE gid = $1::integer;";

static const char SQL_update_game[] =
    "UPDATE games "
    "SET ts = 'now', moves = $2::integer, depth = $3::integer, level_desc = $4::text "
    "WHERE gid = $1::integer;";

static const char SQL_get_game_filename[] =
    "SELECT filename " "FROM games "
    "WHERE (owner = $1::integer OR $1::integer = 0) AND gid = $2::integer;";

static const char SQL_set_game_done[] =
    "UPDATE games " "SET done = TRUE " "WHERE gid = $1::integer;";

static const char SQL_list_games[] =
    "SELECT g.gid, g.filename, u.name "
    "FROM games AS g JOIN users AS u ON g.owner = u.uid "
    "WHERE (u.uid = $1::integer OR $1::integer = 0) AND g.done = $2::boolean "
    "ORDER BY g.ts DESC " "LIMIT $3::integer;";

static const char SQL_update_option[] =
    "UPDATE options " "SET optvalue = $1::text "
    "WHERE uid = $2::integer AND optname = $3::text;";

static const char SQL_insert_option[] =
    "INSERT INTO options (optvalue, uid, optname, opttype) "
    "VALUES ($1::text, $2::integer, $3::text, $4::integer);";

static const char SQL_get_options[] =
    "SELECT optname, optvalue " "FROM options " "WHERE uid = $1::integer;";

static const char SQL_add_topten_entry[] =
    "INSERT INTO topten (gid, points, hp, maxhp, deaths, end_how, death, entrytxt) "
    "VALUES ($1::integer, $2::integer, $3::integer, $4::integer, "
    "$5::integer, $6::integer, $7::text, $8::text);";

static const char SQL_get_games_since[] =
    "SELECT * FROM games WHERE ts>$1::timestamp AND done='f' ORDER BY start_ts;";


static PGconn *conn;
static char *uri = NULL;

/*
 * init the database connection.
 */
int
init_database(void)
{
    if (conn)
        close_database();
    int uri_len = asprintf(&uri, "user=%s password=%s host=%s port=%s dbname=%s %s",
                           settings.dbuser ? settings.dbuser : "''",
                           settings.dbpass ? settings.dbpass : "''",
                           settings.dbhost ? settings.dbhost : "''",
                           settings.dbport ? settings.dbport : "''",
                           settings.dbname ? settings.dbname : "''",
                           settings.dboptions ? settings.dboptions : "''");
    if (uri_len == -1) {
        fprintf(stderr, "Failed to calloc database URI. %s\n", strerror(errno));
        goto err;
    }
    conn = PQconnectdb(uri);
    if (PQstatus(conn) == CONNECTION_BAD) {
        fprintf(stderr, "Database connection failed. Check your settings.\n");
        goto err;
    }

    return TRUE;

err:
    if (uri) 
    {
        free(uri);
        uri = NULL;
    }
    PQfinish(conn);
    return FALSE;
}


static int
check_create_table(const char *tablename, const char *create_stmt)
{
    PGresult *res, *res2;
    const char *params[1];
    int paramFormats[1] = { 0 };

    params[0] = tablename;
    res2 =
        PQexecParams(conn, SQL_check_table, 1, NULL, params, NULL, paramFormats,
                     0);
    if (PQresultStatus(res2) != PGRES_TUPLES_OK || PQntuples(res2) == 0) {
        fprintf(stderr, "Table '%s' was not found. It will be created now.\n",
                tablename);
        PQclear(res2);

        res = PQexec(conn, create_stmt);
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            fprintf(stderr, "Failed to create table %s: %s", tablename,
                    PQerrorMessage(conn));
            PQclear(res);
            return FALSE;
        }
        PQclear(res);
    } else
        PQclear(res2);

    return TRUE;
}


/*
 * check the database tables and create them if necessary. Also check for the
 * existence of the crypt function
 */
int
check_database(void)
{
    PGresult *res;

    /* 
     * Perform a quick check for the presence of the pgcrypto extension:
     * A function crypt(text, text) must exist.
     */
    res = PQexec(conn, SQL_check_pgcrypto);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "PostgreSQL pgcrypto check failed: %s",
                PQerrorMessage(conn));
        PQclear(res);
        goto err;
    }
    PQclear(res);

    /* 
     * Create the required set of tables if they don't exist
     */
    if (!check_create_table("users", SQL_init_user_table) ||
        !check_create_table("games", SQL_init_games_table) ||
        !check_create_table("topten", SQL_init_topten_table) ||
        !check_create_table("options", SQL_init_options_table))
        goto err;

    /* 
     * Create prepared statements
     */
    res = PQprepare(conn, PREP_REGISTER, SQL_register_user, 0, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "prepare statement failed: %s", PQerrorMessage(conn));
        PQclear(res);
        goto err;
    }
    PQclear(res);

    res = PQprepare(conn, PREP_AUTH, SQL_auth_user, 0, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "prepare statement failed: %s", PQerrorMessage(conn));
        PQclear(res);
        goto err;
    }
    PQclear(res);

    return TRUE;

err:
    PQfinish(conn);
    return FALSE;
}


void
close_database(void)
{
    PQfinish(conn);
    conn = NULL;
    free(uri);
    uri = NULL;
}


int
db_auth_user(const char *name, const char *pass)
{
    PGresult *res;
    const char *const params[] = { name, pass };
    int uid, auth_ok, col;
    const char *uidstr;

    res = PQexecPrepared(conn, PREP_AUTH, 2, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        log_msg("db_auth_user failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return 0;
    }

    col = PQfnumber(res, "uid");
    uidstr = PQgetvalue(res, 0, col);
    uid = atoi(uidstr);

    col = PQfnumber(res, "auth_ok");
    auth_ok = (PQgetvalue(res, 0, col)[0] == 't');
    PQclear(res);

    return auth_ok ? uid : -uid;
}


int
db_register_user(const char *name, const char *pass, const char *email)
{
    PGresult *res;
    const char *const params[] = { name, pass, email };
    int uid;
    const char *uidstr;

    res = PQexecPrepared(conn, PREP_REGISTER, 3, params, NULL, NULL, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        log_msg("db_register_user failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return 0;
    }
    PQclear(res);

    res = PQexec(conn, SQL_last_reg_id);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        log_msg("db_register_user get last id failed: %s",
                PQerrorMessage(conn));
        PQclear(res);
        return 0;
    }
    uidstr = PQgetvalue(res, 0, 0);
    uid = atoi(uidstr);
    PQclear(res);

    return uid;
}


int
db_get_user_info(int uid, struct user_info *info)
{
    PGresult *res;
    char uidstr[16];
    const char *const params[] = { uidstr };
    const int paramFormats[] = { 0 };   /* text format */
    int col;

    sprintf(uidstr, "%d", uid);

    res =
        PQexecParams(conn, SQL_get_user_info, 1, NULL, params, NULL,
                     paramFormats, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        log_msg("db_get_user_info error: %s", PQerrorMessage(conn));
        PQclear(res);
        return FALSE;
    }

    col = PQfnumber(res, "can_debug");
    info->can_debug = (PQgetvalue(res, 0, col)[0] == 't');

    col = PQfnumber(res, "name");
    info->username = strdup(PQgetvalue(res, 0, col));

    info->uid = uid;

    PQclear(res);
    return TRUE;
}


void
db_update_user_ts(int uid)
{
    PGresult *res;
    char uidstr[16];
    const char *const params[] = { uidstr };
    const int paramFormats[] = { 0 };   /* text format */

    sprintf(uidstr, "%d", uid);
    res =
        PQexecParams(conn, SQL_update_user_ts, 1, NULL, params, NULL,
                     paramFormats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
        log_msg("update_user_ts error: %s", PQerrorMessage(conn));
    PQclear(res);
}


int
db_set_user_email(int uid, const char *email)
{
    PGresult *res;
    char uidstr[16];
    const char *const params[] = { uidstr, email };
    const int paramFormats[] = { 0, 0 };
    const char *numrows;

    sprintf(uidstr, "%d", uid);

    res =
        PQexecParams(conn, SQL_set_user_email, 2, NULL, params, NULL,
                     paramFormats, 0);
    numrows = PQcmdTuples(res);
    if (PQresultStatus(res) == PGRES_COMMAND_OK && atoi(numrows) == 1) {
        PQclear(res);
        return TRUE;
    }
    PQclear(res);
    return FALSE;
}


int
db_set_user_password(int uid, const char *password)
{
    PGresult *res;
    char uidstr[16];
    const char *const params[] = { uidstr, password };
    const int paramFormats[] = { 0, 0 };
    const char *numrows;

    sprintf(uidstr, "%d", uid);

    res =
        PQexecParams(conn, SQL_set_user_password, 2, NULL, params, NULL,
                     paramFormats, 0);
    numrows = PQcmdTuples(res);
    if (PQresultStatus(res) == PGRES_COMMAND_OK && atoi(numrows) == 1) {
        PQclear(res);
        return TRUE;
    }
    PQclear(res);
    return FALSE;
}


long
db_add_new_game(int uid, const char *filename, const char *role,
                const char *race, const char *gend, const char *align, int mode,
                const char *plname, const char *levdesc, int hp, int hpmax,
                int en, int enmax, int wi, int in, int st, int dx,
                int co, int ch)
{
    PGresult *res;
    char uidstr[16], modestr[16], hpstr[16], hpmaxstr[16], enstr[16],
         enmaxstr[16], wistr[16], intstr[16], ststr[16], dxstr[16], 
         costr[16], chstr[16];

    const char *const params[] = { filename, role, race, gend, align, modestr,
        uidstr, plname, levdesc, hpstr, hpmaxstr, enstr, enmaxstr, wistr,
        intstr, ststr, dxstr, costr, chstr
    };
    const int paramFormats[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0 };
    const char *gameid_str;
    int gid, numparams;

    sprintf(uidstr, "%d", uid);
    sprintf(modestr, "%d", mode);
    sprintf(hpstr, "%d", hp);
    sprintf(hpmaxstr, "%d", hpmax);
    sprintf(enstr, "%d", en);
    sprintf(enmaxstr, "%d", enmax);
    sprintf(wistr, "%d", wi);
    sprintf(intstr, "%d", in);
    sprintf(ststr, "%d", st);
    sprintf(dxstr, "%d", dx);
    sprintf(costr, "%d", co);
    sprintf(chstr, "%d", ch);

    numparams = sizeof(params) / sizeof(const char *);
    res =
        PQexecParams(conn, SQL_add_game, numparams, NULL, params, NULL,
                     paramFormats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        log_msg("db_add_new_game error while adding (%s - %s): %s", plname,
                filename, PQerrorMessage(conn));
        PQclear(res);
        return 0;
    }

    res = PQexec(conn, SQL_last_game_id);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        PQclear(res);
        return 0;
    }

    gameid_str = PQgetvalue(res, 0, 0);
    gid = atoi(gameid_str);
    PQclear(res);

    return gid;
}


void
db_update_game(int game, int moves, int depth, const char *levdesc)
{
    PGresult *res;
    char gidstr[16], movesstr[16], depthstr[16];
    const char *const params[] = { gidstr, movesstr, depthstr, levdesc };
    const int paramFormats[] = { 0, 0, 0, 0 };

    sprintf(gidstr, "%d", game);
    sprintf(movesstr, "%d", moves);
    sprintf(depthstr, "%d", depth);

    res =
        PQexecParams(conn, SQL_update_game, 4, NULL, params, NULL, paramFormats,
                     0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
        log_msg("update_game_ts error: %s", PQerrorMessage(conn));
    PQclear(res);
}


void
db_update_game_and_stats(int game, int moves, int depth, const char *levdesc,
                         int level, int hp, int hpmax, int en, int enmax,
                         int wi, int in, int st, int dx, int co, int ch)
{
    PGresult *res;
    char gidstr[16], movesstr[16], depthstr[16], lvlstr[16], hpstr[16],
         hpmaxstr[16], enstr[16], enmaxstr[16], wistr[16], intstr[16],
         ststr[16], dxstr[16], costr[16], chstr[16];
    const char *const params[] = { gidstr, movesstr, depthstr, lvlstr, levdesc,
                                   hpstr, hpmaxstr, enstr, enmaxstr, wistr,
                                   intstr, ststr, dxstr, costr, chstr };
    const int paramFormats[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    sprintf(gidstr, "%d", game);
    sprintf(movesstr, "%d", moves);
    sprintf(depthstr, "%d", depth);
    sprintf(lvlstr, "%d", level);
    sprintf(hpstr, "%d", hp);
    sprintf(hpmaxstr, "%d", hpmax);
    sprintf(enstr, "%d", en);
    sprintf(enmaxstr, "%d", enmax);
    sprintf(wistr, "%d", wi);
    sprintf(intstr, "%d", in);
    sprintf(ststr, "%d", st);
    sprintf(dxstr, "%d", dx);
    sprintf(costr, "%d", co);
    sprintf(chstr, "%d", ch);

    res =
        PQexecParams(conn, SQL_update_game_and_stats, 15, NULL, params, NULL,
                     paramFormats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
        log_msg("update_game_ts error: %s", PQerrorMessage(conn));
    PQclear(res);
}


int
db_get_game_filename(int uid, int gid, char *namebuf, int buflen)
{
    PGresult *res;
    char uidstr[16], gidstr[16];
    const char *const params[] = { uidstr, gidstr };
    const int paramFormats[] = { 0, 0 };

    sprintf(uidstr, "%d", uid);
    sprintf(gidstr, "%d", gid);

    res =
        PQexecParams(conn, SQL_get_game_filename, 2, NULL, params, NULL,
                     paramFormats, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        log_msg("get_game_filename error: %s", PQerrorMessage(conn));
        PQclear(res);
        return FALSE;
    }

    strncpy(namebuf, PQgetvalue(res, 0, 0), buflen);
    PQclear(res);
    return TRUE;
}


void
db_delete_game(int uid, int gid)
{
    PGresult *res;
    char uidstr[16], gidstr[16];
    const char *const params[] = { uidstr, gidstr };
    const int paramFormats[] = { 0, 0 };

    sprintf(uidstr, "%d", uid);
    sprintf(gidstr, "%d", gid);

    res =
        PQexecParams(conn, SQL_delete_game, 2, NULL, params, NULL, paramFormats,
                     0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
        log_msg("db_delete_game error: %s", PQerrorMessage(conn));

    PQclear(res);
}


struct gamefile_info *
db_list_games(int completed, int uid, int limit, int *count)
{
    PGresult *res;
    int i, gidcol, fncol, ucol;
    struct gamefile_info *files;
    char uidstr[16], complstr[16], limitstr[16];
    const char *const params[] = { uidstr, complstr, limitstr };
    const int paramFormats[] = { 0, 0, 0 };

    if (limit <= 0 || limit > 100)
        limit = 100;

    sprintf(uidstr, "%d", uid);
    sprintf(complstr, "%d", ! !completed);
    sprintf(limitstr, "%d", limit);

    res =
        PQexecParams(conn, SQL_list_games, 3, NULL, params, NULL, paramFormats,
                     0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_msg("list_games error: %s", PQerrorMessage(conn));
        PQclear(res);
        *count = 0;
        return NULL;
    }

    *count = PQntuples(res);
    gidcol = PQfnumber(res, "gid");
    fncol = PQfnumber(res, "filename");
    ucol = PQfnumber(res, "name");

    files = malloc(sizeof (struct gamefile_info) * (*count));
    for (i = 0; i < *count; i++) {
        files[i].gid = atoi(PQgetvalue(res, i, gidcol));
        files[i].filename = strdup(PQgetvalue(res, i, fncol));
        files[i].username = strdup(PQgetvalue(res, i, ucol));
    }

    PQclear(res);
    return files;
}


void
db_set_option(int uid, const char *optname, int type, const char *optval)
{
    PGresult *res;
    char uidstr[16], typestr[16];
    const char *const params[] = { optval, uidstr, optname, typestr };
    const int paramFormats[] = { 0, 0, 0, 0 };
    const char *numrows;

    sprintf(uidstr, "%d", uid);
    sprintf(typestr, "%d", type);

    /* try to update first */
    res =
        PQexecParams(conn, SQL_update_option, 3, NULL, params, NULL,
                     paramFormats, 0);
    numrows = PQcmdTuples(res);
    if (PQresultStatus(res) == PGRES_COMMAND_OK && atoi(numrows) == 1) {
        PQclear(res);
        return;
    }
    PQclear(res);

    /* update failed, try to insert */
    res =
        PQexecParams(conn, SQL_insert_option, 4, NULL, params, NULL,
                     paramFormats, 0);
    numrows = PQcmdTuples(res);
    if (PQresultStatus(res) == PGRES_COMMAND_OK && atoi(numrows) == 1) {
        PQclear(res);
        return;
    }
    PQclear(res);

    /* insert failed too */
    log_msg("Failed to store an option. '%s = %s': %s", optname, optval,
            PQerrorMessage(conn));
}


void
db_restore_options(int uid)
{
    PGresult *res;
    char uidstr[16];
    const char *const params[] = { uidstr };
    const char *optname;
    const int paramFormats[] = { 0 };
    int i, count, ncol, vcol;
    union nh_optvalue value;

    sprintf(uidstr, "%d", uid);

    res =
        PQexecParams(conn, SQL_get_options, 1, NULL, params, NULL, paramFormats,
                     0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_msg("get_options error: %s", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    count = PQntuples(res);
    ncol = PQfnumber(res, "optname");
    vcol = PQfnumber(res, "optvalue");
    for (i = 0; i < count; i++) {
        optname = PQgetvalue(res, i, ncol);
        value.s = PQgetvalue(res, i, vcol);
        nh_set_option(optname, value, 1);
    }
    PQclear(res);
}


void
db_add_topten_entry(int gid, int points, int hp, int maxhp, int deaths,
                    int end_how, const char *death, const char *entrytxt)
{
    PGresult *res;
    char gidstr[16], pointstr[16], hpstr[16], maxhpstr[16], dcountstr[16],
        endstr[16];
    const char *const params[] = { gidstr, pointstr, hpstr, maxhpstr,
        dcountstr, endstr, death, entrytxt
    };
    const int paramFormats[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    sprintf(gidstr, "%d", gid);
    sprintf(pointstr, "%d", points);
    sprintf(hpstr, "%d", hp);
    sprintf(maxhpstr, "%d", maxhp);
    sprintf(dcountstr, "%d", deaths);
    sprintf(endstr, "%d", end_how);

    res =
        PQexecParams(conn, SQL_add_topten_entry, 8, NULL, params, NULL,
                     paramFormats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
        log_msg("add_topten_entry error: %s", PQerrorMessage(conn));
    PQclear(res);

    /* note: the params and paramFormats arrays are re-used, but only the 1.
       entry matters */
    res =
        PQexecParams(conn, SQL_set_game_done, 1, NULL, params, NULL,
                     paramFormats, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
        log_msg("set_game_done error: %s", PQerrorMessage(conn));
    PQclear(res);
    return;
}

struct nh_board_entry *
db_get_board_entries(char *since, int *length) {
    PGresult *res;
    int i, tmp, levelcol, depthcol, hpcol, hpmaxcol, encol, enmaxcol, wicol,
        incol, stcol, dxcol, cocol, chcol, tscol, movescol, plrolecol,
        plracecol, plgendcol, plaligncol, namecol, leveldesccol;
    const char *plrole, *plrace, *plgend, *plalign, *name, *leveldesc, *ts;

    const char *const params[] = { since };
    const int paramFormats[] = { 0 };
    struct nh_board_entry *entries;

    res =
        PQexecParams(conn, SQL_get_games_since, 1, NULL, params, NULL,
                     paramFormats, 0);
    if(PQresultStatus(res) != PGRES_TUPLES_OK) {
        log_msg("db_get_board_entries error: %s", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    *length = PQntuples(res);
    levelcol = PQfnumber(res, "level");
    depthcol = PQfnumber(res, "depth");
    hpcol = PQfnumber(res, "health");
    hpmaxcol = PQfnumber(res, "maxhealth");
    encol = PQfnumber(res, "energy");
    enmaxcol = PQfnumber(res, "maxenergy");
    wicol = PQfnumber(res, "wi");
    incol = PQfnumber(res, "intel");
    stcol = PQfnumber(res, "str");
    dxcol = PQfnumber(res, "dx");
    cocol = PQfnumber(res, "co");
    chcol = PQfnumber(res, "ch");
    tscol = PQfnumber(res, "ts");
    movescol = PQfnumber(res, "moves");
    plrolecol = PQfnumber(res, "role");
    plracecol = PQfnumber(res, "race");
    plgendcol = PQfnumber(res, "gender");
    plaligncol = PQfnumber(res, "alignment");
    namecol = PQfnumber(res, "plname");
    leveldesccol = PQfnumber(res, "level_desc");

    entries = calloc((*length), sizeof (struct nh_board_entry));
    for (i = 0; i < *length; i++) {
        tmp = strtol(PQgetvalue(res, i, levelcol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].level = tmp;

        tmp = strtol(PQgetvalue(res, i, depthcol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].depth = tmp;

        tmp = strtol(PQgetvalue(res, i, hpcol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].hp = tmp;

        tmp = strtol(PQgetvalue(res, i, hpmaxcol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].hpmax = tmp;

        tmp = strtol(PQgetvalue(res, i, encol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].en = tmp;

        tmp = strtol(PQgetvalue(res, i, enmaxcol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].enmax = tmp;

        tmp = strtol(PQgetvalue(res, i, wicol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].wi = tmp;

        tmp = strtol(PQgetvalue(res, i, incol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].in = tmp;

        tmp = strtol(PQgetvalue(res, i, stcol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].st = tmp;

        tmp = strtol(PQgetvalue(res, i, dxcol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].dx = tmp;

        tmp = strtol(PQgetvalue(res, i, cocol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].co = tmp;

        tmp = strtol(PQgetvalue(res, i, chcol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].ch = tmp;

        tmp = strtol(PQgetvalue(res, i, movescol), NULL, 10);
        if (errno != 0 || tmp > INT_MAX) {
            log_msg("Error converting string to int");
            return NULL;
        }
        entries[i].moves = tmp;


        plrole = PQgetvalue(res, i, plrolecol);
        plrace = PQgetvalue(res, i, plracecol);
        plgend = PQgetvalue(res, i, plgendcol);
        plalign = PQgetvalue(res, i, plaligncol);
        name = PQgetvalue(res, i, namecol);
        leveldesc = PQgetvalue(res, i, leveldesccol);
        ts = PQgetvalue(res, i, tscol);

        strcpy(entries[i].plrole, plrole);
        strcpy(entries[i].plrace, plrace);
        strcpy(entries[i].plgend, plgend);
        strcpy(entries[i].plalign, plalign);
        strcpy(entries[i].name, name);
        strcpy(entries[i].leveldesc, leveldesc);
        strcpy(entries[i].lastactive, ts);
    }

    PQclear(res);
    return entries;
}

/* db_pgsql.c */
