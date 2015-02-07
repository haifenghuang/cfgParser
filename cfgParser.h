#ifndef __CFG_PARSER_H__
#define __CFG_PARSER_H__

int cfgParse(const char *filename);
int cfgGet(const char *section, const char *key, char *out);
void cfgFree(void);

/* 如果希望调试，请打开注释 */
//void cfgPrint();

#endif
