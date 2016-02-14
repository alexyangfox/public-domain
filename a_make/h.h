/*************************************************************************
|                                                                        |
|   H.H                                                         30.09.89 |
|   PAMAKE Utility:  header file                                         |
|                                                                        |
*************************************************************************/

#define uchar       unsigned char
#define bool        uchar
#define TRUE        (1)
#define FALSE       (0)
#define DEFN1       "makefile"              /*  Default names  */
#define LIFNAME     "TEMP_LIF.TMP"
#ifdef  VMS
#define LZ          (4096)                  /*  Line size  */
#else
#define LZ          (1024)                  /*  Line size  */
#endif
#define DOLLAR      1                       /* special dollar processing */

#ifdef VMS
#define PXNORMAL 1          /* normal exit() arg */
#define PXERROR  0          /* error exit() arg */
#define unlink(filename) remove(filename)
#else
#define PXNORMAL 0          /* normal exit() arg */
#define PXERROR  1          /* error exit() arg */
#define PFPROTO  1          /* use function prototypes */
#endif

#ifdef __TSC__
#define PAMDOS 1
#endif

#ifdef __TURBOC__
#define PAMAKE_FTIME 1
#define PAMDOS 1
#endif

#ifdef LATTICE
#define PAMAKE_FTIME 1
#define PAMOS2 1
#endif

#ifndef DOLLAR
#define dollar(p) (p)       /* dont bother to strip special dollars */
#endif

struct name
{
    struct name *           n_next;         /* Next in the list of names */
    char *                  n_name;         /* Called */
    struct line *           n_line;         /* Dependencies */
    long                    n_time;         /* Modify time of this name */
    uchar                   n_flag;         /* Info about the name */
};

#define N_MARK              0x01            /* For cycle check */
#define N_DONE              0x02            /* Name looked at */
#define N_TARG              0x04            /* Name is a target */
#define N_PREC              0x08            /* Target is precious */
#define N_DYND              0x10            /* Name was made by dyndep */
#define N_DOUBLE            0x20            /* Double colon */

struct  line
{
    struct line *           l_next;         /* Next line (for ::) */
    struct depend *         l_dep;          /* Dependents for this line */
    struct cmd *            l_cmd;          /* Commands for this line */
};

struct  depend
{
    struct depend *         d_next;         /* Next dependent */
    struct name *           d_name;         /* Name of dependent */
};

struct  cmd
{
    struct cmd *            c_next;         /* Next command line */
    struct lif *            c_lif;          /* LIF lines for this command */
    char *                  c_cmd;          /* Command line */
};

struct  lif
{
    struct lif *            f_next;         /* Next LIF line */
    char *                  f_lif;          /* LIF line */
};

struct  macro
{
    struct macro *          m_next;         /* Next variable */
    char *                  m_name;         /* Called ... */
    char *                  m_val;          /* Its value */
    char *                  m_sub;          /* Temp subst value */
    uchar                   m_flag;         /* Infinite loop check */
};

#define M_LOOPCHK           0x01            /* For loop check */
#define M_ENV               0x02            /* ex-environment macro */
#define M_PERM              0x04            /* not resetable */

extern char *           myname;
extern struct name      namehead;
extern struct macro *   macrohead;
extern struct name *    firstname;
extern bool             silent;
extern bool             confirm;
extern bool             ignore;
extern bool             rules;
extern bool             dotouch;
extern bool             quest;
extern bool             domake;
extern bool             display;
extern char             str1[];
extern char             str2[];
extern int              lineno;
extern uchar            macrotype;
extern unsigned char    pamakeos2;

extern FILE *           ifile[4];
extern int              fln[4];
extern char             fname[4][80];
extern int              nestlvl;

#ifdef PFPROTO
extern  void            check(struct name *np);
extern  void            checklif(struct cmd *cp);
extern  void            circh(void);
extern  void            cleardynflag(struct name *np);
extern  unsigned long   curtime(void);
extern  void            docmds1(struct name *np,struct line *lp);
extern  void            docmds(struct name *np);
extern  void            dodisp(char *name,long t);
extern  void            doexp(char * *to,char *from,int *len,char *buf);
extern  char *          dollar(char *);
extern  void            dosetcmd(char *p);
extern  void            dostatcmd(char *p);
extern  int             dosh(char *p,int shell);
extern  int             dos_internal(char *s);
extern  bool            dyndep(struct name *np);
extern  void            expand(char *str);
extern  void            ifeoc(void);
extern  void            ifeof(void);
extern  int             ifproc(char *s,int n);
extern  void            input(void);
extern  int             istrue(char *s);
extern  bool            getline(char *str);
extern  struct macro *  getmp(char *name);
extern  char *          gettok(char * *ptr);
extern  void            killlif(void);
extern  void            main(int argc,char * *argv,char * *envp);
extern  int             make(struct name *np,int level);
extern  void            make1(struct name *np,struct line *lp,struct depend *qdp);
extern  void            makelif(struct cmd *cp);
extern  void            makerules(void);
extern  void            markmacros(void);
extern  void            modtime(struct name *np);
extern  struct name *   newname(char *name);
extern  struct depend * newdep(struct name *np,struct depend *dp);
extern  struct cmd *    newcmd(char *str,struct cmd *cp,struct cmd * *crp);
extern  struct lif *    newlif(char *str,struct lif *lp);
extern  void            newline(struct name *np,struct depend *dp,struct cmd *cp,int flag);
extern  void            precious(void);
extern  void            prt(void);
extern  char *          pstrstr(char *, char *);
extern  void            setdmacros(struct name *np,struct depend *qdp);
extern  struct macro *  setmacro(char *name,char *val);
extern  char *          suffix(char *name);
extern  void            touch(struct name *np);
extern  void            usage(void);
extern  int             pspace(int);
#ifdef __TSC__
#pragma save, call (reg_param => ())
extern  void            error(char *msg,...);
extern  void            fatal(char *msg,...);
#pragma restore
#else
extern  void            error(char *msg,...);
extern  void            fatal(char *msg,...);
#endif
#else
unsigned long           curtime();
char *                  dollar();
void                    error();
void                    fatal();
char *                  getmacro();
struct macro *          getmp();
char *                  gettok();
void                    input();
int                     make();
void                    makerules();
struct cmd *            newcmd();
struct depend *         newdep();
void                    newline();
struct name *           newname();
void                    precious();
char *                  pstrstr();
struct macro *          setmacro();
char *                  suffix();
void                    touch();
#endif
