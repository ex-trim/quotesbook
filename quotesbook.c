/* quotesbook -- collect in db and print quotes on the standard output.

   The MIT License (MIT)
   Copyright (C) 2016 Ex-Trim (extrimmail@gmail.com).

   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to use,
   copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
   Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sqlite3.h>
#include <getopt.h>
#include <pwd.h>
#include <sys/stat.h>

/* The official name of this program */
#define PROGRAM_NAME "quotesbook"

/* Buffer size for strings */
#define BUF_SIZE 1024

/* Path length */
#define PATH_SIZE 150

/* status and options codes */
#define O_EOK 0   /* operation ends normal    */
#define O_ABR 6   /* abort operation          */
#define O_NUM 11  /* numerate option          */
#define O_DEF 10  /* default option           */
#define O_WRT 12  /* write to db option       */
#define O_CHK 15  /* check record option      */
#define R_EMT 21  /* empty resource           */
#define E_ODB 31  /* open db error            */
#define E_MEM 32  /* memory allocation error  */
#define E_SQL 33  /* sql error                */
#define E_WRT 34  /* db write error           */
#define E_WUD 35  /* wrong user data          */
#define E_RIN 36  /* read input error         */
#define E_NOD 37  /* no data error            */

/* database filename */
char DB_FILENAME[PATH_SIZE] = {0};


/*
* TODO:
*
*   убрать вывод из query_db и возвращать только значение(массив)
*       - тогда можно будет убрать опцию нумерации (посчитаем количество эл-тов массива)
*/


/*
* Initial environment variables
*/
int p_init()
{
  struct passwd *pw = getpwuid(getuid());

  const char *homedir = pw->pw_dir;

  char confdir[PATH_SIZE] = {0};
  strcat(confdir, homedir);
  strcat(confdir, "/.");
  strcat(confdir, PROGRAM_NAME);

  /* check application directory in user home */
  struct stat st = {0};

  if (stat(confdir, &st) == -1)
  {
    mkdir(confdir, 0700);
  }

  /* set quotes database file path */
  strcat(DB_FILENAME, confdir);
  strcat(DB_FILENAME, "/quotes.db");

  /*
  * if database not exist in user home (application) directory 
  * but exist sample_quotes.db move last to application directory
  */
  if ((stat(DB_FILENAME, &st) == -1) && (stat("sample_quotes.db", &st) == 0))
    rename("sample_quotes.db", DB_FILENAME);

  return O_EOK;
}


/*
* Connect to db and exec sql
* Q_TYPE - query type
*
*/
int db_query(char *sql, const int Q_TYPE)
{
  sqlite3 *db;
  char *err_msg = 0;
  sqlite3_stmt *res;
  const char *tail;

  int rc = sqlite3_open(DB_FILENAME, &db);

  if ( rc != SQLITE_OK )
  {
    fprintf (stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);

    return E_ODB;
  }

  if (sql == NULL)
  {
    perror("Fail to allocate sql");
    free(sql);
    return E_MEM;
  }

  rc = sqlite3_prepare_v2( db, sql, strlen(sql), &res, &tail );

  if ( rc != SQLITE_OK )
  {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);

    return E_SQL;
  }

  /* return check value: 0 - if record in db, other if no */
  if (Q_TYPE == O_CHK)
  {
    if (sqlite3_step(res) == SQLITE_ROW)
    {
      sqlite3_finalize(res);
      sqlite3_close(db);

      return O_EOK;
    }
    else
    {
      sqlite3_finalize(res);
      sqlite3_close(db);

      return R_EMT;
    }
  }

  while(sqlite3_step(res) == SQLITE_ROW)
  {
    if (Q_TYPE == O_NUM)
    {
      printf("%s: ", sqlite3_column_text(res, 0));
      printf("%s\n", sqlite3_column_text(res, 1));
    }

    if (Q_TYPE == O_DEF)
    {
      printf("%s\n", sqlite3_column_text(res, 1));
    }
  }

  sqlite3_finalize(res);
  sqlite3_close( db );

  return O_EOK;
}


