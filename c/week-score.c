#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <argp.h>
#include <string.h>

const char *argp_program_version = "week-score 1.0";

static char doc[] = 
"Reads in an sqlite file and week and then\
computes the score for the week";

static char args_doc[] = "SQLITE_FILE WEEK";


//static struct argp_option options[] = {NULL}

struct arguments {
	char *args[2];
	//int qb, rb, wr, te, k, all;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;

	switch (key) {
		case ARGP_KEY_ARG:
			if (state->arg_num >= 2) {
				argp_usage(state);
			}
			arguments->args[state->arg_num] = arg;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 2)
				argp_usage(state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}
	
static struct argp argp = {0, parse_opt, args_doc, doc};

void print_score(sqlite3 *, int);
int calculate_score(const char *, int, int, int, int, int, int, int, int, int);

int main(int argc, char **argv)
{
	struct arguments arguments;

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

	int wk = atoi(arguments.args[1]);
	print_score(db, wk);

        sqlite3_close(db);

        exit(0);
}

void print_score(sqlite3 *db, int wk)
{
        int rc = 0;
        int week;
	const char *position = NULL;
        const char *first_name = NULL;
        const char *last_name = NULL;
        const char *team = NULL;
	int tds;
	int ret_tds;
	int fgs;
	int xps;
	int two_pts;
	int pass_yds;
	int rush_yds;
	int rec_yds;
	int ints;    
	int pos_score;
	int total_score = 0;
    
        const char *lineup =
"SELECT * \
FROM data \
WHERE week = ?";
        sqlite3_stmt *stmt = NULL;

        rc = sqlite3_prepare_v2(
                db,
                lineup,
                -1,
                &stmt,
                NULL);

        if(rc != SQLITE_OK) {
                printf("Problem with sqlite3_prepare_v2: %s\n", sqlite3_errmsg(db));
                exit(-1);
        }

        rc = 0;

        rc = sqlite3_bind_int(stmt, 1, wk);

        if(rc != SQLITE_OK) {
                printf("Problem with sqlite3_bind_text: %s\n", sqlite3_errmsg(db));
                exit(-1);
        }
	
	printf("\nScoore for week: %d\n", wk);
        while(sqlite3_step(stmt) == SQLITE_ROW) {
		week = (int)sqlite3_column_int(stmt, 0);
                position = (const char*)sqlite3_column_text(stmt, 1);
                first_name = (const char*)sqlite3_column_text(stmt, 2);
                last_name = (const char*)sqlite3_column_text(stmt, 3);
                team = (const char*)sqlite3_column_text(stmt, 4);
		tds = (int)sqlite3_column_int(stmt, 5);
		ret_tds = (int)sqlite3_column_int(stmt, 6);
		fgs = (int)sqlite3_column_int(stmt, 7);
		xps = (int)sqlite3_column_int(stmt, 8);
		two_pts = (int)sqlite3_column_int(stmt, 9);
		pass_yds = (int)sqlite3_column_int(stmt, 10);
		rush_yds = (int)sqlite3_column_int(stmt, 11);
		rec_yds = (int)sqlite3_column_int(stmt, 12);
		ints = (int)sqlite3_column_int(stmt, 13);
                printf("\t%d %s %s %s %s", 
			week, 
			position, 
			first_name, 
			last_name,
			team); 
		pos_score = calculate_score(position, tds, ret_tds, fgs, xps, two_pts, pass_yds, rush_yds, rec_yds, ints);
		printf(" %d\n", pos_score);
		total_score += pos_score;
        }
	
	printf("\n\n\tWeekly score: %d\n", total_score);

        sqlite3_finalize(stmt);
}

// I know, this is a terrible function. 
int calculate_score(const char *position, 
		int tds,
		int ret_tds,
		int fgs,
		int xps,
		int two_pts,
		int pass_yds,
		int rush_yds,
		int rec_yds,
		int ints)
{
	int total_score = 0;
	int pass_score = 0;
	int rush_score = 0;
	int rec_score = 0;
	int rc = strcmp(position, "WR\0");
	
	total_score = (tds*6) + (ret_tds*6) + (fgs*3) + xps + (two_pts*2) - (ints*2);

	if (pass_yds >= 300)
		pass_score = 6 + (((pass_yds-300) / 50)*3);
	total_score += pass_score;

	if (rush_yds >= 100)
		rush_score = 6 + (((rush_yds-100) / 50)*3);
	total_score += rush_score;

	if ((rc == 0) && rec_yds >= 100) {
		rec_score = 6 + (((rec_yds-100) / 50)*3);
	}
	
	if (rc != 0) {
		rec_score = (rec_yds / 50)*3;
	}
	total_score += rec_score;
		
	return total_score;
}
