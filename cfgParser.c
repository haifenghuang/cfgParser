#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cfgParser.h"

typedef struct tagSections{
    struct tagOptions *options;
    struct tagSections *next;
    char name[256];
}Sections;

typedef struct tagOptions{
    struct tagOptions *next;
    char name[256];
    char value[1024];
}Options;

static Sections gSections, *gLastSection;
static Options *gLastOption;
static char buf[1024];
static char optionKey[256];
static int meetSection;
static int line;

//uncomment this if you want strict parse
//#define STRICT_PARSE

/* Rules for describing the state changes and associated actions for the FSM. */
struct rule {
  int state;
  int c;
  int new_state;
  int (*state_action)(int state, int symbol);
};

static int ParseError(int state, int symbol)
{
  printf("Parsing Error, state[%d], symbol=[%d], line=[%d]\n", 
      state, symbol, line);
  return 1;
}

static int newSection(int state, int symbol)
{
  if (symbol != '[') sprintf(buf+strlen(buf), "%c", symbol);

  meetSection = 1;
  return 0;
}

static int endSection(int state, int symbol)
{
  Sections *cur;
  Sections *sec = calloc(1, sizeof(Sections));
  if (sec == NULL)
    {
      printf("Section allocation error\n");
      return 1;
    }

  if (gSections.next == NULL) /* no section yet */
      gSections.next = sec;
  else /* already have section(s) */
    {
      cur = &gSections;
      while (cur->next != NULL) cur = cur->next;
      cur->next = sec;
    }

  gLastSection = sec;
  strcpy(gLastSection->name, buf);
  memset(buf, 0x00, sizeof(buf));
  return 0;
}

static int newOption(int state, int symbol)
{
  sprintf(optionKey+strlen(optionKey), "%c", symbol);
  return 0;
}

static int newValue(int state, int symbol)
{  
  Options *cur;
  Options *option = calloc(1, sizeof(Options));
  if (option == NULL)
  {
    printf("Option allocation error\n");
    return 1;
  }
  if (meetSection) /* already has meet section(s) */    
  {
    if (gLastSection->options == NULL) 
      gLastSection->options = option;
    else
      gLastOption->next = option;
  }
  else
  {
    gSections.name[0]='\0';
    if (gSections.options == NULL) /* no option yet */
      gSections.options = option;
    else /* already have option(s) */
    {
      cur = gSections.options;
      while (cur->next != NULL)  cur = cur->next;
      cur->next = option;
    }
  }
  gLastOption = option;
  strcpy(gLastOption->name, optionKey);
  memset(optionKey, 0x00, sizeof(optionKey));

  if (symbol != '=')
    sprintf(gLastOption->value+strlen(gLastOption->value), "%c", symbol);

  /* InStr state, if string contains multiple lines. */
  if (symbol == '\n') line++;

  return 0;
}

static int newLine(int state, int symbol)
{
  line++;
  return 0;
}