/*
* Create db if not exist
*/
int db_create()
{
  char *sql = "CREATE TABLE IF NOT EXISTS Quotes(\
              'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,\
              'quote' TEXT);";

  if (db_query(sql, O_WRT) != O_EOK)
  {
    perror("Can't create db table");
    free(sql);
    return E_WRT;
  }

  return O_EOK;
}


/*
* Check record is in db
* return ecode from db_query function
* which must be O_EOK if record with id is there
* or error code
*/
int db_check_value(int id)
{
  char *sql;

  if (id <= 0)
  {
    fprintf(stderr, "Wrong identifier: %d\n", id);

    return E_WUD;
  }

  if (asprintf(&sql, "SELECT * FROM Quotes WHERE id='%d';", id) < 0)
      return NULL;

  int ecode = db_query(sql, O_CHK);
  free(sql);

  return ecode;
}


/*
* Strip spaces from start and end of string
* text must be null-terminated
* return trimed string
*/
char *str_trim(char *text)
{
  /* skip spaces at start */
  while(isspace((unsigned char)*text))
    text++;

  if (*text == 0)
    return text;

  /* strip trailing newline character */
  size_t end = strlen(text) - 1;

  int i;

  for (i=strlen(text); i>=0; i--)
  {
    if (isspace((unsigned char)text[end]))
      end--;
  }
  text[end + 1] = '\0';

  return text;
}


/*
* Read user input from stdin
* while EOF/[ctrl^D] pressed
* return ptr to string
*/
char *get_user_input()
{
  char buffer[BUF_SIZE];
  size_t content_size = 1;

  char *content = (char *) malloc(BUF_SIZE);

  if (content == NULL)
  {
    perror("Fail to allocate content");
    exit(E_MEM);
  }
  /* null-terminate string */
  content[0] = '\0';
  /* while EOF read input and add to content */
  while (fgets(buffer, BUF_SIZE, stdin))
  {
    content_size += strlen(buffer);
    content = realloc(content, content_size);

    if (content == NULL)
    {
      perror("Failed to reallocate content while get input");
      free(content);
      exit(E_MEM);
    }

    strcat(content, buffer);
  }

  /* strip lead and trailing space characters */
  content = str_trim(content);

  if(ferror(stdin))
  {
    free(content);
    perror("Error reading from stdin");
    exit(E_RIN);
  }

  return content;
}

/*
* Check thar entry with id exist in db
* Get confirmation from user if is
*/
int get_confirmation()
{
  char u_answer;
  char *question = "Are you sure? [y|Y]:";

  printf("%s", question);
  scanf("%c", &u_answer);

  if (u_answer == 'y' || u_answer == 'Y')
  {
    return O_EOK;
  }
  else
  {
    return O_ABR;
  }
}

/*
* Put text in db
*/
int db_write(char *text)
{
  if (db_create() != O_EOK)
  {
    return E_WRT;
  }

  char *sql;

  if (asprintf(&sql, "INSERT INTO Quotes (Quote) VALUES('%s');", text) < 0)
    return NULL;

  if (db_query(sql, O_WRT) != O_EOK)
  {
    fprintf(stderr, "There was an error in db_write method\n");
    return E_WRT;
  }

  free(sql);

  return O_EOK;
}

/*
* Read entry with id from db
*/
int db_read(int id)
{
  char *sql;

  if (id != 0)
  {
    if (asprintf(&sql, "SELECT * FROM Quotes WHERE id='%d';", id) < 0)
      return NULL;
  }
  else
  {
    char *query = "SELECT * FROM Quotes;";

    sql = (char *) malloc(strlen(query) + 1);

    if (sql == NULL)
    {
      fprintf(stderr, "Can't malloc sql query string\n");

      return E_MEM;
    }
    strcpy(sql, query);
  }

  int ecode = db_query(sql, O_NUM);

  if (ecode != O_EOK)
  {
    fprintf (stderr, "There was an error in db_read method\n");
    free(sql);

    return ecode;
  }

  free(sql);

  return O_EOK;
}

