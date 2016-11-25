#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <argp.h>

const char *argp_program_version = "count-players 2.0";

static char doc[] = 
"Reads in an sqlite file and query's \
the number of times a player has been used.";

static char args_doc[] = "SQLITE_FILE";

static struct argp_option options[] = {
	{"qb",  'q', 0, 0, "Print QB usage count"},
	{"rb",  'r', 0, 0, "Print RB usage count"},
	{"wr",  'w', 0, 0, "Print WR usage count"},
	{"te",  't', 0, 0, "Print TE usage count"},
	{"k",   'k', 0, 0, "Print K usage count"},
	{"all", 'a', 0, 0, "Print all positions usage count"},
	{0}
};

struct arguments {
	char *args[1];
	int qb, rb, wr, te, k, all;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;

	switch (key) {
		case 'a':
			arguments->all = 1;
			break;
		case 'q':
			arguments->qb = 1;
			break;
		case 'r':
			arguments->rb = 1;
			break;
		case 'w':
			arguments->wr = 1;
			break;
		case 't':
			arguments->te = 1;
			break;
		case 'k':
			arguments->k = 1;
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num >= 1) {
				argp_usage(state);
			}
			arguments->args[state->arg_num] = arg;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 1)
				argp_usage(state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}
	
static struct argp argp = {options, parse_opt, args_doc, doc};

void print_count(sqlite3 *, const char *);

int main(int argc, char **argv)
{
	struct arguments arguments;
	arguments.all = 0;
	arguments.qb = 0;
	arguments.rb = 0;
	arguments.wr = 0;
	arguments.te = 0;
	arguments.k = 0;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);

        char *file = arguments.args[0];
        sqlite3 *db;
        int rc = 0;

        sqlite3_initialize();
        rc = sqlite3_open(file, &db);

        if(rc) {
                fprintf(stderr, "Can't open database: %s\n",
                                sqlite3_errmsg(db));
                exit(0);
        } else {
                fprintf(stderr, "Opened database successfully\n");
        }

	if (arguments.all) {
        	print_count(db, "QB");
        	print_count(db, "RB");
        	print_count(db, "WR");
        	print_count(db, "TE");
        	print_count(db, "K");
	} else {
		if (arguments.qb)
			print_count(db, "QB");
		if (arguments.rb)
			print_count(db, "RB");
		if (arguments.wr)
			print_count(db, "WR");
		if (arguments.te)
			print_count(db, "TE");
		if (arguments.k)
			print_count(db, "K");
	}

        sqlite3_close(db);

        exit(0);
}

void print_count(sqlite3 *db, const char *pos)
{
        int rc = 0;
        const char *position = NULL;
        const char *first_name = NULL;
        const char *last_name = NULL;
        const char *team = NULL;
        const char *count = NULL;
        const char *player_count =
"SELECT position, first_name, last_name, team, COUNT(*) \
FROM data \
WHERE position = ? \
GROUP BY position, first_name, last_name, team \
HAVING COUNT(*) >= 1 \
ORDER BY COUNT(*) DESC";
        sqlite3_stmt *stmt = NULL;

        rc = sqlite3_prepare_v2(
                db,
                player_count,
                -1,
                &stmt,
                NULL);

        if(rc != SQLITE_OK) {
                printf("Problem with sqlite3_prepare_v2: %s\n", sqlite3_errmsg(db));
                exit(-1);
        }

        rc = 0;

        rc = sqlite3_bind_text(stmt, 1, pos, -1, SQLITE_STATIC);

        if(rc != SQLITE_OK) {
                printf("Problem with sqlite3_bind_text: %s\n", sqlite3_errmsg(db));
                exit(-1);
        }
	
	printf("\nPosition: %s\n", pos);
        while(sqlite3_step(stmt) == SQLITE_ROW) {
                position = (const char*)sqlite3_column_text(stmt, 0);
                first_name = (const char*)sqlite3_column_text(stmt, 1);
                last_name = (const char*)sqlite3_column_text(stmt, 2);
                team = (const char*)sqlite3_column_text(stmt, 3);
                count = (const char*)sqlite3_column_text(stmt, 4);
                printf("\t%s %s %s %s %s\n", position, first_name, last_name, team, count);
        }

        sqlite3_finalize(stmt);
}
