#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>

//GNU readline
 #include <readline/readline.h>
 #include <readline/history.h>
 
#include <editline/readline.h>

void * xmalloc(size_t size);
void initialize_readline();

/* Forward declarations. */
char *stripwhite(char *string);

/* When non-zero, this means the user is done using this program. */

int main(int argc, char **argv) {
	char *line, *s;

	setlocale(LC_CTYPE, "");

	initialize_readline(); /* Bind our completer. */

	stifle_history(7);

	/* Loop reading and executing lines until the user quits. */
	while (1) {
		line = readline("FileMan: ");
		printf("%s\n",line);
		/* Remove leading and trailing whitespace from the line.
		 Then, if there is anything left, add it to the history list
		 and execute it. */
		s = stripwhite(line);

		if (*s) {

			char* expansion;
			int result;

			result = history_expand(s, &expansion);

			if (result < 0 || result == 2) {
				fprintf(stderr, "%s/n", expansion);
			} else {
				add_history(expansion);
			}
			free(expansion);
		}

		free(line);
	}
	exit(0);

	return 0;
}

/* Look up NAME as the name of a command, and return a pointer to that
 command.  Return a NULL pointer if NAME isn't a command name. */

/* Strip whitespace from the start and end of STRING.  Return a pointer
 into STRING. */
char * stripwhite(char *string) {
	register char *s, *t;

	for (s = string; isspace(*s); s++)
		;

	if (*s == 0)
		return (s);

	t = s + strlen(s) - 1;
	while (t > s && isspace(*t))
		t--;
	*++t = '\0';

	return s;
}

/* **************************************************************** */
/*                                                                  */
/*                  Interface to Readline Completion                */
/*                                                                  */
/* **************************************************************** */

char *command_generator(const char *, int);
char **fileman_completion(const char *, int, int);

/* Tell the GNU Readline library how to complete.  We want to try to
 complete on command names if this is the first word in the line, or
 on filenames if not. */
void initialize_readline() {
	/* Allow conditional parsing of the ~/.inputrc file. */
	rl_readline_name = "FileMan";

	/* Tell the completer that we want a crack first. */
	rl_attempted_completion_function = fileman_completion;
}

/* Attempt to complete on the contents of TEXT.  START and END
 bound the region of rl_line_buffer that contains the word to
 complete.  TEXT is the word to complete.  We can use the entire
 contents of rl_line_buffer in case we want to do some simple
 parsing.  Returnthe array of matches, or NULL if there aren't any. */
char ** fileman_completion(const char* text, int start, int end) {
	char **matches;

	matches = (char **) NULL;

	/* If this word is at the start of the line, then it is a command
	 to complete.  Otherwise it is the name of a file in the current
	 directory. */
	if (start == 0)
		/* TODO */
		matches = rl_completion_matches(text, command_generator);
	/* matches = rl_completion_matches (text, command_generator); */

	return (matches);
}

/* Generator function for command completion.  STATE lets us
 know whether to start from scratch; without any state
 (i.e. STATE == 0), then we start at the top of the list. */
char * command_generator(const char *text, int state) {
	/* If this is a new word to complete, initialize now.  This
	 includes saving the length of TEXT for efficiency, and
	 initializing the index variable to 0. */
	/* If no names matched, then return NULL. */
	return ((char *) NULL);
}

void * xmalloc(size_t size) {
	register void *value = (void*) malloc(size);
	if (value == 0)
		fprintf(stderr, "virtual memory exhausted");
	return value;
}