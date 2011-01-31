#ifndef BDB_LIB_H
#define BDB_LIB_H

#define DEBUG 0

/*   macros           for both
 *   types                 "
 *
 *   SCM_CLASS_DECL(ScmDB_class);    for the .c !!!
 *   
 *   export of functions from  .c    for .stub
 *   
 *  */

/* free(): */
#include <stdlib.h>


#include <gauche.h>
#include <gauche/extend.h>	/*  why ?*/
#include <gauche/class.h>

/* fixme: This needs manual intervention: */
#include <db.h>
#include <gauche/uvector.h>



/****  `DB_ENV'  */
typedef struct ScmDbEnvRec{
    SCM_HEADER;
    DB_ENV* env;
 } ScmDbEnv;

SCM_CLASS_DECL(ScmDBE_class);



#define SCM_CLASS_DB_ENV       (&ScmDBE_class)
/* #define SCM_DB_ENV(obj)        ((DB_ENV*) (((ScmDbEnv*) obj)->env)) */
/* Lvalues: see http://gcc.gnu.org/gcc-3.4/changes.html */
#define SCM_DB_ENV(obj)        (((ScmDbEnv*) obj)->env)


#define SCM_DB_ENV_P(obj)      (Scm_TypeP(obj, SCM_CLASS_DB_ENV))

/* argument checks */
#define CHECK_SENV(env_scm) if (!SCM_DB_ENV(env_scm)) {Scm_Error("%s: BDB: cannot call ... on a closed Env!", __SCM_FUNCTION__);}
#if 0
#define CHECK_ENV(env) if (!env) {Scm_Error("%s: BDB: cannot call ... on a closed ENV!", __FUNCTION__);}
#endif


extern ScmObj new_db_env (DB_ENV* env);

void dbe_print(ScmObj, ScmPort*, ScmWriteContext*);


/**** `db_txn' */


SCM_CLASS_DECL(ScmDBTXN_class);
typedef struct ScmDbTxnRec{
    SCM_HEADER;
    DB_TXN* txn;
 } ScmDbTxn;

#define SCM_CLASS_DB_TXN       (&ScmDBTXN_class)
#define SCM_DB_TXN(obj)        (((ScmDbTxn*) obj)->txn)
#define SCM_DB_TXN_P(obj)      Scm_TypeP(obj, SCM_CLASS_DB_TXN)
/*unused*/
#define SCM_MAKE_DB_TXN(txn)



/* void db_txn_finalize(ScmObj obj, void* data); */
ScmObj new_db_txn (DB_TXN* e);
void dbtxn_print(ScmObj obj, ScmPort *out, ScmWriteContext *ctx);





ScmObj mmc_db_env_create(int flags);


/**** `db_handle' */
SCM_CLASS_DECL(ScmDB_HASH_STAT_class);
SCM_CLASS_DECL(ScmDB_BTREE_STAT_class);
SCM_CLASS_DECL(ScmDB_RECNO_STAT_class);
SCM_CLASS_DECL(ScmDB_QUEUE_STAT_class);

/* value types: I convert automatically */
enum {
   db_thang_type_scm,           /* read/write */
   db_thang_type_uvector,
   /* 2 */
   db_thang_type_string,
   db_thang_type_int,
   db_thang_type_boolean,
};



SCM_CLASS_DECL(ScmDB_class);
typedef struct ScmDbRec{
   SCM_HEADER;
   DB* db;
   int open_p;                            /*boolean */

   /* 0 uvector,  1  int, 2 c-string, 3 ?? */
   int key_type;
   int value_type;
} ScmDb;

#define SCM_CLASS_DB       (&ScmDB_class)
#define SCM_DB(obj)        (((ScmDb*) obj)->db)

#define SCM_DB_P(obj)      (Scm_TypeP(obj, SCM_CLASS_DB)) /*  && (SCM_DB_ENV(obj)) */
/*unused*/
#define SCM_MAKE_DB(db)

#define __SCM_FUNCTION__ __FUNCTION__
#define CHECK_SDB(db_scm)  if (!SCM_DB(db_scm)) {Scm_Error("%s: BDB: cannot call ... on a closed DB!", __SCM_FUNCTION__);}

#define CHECK_DB(db)  if (!db) {Scm_Error("%s: BDB: cannot call ... on a closed DB!", __FUNCTION__);}


ScmObj new_db (DB* db, ScmObj env);


void Scm_DBError(const DB_ENV *dbenv, const char *errpfx, const char *msg);



/* ScmObj new_db (DB* e); */

/* void db_finalize(ScmObj obj, void* data); */
// ScmObj Scm_db_allocate(ScmClass *klass, ScmObj initargs);

ScmObj Scm_db_allocate(ScmClass *klass, ScmObj initargs);



