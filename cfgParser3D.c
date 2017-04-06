#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct tagSections{
    char name[256];
    struct tagOptions *options;
    struct tagSections *next;
}Sections;

typedef struct tagOptions{
    char name[256];
    char value[1024];
    struct tagOptions *next;
}Options;

static Sections gSections, *gLastSection;
static Options *gLastOption;
static char buf[1024];
static char optionKey[256];
static int meetSection;
static int line;

static int ParseError(int state, int symbol)
{
  printf("Parsing Error, state[%d], symbol=[%d]\n", state, symbol);
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
      while (cur->next != NULL) cur = cur->next;
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

/* action */
#define NS  1 /* newSection */
#define ES  2 /* endSection */
#define NO  3 /* newOption */
#define NV  4 /* newValue */
#define PE  5 /* ParseError */
#define NL  6 /* newLine */
static int (*state_action[7])(int state, int symbol) = 
{
  NULL, 
  newSection, //1
  endSection, //2
  newOption,  //3
  newValue,   //4
  ParseError, //5
  newLine,    //6
};

int cfgParse(const char *filename)
{
  int in, state = 0;
  int result = 0;
  int actionIndex = 0;
  int k, j, iFound;
  FILE *fp;

  static const int stable[8][9][3] = { /* stable[symbol][state][actionIndex] */
     /* Init = 0    comment = 1   InSection=2   EndSetion=3   InValue=4    InStr=5        StrQuote=6 Invalid=7  */
     {{'\n', 0, NL}, {'\n', 0, NL},{']', 3, ES}, {';', 1, 0},  {'#', 1, 0}, {'\\', 6, 0},  {0, 5, NV},{0, -1, PE}},
     {{' ',  0, 0},  {0,    1, 0}, {0,   2, NS}, {'#', 1, 0},  {';', 1, 0}, {'"',  4, 0},  {0, 0, 0}, {0, 0, 0}},
     {{'\t', 0, 0},  {0,    0, 0}, {0,   0, 0},  {0,   3, 0},  {'"', 5, 0}, {0,    5, NV}, {0, 0, 0}, {0, 0, 0}},
     {{';',  1, 0},  {0,    0, 0}, {0,   0, 0},  {0,   0, 0},  {' ', 4, 0}, {0,    0, 0},  {0, 0, 0}, {0, 0, 0}},
     {{'#',  1, 0},  {0,    0, 0}, {0,   0, 0},  {0,   0, 0},  {'\t',4, 0}, {0,    0, 0},  {0, 0, 0}, {0, 0, 0}},
     {{'=',  4, NV}, {0,    0, 0}, {0,   0, 0},  {0,   0, 0},  {'\n',0, NL},{0,    0, 0},  {0, 0, 0}, {0, 0, 0}},
     {{'[',  2, NS}, {0,    0, 0}, {0,   0, 0},  {0,   0, 0},  {0,   4, NV},{0,    0, 0},  {0, 0, 0}, {0, 0, 0}},
     {{0,    0, NO}, {0,    0, 0}, {0,   0, 0},  {0,   0, 0},  {0,   0, 0}, {0,    0, 0},  {0, 0, 0}, {0, 0, 0}},
  };

  fp = fopen(filename, "r");
  if (fp == NULL)
    {
      fprintf(stderr, "open file %s error\n", filename);
      return 1;
    }

  line = 1;
  while((in = getc(fp)) != EOF) {
      k = j = iFound = 0;
      while(stable[j][state][0] != 0) {
          if (in == stable[j][state][0]) { // is special char
              iFound = 1;
              k = j;
              break;
          }
          j++;
      } /* end while */

      if (!iFound) k = j;
      if (state == -1) 
        {
          fclose(fp);
          return 1;
        }

      actionIndex = stable[k][state][2];
      if (actionIndex != 0) 
          if ((state_action[actionIndex])(state, in) != 0) 
            {
              fclose(fp);
              return 1;
            }

      state = stable[k][state][1];  //set next state

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
  int iFound = 0;

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

int main(int argc, char **argv) {
  int result = 0;
  Options *opt;
  Sections *sec;
  char out[1024];

  result = cfgParse("a.ini");
  if (result != 0) return 1;

  printf("\n==================GET RESULT:GLOBAL======================\n");
  memset(out, 0x00, sizeof(out));
  cfgGet(NULL, "aa", out);
  printf("Global section, aa=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet(NULL, "a", out);
  printf("Global section, a=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet(NULL, "b", out);
  printf("Global section, b=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet(NULL, "c", out);
  printf("Global section, c=[%s]\n", out);

  printf("\n==================GET RESULT:[ab;cdefg]======================\n");
  memset(out, 0x00, sizeof(out));
  cfgGet("ab;cdefg", "c", out);
  printf("Named section [ab;cdefg], c=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet("ab;cdefg", "d", out);
  printf("Named section [ab;cdefg], d=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet("ab;cdefg", "e", out);
  printf("Named section [ab;cdefg], e=[%s]\n", out);

  printf("\n==================GET RESULT:[xxxx]======================\n");
  memset(out, 0x00, sizeof(out));
  cfgGet("xxxx", "e", out);
  printf("Named section [xxxx], e=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet("xxxx", "m", out);
  printf("Named section [xxxx], m=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet("xxxx", "n", out);
  printf("Named section [xxxx], n=[%s]\n", out);

  //cfgPrint();
  cfgFree();
  return result;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct tagSections{
    char name[64];
    struct tagOptions *options;
    struct tagSections *next;
}Sections;

typedef struct tagOptions{
    char name[64];
    char value[1024];
    struct tagOptions *next;
}Options;

static Sections gSections, *gLastSection;
static Options *gLastOption;
static char buf[1024];
static int meetSection;
static int line;

static int ParseError(int state, int symbol)
{
  printf("Parsing Error, state[%d], symbol=[%d]\n", state, symbol);
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
          while (cur->next != NULL) cur = cur->next;
          cur->next = option;
        }
    }

  gLastOption = option;
  sprintf(gLastOption->name+strlen(gLastOption->name), "%c", symbol);

  return 0;
}

static int newValue(int state, int symbol)
{
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

/* action */
#define NS  1 /* newSection */
#define ES  2 /* endSection */
#define NO  3 /* newOption */
#define NV  4 /* newValue */
#define PE  5 /* ParseError */
#define NL  6 /* newLine */
static int (*state_action[7])(int state, int symbol) = 
{
  NULL, 
  newSection, //1
  endSection, //2
  newOption,  //3
  newValue,   //4
  ParseError, //5
  newLine,    //6
};

int cfgParse(const char *filename)
{
  int in, state = 0;
  int result = 0;
  int actionIndex = 0;
  int k, j, iFound;
  FILE *fp;

  static const int stable[8][9][3] = { /* stable[symbol][state][actionIndex] */
     /* Init = 0    comment = 1   InSection=2   EndSetion=3   InValue=4    InStr=5        StrQuote=6 Invalid=7  */
     {{'\n', 0, NL}, {'\n', 0, NL},{']', 3, ES}, {';', 1, 0},  {'#', 1, 0}, {'\\', 6, 0},  {0, 5, NV},{0, -1, PE}},
     {{' ',  0, 0},  {0,    1, 0}, {0,   2, NS}, {'#', 1, 0},  {';', 1, 0}, {'"',  4, 0},  {0, 0, 0}, {0, 0, 0}},
     {{'\t', 0, 0},  {0,    0, 0}, {0,   0, 0},  {0,   3, 0},  {'"', 5, 0}, {0,    5, NV}, {0, 0, 0}, {0, 0, 0}},
     {{';',  1, 0},  {0,    0, 0}, {0,   0, 0},  {0,   0, 0},  {' ', 4, 0}, {0,    0, 0},  {0, 0, 0}, {0, 0, 0}},
     {{'#',  1, 0},  {0,    0, 0}, {0,   0, 0},  {0,   0, 0},  {'\t',4, 0}, {0,    0, 0},  {0, 0, 0}, {0, 0, 0}},
     {{'=',  4, NV}, {0,    0, 0}, {0,   0, 0},  {0,   0, 0},  {'\n',0, NL},{0,    0, 0},  {0, 0, 0}, {0, 0, 0}},
     {{'[',  2, NS}, {0,    0, 0}, {0,   0, 0},  {0,   0, 0},  {0,   4, NV},{0,    0, 0},  {0, 0, 0}, {0, 0, 0}},
     {{0,    0, NO}, {0,    0, 0}, {0,   0, 0},  {0,   0, 0},  {0,   0, 0}, {0,    0, 0},  {0, 0, 0}, {0, 0, 0}},
  };

  fp = fopen(filename, "r");
  if (fp == NULL)
    {
      fprintf(stderr, "open file %s error\n", filename);
      return 1;
    }

  line = 1;
  while((in = getc(fp)) != EOF) {
      k = j = iFound = 0;
      while(stable[j][state][0] != 0) {
          if (in == stable[j][state][0]) { // is special char
              iFound = 1;
              k = j;
              break;
          }
          j++;
      } /* end while */

      if (!iFound) k = j;
      if (state == -1) 
        {
          fclose(fp);
          return 1;
        }

      actionIndex = stable[k][state][2];
      if (actionIndex != 0) 
          if ((state_action[actionIndex])(state, in) != 0) 
            {
              fclose(fp);
              return 1;
            }

      state = stable[k][state][1];  //set next state

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
  int iFound = 0;

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

int main(int argc, char **argv) {
  int result = 0;
  Options *opt;
  Sections *sec;
  char out[1024];

  result = cfgParse("a.ini");
  if (result != 0) return 1;

  printf("\n==================GET RESULT:GLOBAL======================\n");
  memset(out, 0x00, sizeof(out));
  cfgGet(NULL, "aa", out);
  printf("Global section, aa=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet(NULL, "a", out);
  printf("Global section, a=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet(NULL, "b", out);
  printf("Global section, b=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet(NULL, "c", out);
  printf("Global section, c=[%s]\n", out);

  printf("\n==================GET RESULT:[ab;cdefg]======================\n");
  memset(out, 0x00, sizeof(out));
  cfgGet("ab;cdefg", "c", out);
  printf("Named section [ab;cdefg], c=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet("ab;cdefg", "d", out);
  printf("Named section [ab;cdefg], d=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet("ab;cdefg", "e", out);
  printf("Named section [ab;cdefg], e=[%s]\n", out);

  printf("\n==================GET RESULT:[xxxx]======================\n");
  memset(out, 0x00, sizeof(out));
  cfgGet("xxxx", "e", out);
  printf("Named section [xxxx], e=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet("xxxx", "m", out);
  printf("Named section [xxxx], m=[%s]\n", out);

  memset(out, 0x00, sizeof(out));
  cfgGet("xxxx", "n", out);
  printf("Named section [xxxx], n=[%s]\n", out);

  //cfgPrint();
  cfgFree();
  return result;
}

