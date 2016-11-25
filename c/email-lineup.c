#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <argp.h>
#include <string.h>

const char *argp_program_version = "email-lineup 1.0";

static char doc[] = 
"Reads in an sqlite file and emails the \
weekly lineup.";

static char args_doc[] = "SQLITE_FILE WEEK SUBJECT TO";

static struct argp_option options[] = {
	{"send", 's', 0, 0, "Send Email"},
	{0}
};

struct arguments {
	char *args[4];
	int send;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;

	switch (key) {
		case 's':
			arguments->send = 1;
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num >= 4) {
				argp_usage(state);
			}
			arguments->args[state->arg_num] = arg;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 4)
				argp_usage(state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}
	
static struct argp argp = {options, parse_opt, args_doc, doc};

void send_lineup(sqlite3 *, int, const char *, const char *, int);

int main(int argc, char **argv)
{
	struct arguments arguments;
	arguments.send = 0;

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
	char *subject = arguments.args[2];
	char *to = arguments.args[3];
	
	if (arguments.send) {
        	send_lineup(db, wk, subject, to, arguments.send);
	} else {
		printf("\nLineup to be sent (use -s to send):\n");
		send_lineup(db, wk, subject, to, 0);
	}

        sqlite3_close(db);

        exit(0);
}

void send_lineup(sqlite3 *db, int week, const char *subject, const char *to, int send)
{
        int rc = 0;
	char body[6][120];
	int count = 0;
        const char *position = NULL;
        const char *first_name = NULL;
        const char *last_name = NULL;
        const char *team = NULL;
        const char *lineup =
"SELECT position, first_name, last_name, team \
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

        rc = sqlite3_bind_int(stmt, 1, week);

        if(rc != SQLITE_OK) {
                printf("Problem with sqlite3_bind_text: %s\n", sqlite3_errmsg(db));
                exit(-1);
        }
	
	printf("\nWeek %d lineup:\n", week);
        while(sqlite3_step(stmt) == SQLITE_ROW) {
                position = (const char*)sqlite3_column_text(stmt, 0);
                first_name = (const char*)sqlite3_column_text(stmt, 1);
                last_name = (const char*)sqlite3_column_text(stmt, 2);
                team = (const char*)sqlite3_column_text(stmt, 3);
                sprintf(body[count], "%s %s %s %s\n", position, first_name, last_name, team);
		count++;
        }
	for (int i=0; i<count; i++) 
		printf("\t%s", body[i]);

	if (send) {
		char mail_cmd[250];
		sprintf(mail_cmd, "echo \"%s%s%s%s%s%s%s\" |mail -s \"%s\" %s",
			body[0],
			body[1],
			body[2],
			body[3],
			body[4],
			body[5],
			body[6],
			subject,
			to);
		system(mail_cmd);
	}
	
        sqlite3_finalize(stmt);
}