/*
* Get user confirmation and delete entry with id from db if exist
*/
int db_delete(int id)
{
  char *sql;

  if (id > 0)
  {
    if (asprintf(&sql, "DELETE FROM Quotes WHERE id='%d';", id) < 0)
      return NULL;
  }
  else
  {
    fprintf(stderr, "Wrong identifier. Enter correct one...\n");

    return E_WUD;
  }

  int ecode = get_confirmation();

  if (ecode == O_EOK)
  {
    ecode = db_query(sql, O_WRT);

    if (ecode == O_EOK)
    {
      printf("Entry with #%d was deleted!\n", id);
    }
    else
    {
      fprintf(stderr, "There was an error in delete method\n");

      return ecode;
    }
  }

  free(sql);

  return O_EOK;
}

/*
* Get random entry from db
*/
int get_random()
{
  char *sql = "SELECT * FROM Quotes ORDER BY RANDOM() LIMIT 1;";

  int ecode = db_query(sql, O_DEF);

  if (ecode != O_EOK)
  {
    fprintf(stderr, "There was an error in get_random method\n");

    return ecode;
  }

  return O_EOK;
}


/*
* Print help/program usage
*/
int print_usage()
{
  printf("\
Usage: %s [adhln?] <args>\n", PROGRAM_NAME);

  fputs("\
  \n\
  -l                          list all entries in db\n\
  -d <num>                    delete entry with <num>\n\
  -a                          append new record to db\n\
  -h, -?                      show this help\n\
", stdout);

  printf("\
\n\
Examples:\n\n\
  %s -n 10            shows record #10 from db\n\
  %s -l               shows list of all records\
\n", PROGRAM_NAME, PROGRAM_NAME);

  exit(O_EOK);
}


/*
* Program body
*/
int main(int argc, char **argv)
{
  int id = 0;
	int opt = 0;
  int dflag = 0;
  int nflag = 0;
	int lflag = 0;
	int aflag = 0;
  extern char *optarg;

  /* initial application settings */
  p_init();

  /* get options */
	while ((opt = getopt( argc, argv, "ad:n:hl" )) != -1)
	{
		switch (opt)
		{
      case 'd':
        dflag = 1;
        id = atoi(optarg);
        break;
			case 'n':
        nflag = 1;
				id = atoi(optarg);
				break;
			case 'l':
				lflag = 1;
				break;
			case 'a':
				aflag = 1;
				break;
      case 'h':
			case '?':
				print_usage();
		}
	}

	if (nflag == 1)
  {
    if (db_check_value(id) != O_EOK)
    {
      fprintf(stderr, "There is no such value in db! Look all entry list by -l key\n");

      return E_NOD;
    }
    else
    {
      db_read(id);
    }
  }
  else if (dflag == 1)
  {
    if (db_check_value(id) != O_EOK)
    {
      fprintf(stderr, "There is no such value in db! Look all entry list by -l key\n");

      return E_NOD;
    }
    else
    {
      db_delete(id);
    }
  }
  else if (aflag == 1)
	{
	  char *text = (char *) malloc(BUF_SIZE);
    /* if there is text after -a flag
    * putting it all together
    * and write to db
    */
	  if (optind < argc)
		{
		  size_t s_length = 1;

      while(optind < argc)
      {
        s_length += strlen(argv[optind]) + 1;
        text = realloc (text, s_length);
        strcat(text, argv[optind]);
        strcat(text, " ");
        optind++;
      }
      text = str_trim(text);
		}
		/* if no text after -a flag get input from user */
		else
		{
		  printf ("Enter string below [ctrl + d] to end input\n");

      text = get_user_input();
		}

    if (strlen(text)>10)
    {
      int ecode = db_write(text);

      if (ecode != O_EOK)
      {
        fprintf(stderr, "Error write to db \"%s\"\n", text);
        free(text);

        return ecode;
      }
      else
      {
        printf("\"%s\"\n%d symbol(s) writed to db succeful!\n", text, strlen(text));
        free(text);

        return O_EOK;
      }
    }
    else
    {
      printf("Too short string. Skiped...\n");
      free (text);

      return O_EOK;
    }
	}
  else if (lflag == 1)
  {
    db_read(0);
  }
  else
  {
    get_random();
  }
  free(optarg);

	return O_EOK;
}
