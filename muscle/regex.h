#ifndef REGEX_H
#define REGEX_H

extern const char *re_comp(const char *);
extern bool  re_exec(const char *);
extern bool re_subs(const char *, char *);
const char *GetGroupStart(unsigned TagIndex);
unsigned GetGroupLength(unsigned TagIndex);

#endif /* REGEX_H */