int cfgParse(const char *filename)
{
  int in, i, state = 0;
  int result = 0;
  const struct rule *p;
  FILE *fp;

  /* The FSM itself. */
  static const struct rule fsm[] = {
        /* state = [InOptions] - 0 */
        /* key name could not hava space */
        {0, '\n', 0, newLine},
        {0, ' ',  0, 0}, /* skip whitespace */
        {0, '\t', 0, 0}, /* skip whitespace */
        {0, ';',  1, 0},
        {0, '#',  1, 0},
        {0, '=',  4, newValue},
        {0, '[',  2, newSection},
        {0, 0,    0, newOption},

        /* state = [Comment] - 1 */
        {1, '\n', 0, newLine},
        {1, 0,    1, 0},

        /* state = [InSection] - 2 */
        /* section name could have space */
        {2, ']', 3, endSection},
#ifdef STRICT_PARSE
        {2, ';', 7, 0}, /* seciton could not contail ';' */
        {2, '#', 7, 0}, /* seciton could not contail '#' */
#endif
        {2, 0,   2, newSection}, /* seciton could contail '#' or ';' */

        /* state = [EndSection] - 3 */
        {3, ';',  1, 0},
        {3, '#',  1, 0},
        {3, 0,    3, 0}, //? "[xxxx] yyyy", then yyyy will be ignored

        /* state = [InValue] - 4 */
        {4, '#',  1, 0},
        {4, ';',  1, 0},
        {4, '"',  5, 0},
        {4, ' ',  4, 0}, /* skip whitespace */
        {4, '\t', 4, 0}, /* skip whitespace */
        {4, '\n', 0, newLine},
        {4, 0,    4, newValue},

        /* state = [InStr] - 5 */
        /* when string value across multiple lines, 
         * then 'line' report is not correct.
         * */
        {5, '\\', 6, 0},
        {5, '"',  4, 0},
        {5, 0,    5, newValue},

        /* state = [StrQuote] - 6 */
        {6, 0,  5, newValue},

        /* state =[Invalid] - 7 */
        {7, 0, -1, ParseError},
    };

  fp = fopen(filename, "r");
  if (fp == NULL)
    {
      fprintf(stderr, "open file %s error\n", filename);
      return 1;
    }

  line = 1;
  while((in = getc(fp)) != EOF) {
      for (i = 0; i< sizeof(fsm) /sizeof(fsm[0]); ++i)
        {
          p = &fsm[i];
          if (p->state == state && (p->c == in || p->c == 0))
            {
              /*
              if (p->new_state == -1)
                {
                  printf("Parser error\n");
                  return 1;
                }
              */

              //printf("state=[%d] c=[%d] in=[%d], line=[%d] ", state, p->c, in, line);
              //state action
              if (p->state_action != NULL)
                {
                  result = (p->state_action)(state, in);
                  if (result != 0) 
                    {
                      fclose(fp);
                      return 1;
                    }
                }

              state = p->new_state;
              //printf("next state=[%d] \n", state);
              break;
            }
        } /* end for */
    } /* end while */
  if (state != 0)
    {
      printf("input is malformed, ends inside comment or literal \n");
      fclose(fp);
      return 1;
    }

  fclose(fp);
  return 0;
}

void cfgPrint()
{
  Options *opt;
  Sections *sec;

  printf("\n==================RESULT:GLOBAL======================\n");
  if (gSections.name[0] == '\0') /* global options */
    {
      opt = gSections.options;
      while (opt != NULL)
        {
          printf("options key[%s], value=[%s]\n", opt->name, opt->value);
          opt = opt->next;
        }
    }
  printf("\n\n==================RESULT:SECTIONS======================");
  sec = gSections.next;
  while(sec != NULL)
    {
      printf("\nsection name =[%s]\n", sec->name);

      opt = sec->options;
      while (opt != NULL)
        {
          printf("\toptions key[%s], value=[%s]\n", opt->name, opt->value);
          opt = opt->next;
        }
      sec = sec->next;
    }
}

void cfgFree()
{
  Options *opt, *curOpt;

  Sections *sec, *curSec;

  if (gSections.name[0] == '\0') /* global options */
    {
      opt = gSections.options;
      while (opt != NULL)
        {
          curOpt = opt;
          opt = opt->next;
          free(curOpt);
        }
    }

  sec = gSections.next;
  while(sec != NULL)
    {
      opt = sec->options;
      while (opt != NULL)
        {
          curOpt = opt;
          opt = opt->next;
          free(curOpt);
        }

      curSec = sec;
      sec = sec->next;
      free(curSec);
    }
}

int cfgGet(const char *section, const char *key, char *out)
{
  Options *opt;
  Sections *sec;

  out[0] = '\0';
  if (section == NULL || *section == '\0') /* global options */
    {
      opt = gSections.options;
      while (opt != NULL)
        {
          if (strcmp(opt->name, key) == 0)
            {
              strcpy(out, opt->value);
              return 0;
            }
          opt = opt->next;
        } /* end while */
      return 1;
    }

  sec = gSections.next;
  while(sec != NULL)
    {
      if (strcmp(sec->name, section) == 0)
        {
          opt = sec->options;
          while (opt != NULL)
            {
              if (strcmp(opt->name, key) == 0)
                {
                  strcpy(out, opt->value);
                  return 0;
                }
              opt = opt->next;
            } /* end inner while */
        }

      sec = sec->next;
    } /* end outer while */
  return 1;
}