void db_print(ScmObj obj, ScmPort *out, ScmWriteContext *ctx);



/* `DBT'  */
typedef struct ScmDbTRec{
   SCM_HEADER;
   DBT* thang;
} ScmDbT;

SCM_CLASS_DECL(ScmDBT_class);


/* with threads this is a condition! */
#define INIT_THANG(t)   do {memset(&t, 0, sizeof(t)); t.flags = DB_DBT_MALLOC;} while (0)
        


#define SCM_CLASS_DBT       (&ScmDBT_class)
#define SCM_DBT(obj)        (((ScmDbT*) obj)->thang)
#define SCM_DBT_P(obj)      Scm_TypeP(obj, SCM_CLASS_DBT)
/*unused*/
#define SCM_MAKE_DBT(db)



extern ScmObj new_db_thang (DBT* e);
extern void dbt_print(ScmObj obj, ScmPort *out, ScmWriteContext *ctx);


extern DBT* db_thang_from_uvector(ScmUVector* v, int start, int end, int capacity, int partial, int offset, int flags);


extern int scheme_2_thang(const ScmObj data, DBT** real_data, int type);
extern ScmObj convert_thang_to_scheme(DBT* t, int type);


extern ScmObj my_db_get(DB* db, DB_TXN* t, ScmObj key, int flags);
extern ScmObj my_db_put(DB* db, DB_TXN* txnid, ScmObj key, ScmObj data, int flags);
extern ScmObj my_db_del(DB* db, DB_TXN* txnid, ScmObj key, int flags);


/**** `db_cursor' */

typedef struct ScmDbCRec{
   SCM_HEADER;
   DBC* cursor;

   ScmDb*  db;                     /* i keep a reference to the parent DB <db> .... so i can error ?*/
   ScmDbTxn* txn;

        /* Why these ? */
   ScmObj value;
   ScmObj key;

   /* could be in DB, but maybe we look at the data differently */
   int key_type;
   int value_type;
} ScmDbC;


SCM_CLASS_DECL(ScmDBC_class);
#define SCM_CLASS_DBC       (&ScmDBC_class)
#define SCM_DBC(obj)        (((ScmDbC*) obj)->cursor)
#define SCM_DBC_P(obj)      Scm_TypeP(obj, SCM_CLASS_DBC)

#define SCM_DBC_DB(obj)        (SCM_DB(((ScmDbC*) obj)->db))


#define check_flags_zero(flags) if (flags) {Scm_Error("flags must be 0!");}

/*unused*/
#define SCM_MAKE_DBC(db)

extern ScmObj new_db_cursor (ScmDb* d, DBC* e);
extern void dbc_print(ScmObj, ScmPort*, ScmWriteContext*);

/* extern void db_cursor_finalize(ScmObj obj, void* data); */

extern ScmObj db_cursor_close(ScmObj cursor);

extern ScmObj scm_db_cursor_get (ScmDbC* cursor_scm, DBT* key, DBT* data, int flags);

extern ScmObj scm_db_cursor_put (ScmObj cursor_scm, ScmObj key, ScmObj data, int flags);


/* `Stats' */
typedef struct ScmDbHashStatRec{
    SCM_HEADER;
    DB_HASH_STAT* stat;
 } ScmDbHashStat;

#define SCM_CLASS_DB_ENV       (&ScmDBE_class)
#define SCM_DB_HASH_STAT(obj)        ((DB_HASH_STAT*) (((ScmDbHashStat*) obj)->stat))
#define SCM_DB_HASH_STAT_BOX(obj)        ((ScmObj*) (((ScmDbHashStat*) obj)->stat))
#define SCM_DB_HASH_STAT_P(obj)      (Scm_TypeP(obj, ScmDB_HASH_STAT_class)) 

extern ScmObj new_db_hash_stat (DB_HASH_STAT* s);
extern ScmObj new_db_btree_stat (DB_BTREE_STAT* s);


/* stats for Btree  */
typedef struct ScmDbBtreeStatRec{
    SCM_HEADER;
    DB_BTREE_STAT* stat;
 } ScmDbBtreeStat;

#define SCM_CLASS_DB_ENV       (&ScmDBE_class)
#define SCM_DB_BTREE_STAT(obj)        ((DB_BTREE_STAT*) (((ScmDbBtreeStat*) obj)->stat))
#define SCM_DB_BTREE_STAT_BOX(obj)        ((ScmObj*) (((ScmDbBtreeStat*) obj)->stat))
#define SCM_DB_BTREE_STAT_P(obj)      (Scm_TypeP(obj, ScmDB_BTREE_STAT_class))





extern int lev(int* a,int lena, int* b, int lenb);

extern int* utf8_to_ucs(const char* s, int *len,int);

#endif

