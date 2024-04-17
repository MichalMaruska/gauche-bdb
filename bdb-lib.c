/* support functions for bdb.stub */

/* #include <alloca.h> */

#include "bdb-lib.h"
#if 0
#define DB(x)     printf x
#else
#define DB(x) do { ; } while (0)
#endif




/***  `env'  */

/* finalization  */
static
void db_env_finalize(ScmObj obj, void* data)
{
   /* no need to test, if it is indeed a DB_ENV object:
    * the finalizer has just been deduced from the structure itself! */
   DB_ENV* e = SCM_DB_ENV(obj);
   if (e) {
      e->close(e,0);	/* fixme:*/
      SCM_DB_ENV(obj) = NULL;
   }
}


#define  gc_alloc 1

void *my_malloc(size_t s)
{
   Scm_Warn("%s: %d", __FUNCTION__, s);
   return GC_malloc_atomic(s);
}

void my_free(void* p)
{
#if DEBUG
   Scm_Warn("--------------%s-------------", __FUNCTION__);
#endif
   return GC_free(p);           /* is that a no-op? */
}





/* constructor */
ScmObj new_db_env (DB_ENV* env)
{
   ScmDbEnv* g = SCM_NEW(ScmDbEnv);
   SCM_SET_CLASS(g, SCM_CLASS_DB_ENV);
   g->env = env;

#if 0
   if (env) {
      env->app_private = (void*) g;
   }
#endif
   if (env) {
      env->set_alloc(env, GC_malloc_atomic, GC_realloc,
#if gc_alloc
                     my_free
#else
                     GC_free
#endif
                     );
   }
   Scm_RegisterFinalizer(SCM_OBJ(g), db_env_finalize, NULL);
   return SCM_OBJ(g);
}


/* top level: */
ScmObj mmc_db_env_create(int flags)
{
   DB_ENV* E;
   int result = db_env_create(&E, flags);
   if (result){
      Scm_Error("db_env_create failed %d", result);
   };
   SCM_RETURN (new_db_env(E));
}


/* `TXN' */
/* finalization ... why is this not (define-cproc ?? */
static void db_txn_finalize(ScmObj obj, void* data)
 {
   // DB_TXN* e = SCM_DB_TXN(obj);
    /* fixme: */
    /* if (e) { e->close(e,0); e = NULL; }	*/
 }


ScmObj new_db_txn (DB_TXN* e)
{
   ScmDbTxn* g = SCM_NEW(ScmDbTxn);
   SCM_SET_CLASS(g, SCM_CLASS_DB_TXN);
   g->txn = e;
   Scm_RegisterFinalizer(SCM_OBJ(g), db_txn_finalize, NULL);
   return SCM_OBJ(g);
}



/* Printer: */
void dbtxn_print(ScmObj obj, ScmPort *out, ScmWriteContext *ctx)
{
  DB_TXN* E=SCM_DB_TXN(obj);
  if (E) {
    /*   Scm_Printf(out, "#<db-txn %d>", txn_id(E);*/
    Scm_Printf(out, "#<db-txn %d>", E->id(E));
  } else {
    Scm_Printf(out, "#<db-txn null>");
  }
}


/* Printer: */
void
dbe_print(ScmObj obj, ScmPort *out, ScmWriteContext *ctx)
{
   int major, minor, patch;
   DB_ENV* E=SCM_DB_ENV(obj);
   if (E)
      {
         char *version=db_version(&major, &minor, &patch);
         Scm_Printf(out, "#<db-env %s(%d,%d,%d)>",
                    version,
                    major, minor, patch); /* fixme: no name */
      }
   else Scm_Printf(out, "#<db-env null>") ;
}



/* `DB_HANDLE' */
/* Error handling */
void
Scm_DBError(const DB_ENV *dbenv, const char *errpfx, const char *msg)
{
   /* Is it correct?  This throws out of a stack of C functions! */
   /* Maybe I should abort! */
#if 0
   Scm_Error
#else
   Scm_Panic
#endif
      ("%s: %s", errpfx, (msg)?:": unknown error");
};



/* finalization ... why is this not (define-cproc ?? */
static void
db_finalize(ScmObj obj, void* data)
{
   DB* e = SCM_DB(obj);
   if (e) {
      e->close(e,0);
      SCM_DB(obj) = NULL;
   } else if (((ScmDb*) obj)->open_p){
      Scm_Panic("%s: ->db is NULL!", __FUNCTION__);
   }
   /* fixme: we should refcount DBs in an ENV*/
}


/* the basic constructor: */
ScmObj
new_db (DB* db, ScmObj env_s)
{
   ScmDb* sdb = SCM_NEW(ScmDb);
   ScmDbEnv* env = (ScmDbEnv*) env_s;

   SCM_SET_CLASS(sdb, SCM_CLASS_DB);

   sdb->db = db;
   sdb->open_p = 0;
   sdb->key_type = sdb->value_type = db_thang_type_uvector;

   /* reverse pointer */
   db->app_private = sdb;

   /* fprintf(stderr, "setting the error handler\n"); */
   db->set_errpfx(db,"gauche-bdb:");
   db->set_errcall(db,Scm_DBError);

   /* it could inherit from the env */
#if gc_alloc
#if DEBUG
   Scm_Warn("%s: db->set_alloc = GC_malloc_atomic", __FUNCTION__);
#endif

#if 0 /* wrong! */
   /* Note, that below we indeed call this f. with NULL arg! */
   if(!env)
      Scm_Panic("%s:env argument is NULL!", __FUNCTION__);
#endif


   if (env && env->env){
      /* *** ERROR: gauche-bdb:: DB->set_alloc: method not permitted when environment specified */
      /* assert(db->get_env(db) == env->env); */
      const char* homep;
      DB_ENV *dbenv = db->get_env(db);

      dbenv->get_home(dbenv,&homep);
#if DEBUG
      Scm_Warn("%s: db has an ENV associated (homedir is %s, so it will use its allocation routines",
               __FUNCTION__, homep);
#endif
   } else {
      /* Provocation? */
      db->set_alloc(db, GC_malloc_atomic, GC_realloc, my_free);
      /* my_malloc   GC_malloc_atomic GC_free GC_free*/
   }
#endif  /* gc_alloc */

   Scm_RegisterFinalizer(SCM_OBJ(sdb), db_finalize, NULL);
   return SCM_OBJ(sdb);
}




/*  todo: look at gauche stubs to see :init-keywords  */

/* scheme constructor:  */
ScmObj Scm_db_allocate(ScmClass *klass, ScmObj initargs)
{
   SCM_ENTER_SUBR("db-allocate");
  /* ScmObj flags_scm = SCM_ARGREF(0); */
   int flags = 0;
  /* flags from initargs ? */
   DB* db;
   int result = db_create(&db,NULL, flags);

   if (result){
      Scm_Error("db_create failed %d", result);
   }
   db->set_errpfx(db,"gauche-bdb");
   db->set_errcall(db,Scm_DBError);
 /* free(stat)*/
   SCM_RETURN (new_db(db, NULL));
};





void
db_print(ScmObj obj, ScmPort *out, ScmWriteContext *ctx)
{
   DB* D = SCM_DB(obj);

   /* open?
    * type ?
    */

   if (!(D)) {
      /* This is impossible! */
      Scm_Printf(out, "#<db-handle: null>");
      Scm_Panic("%s: ->db is NULL!", __FUNCTION__);
   }

   else if (! ((ScmDb*) obj)->open_p)
      Scm_Printf(out, "#<db-handle: not open>");
   else
      {
         int fd;
         int res;

         /* get the FD */
         res=D->fd(D, &fd);
         if (res != 0) fd=-1;

         /* get the TYPE */
         DBTYPE type;         /*fixme: */
         res=D->get_type((D), &type);
         char* text_type;
         switch (type) {
         case DB_QUEUE:
            text_type="queue";
         case DB_RECNO:
            text_type="recno";
            break;
         case DB_HASH:
            text_type="hash";
            break;
         case DB_BTREE:
            text_type="btree";
            break;
         default:
            text_type="unknown";
         }

         Scm_Printf(out, "#<db-handle (%d,%s)>",
                    fd,
                    text_type);
      };
}


/****  `DBT'  */
void dbt_print(ScmObj obj, ScmPort *out, ScmWriteContext *ctx)
{
   DBT* D=SCM_DBT(obj);
   if (D) {
      Scm_Printf(out, "#<db-thang %d/%d %d@%d f:%d>", D->size, D->ulen, D->dlen, D->doff, D->flags);
   } else
      Scm_Printf(out, "#<db-thang null>") ;
}

#define BETWEEN(a,min,max)   ((a <= max) && (a >= min))


/* constant e.e. the ->data part is in the uvector! */
DBT* db_thang_from_uvector(ScmUVector* v, int start, int end, int capacity,
			   int partial, int offset, int flags)
{
   int len = SCM_UVECTOR_SIZE(v);
   int eltsize;

   /* SCM_CHECK_START_END(start, end, capacity);*/

   if (! BETWEEN(start,0,len))
      Scm_Error("start out of domain, %d (%d,%d)", start, 0, len);
   if (! BETWEEN(end,0,len))
      Scm_Error("end out of domain, %d (%d,%d)", end, 0, len);

   if (! BETWEEN(capacity,0,len + 1))
      Scm_Error("capacity out of domain, %d (%d,%d)", capacity, 0, len);

   eltsize = Scm_UVectorElementSize(Scm_ClassOf(SCM_OBJ(v)));
   SCM_ASSERT(eltsize >= 1);


   DBT* my_dbt = (DBT*) malloc(sizeof (DBT));        /* out of memory  */
   if (! my_dbt)
      Scm_Panic("out of memory\n");
   memset(my_dbt, 0, sizeof(DBT));

   my_dbt->data= (char*)v->elements + start*eltsize;
   my_dbt->size= (end - start) * eltsize;
   my_dbt->ulen= capacity?(capacity * eltsize):my_dbt->size;   /*  capacity ?? when we start at a >0 offset!!! */

   my_dbt->dlen= partial;
   my_dbt->doff= offset;

   /* must be DB_DBT_USERMEM  */
   if (flags & (DB_DBT_REALLOC | DB_DBT_MALLOC))
      Scm_Panic("%s: wrong flags for the DB thang (memory handling)\n", __FUNCTION__);
   my_dbt->flags = flags;
   /* SCM_RETURN(Scm_MakeInteger(1)); */

   return my_dbt;
}



/* finalization ... why is this not (define-cproc ?? */
static void db_thang_finalize(ScmObj obj, void* data)
{
/*  no need to test. the finalizer has just been deduced from the structure itself ? */
   DBT* dbt = SCM_DBT(obj);
   if (dbt){
#if DEBUG
      Scm_Warn("%s: ->data?\n", __FUNCTION__);
#endif
      if (dbt->flags & (DB_DBT_REALLOC | DB_DBT_MALLOC)) {
         /* Fixme:  Could we keep the same thang in 2 SCM_THangs ? */
         /*
          * -> data is ??
          * -> key is ??
          */
         /* fixme: I should use the DB->free function! */
#if gc_alloc
         my_free(dbt->data);
#else
         GC_free(dbt->data);
#endif
      }
      /* The DBT itself is malloc-ed by ourselves! */
      free (dbt);
      SCM_DBT(obj) = NULL;
   }
}

/* This is called in 2*/
ScmObj new_db_thang (DBT* e)
{
   ScmDbT* g = SCM_NEW(ScmDbT);
   SCM_SET_CLASS(g, SCM_CLASS_DBT);
   g->thang=e;
   Scm_RegisterFinalizer(SCM_OBJ(g), db_thang_finalize, NULL);
   return SCM_OBJ(g);
}


void
dispose_thang(int freshly_allocated, DBT* key_thang)
{
   if (freshly_allocated)
      free (key_thang);
   /* return NULL; */
}



/*  converting scheme data ->   read-only DBT*
 *  note: The DBT->data muste never be freed().
 *   it points to scheme memory, or in some case to same memory pices as DBT itself (ugly hack)
 *
 *   returns:  have to free?
 *      0  no
 *      1  yes
 *      2  only the boiler-plate
 * */
int
scheme_2_thang(const ScmObj data, DBT** real_data, int type)
{

   /* Scm_Warn("%s  %d !", __FUNCTION__, type); */

   /* thang is always ready! by itself.*/
   if (SCM_DBT_P(data))
      {
         *real_data = SCM_DBT(data);
         return 0;
      }

   switch (type){
   case db_thang_type_string:
      if SCM_STRINGP(data) {
            DBT*  rd = malloc (sizeof(DBT));
            /* todo: use bzero! */
            memset(rd, 0, sizeof(DBT));

            /* is ->db const  ?? */
            rd->data = (char*) Scm_GetStringConst(SCM_STRING(data));
            rd->size = SCM_STRING_SIZE(SCM_STRING(data));
#if 0
            rd->ulen = rd->size;
            rd->flags = DB_DBT_USERMEM;
#endif
            *real_data = rd;
            return 1;
         } else {
         /* fixme: error! */
         Scm_Error("string needed, but got ", data);
      }
      break;
   case db_thang_type_int: {
      if SCM_INTP(data) {
            DBT*  rd;
            DB(("%s  intp !\n", __FUNCTION__));

            /* fixme:  we need to know, if we want string representation, or int4 */
            rd = malloc (sizeof(DBT) + sizeof(int) );
            memset(rd, 0, sizeof(DBT));
            rd->data = rd + 1;/* ((char*) rd + sizeof(DBT)); */
            *((int *) rd->data) =  Scm_GetInteger(data); /*  */
            rd->size = (sizeof (int));
#if 0
            rd->ulen = rd->size;
            rd->flags = DB_DBT_USERMEM; /* fixme: This is a mess! this memory must NOT be freed */
#endif
            *real_data = rd;
            return 1;
         } else
         Scm_Error("integer needed, but got ", data);
      break;
   }

   case db_thang_type_uvector: {
      if (SCM_UVECTORP(data)) {

         DBT* my_dbt = db_thang_from_uvector(
            /* eltsize = Scm_UVectorElementSize(Scm_ClassOf(SCM_OBJ(data))); */
            SCM_UVECTOR(data), 0, SCM_UVECTOR_SIZE(data),
            0,                  /* count itself! */
            0, 0,  DB_DBT_USERMEM);

         *real_data = my_dbt;	/* fixme! */
         return 2;              /* fixme! */
         /* SCM_RETURN(new_db_thang(my_dbt)); */
      } else
         Scm_Error("uvector needed, but got ", data);
      break;
   }
   case db_thang_type_scm: {
      ScmObj s_port = Scm_MakeOutputStringPort(1); /* private! */
      Scm_Write(data, s_port, 0); /* ??? */

#if 0
      {
         Scm_Warn("%s:", Scm_GetStringConst(SCM_STRING (Scm_GetOutputString(SCM_PORT(s_port)))));
      };
#endif
      return scheme_2_thang(Scm_GetOutputString(SCM_PORT(s_port), 0),
                            real_data,
                            db_thang_type_string);
      /* Scm_GetStringConst(Scm_GetOutputString(sport, 0)); */
   }
   }

   Scm_Error("%s: data cannot be converted!", __FUNCTION__);
   /* return real_data;*/
   return -1;
}


#if 0
/* scheme value ->  DBT   returns `iff' we freshly allocated the memory for the value */

/* have to free? */
/* mmc: what's the difference wrt.  `scheme_2_thang'  */
static
int extract_key_dbt(const ScmObj data, DBT** real_data)
{
/* recno, number
 * string
 * This would need   alloca (as in Xfree: not limited to the C stack frames, but  )
 */

   DB(("%s\n", __FUNCTION__));

   if (SCM_DBT_P(data)){
      *real_data = SCM_DBT(data);
      return 0;
   } else if SCM_STRINGP(data) {
      DBT*  rd = malloc (sizeof(DBT));
      memset(rd, 0, sizeof(DBT));

      /* is ->db const  ?? */
      rd->data = (char*) Scm_GetStringConst(SCM_STRING(data)); /* fixme: can we really give the DB a shared string? will it not overwrite it? */
      rd->size = SCM_STRING_SIZE(SCM_STRING(data));
      *real_data = rd;
      return 1;

   } else if SCM_INTP(data) {
      DB(("%s  intp !\n", __FUNCTION__));
      DBT*  rd;

      /* fixme:  why  db_recno_t??   if we use a  fixnum ??   this is not correct ! */

      rd = malloc (sizeof(DBT) + sizeof(db_recno_t) );
      memset(rd, 0, sizeof(DBT));
      rd->data = ((char*) rd + sizeof(DBT));
      *((db_recno_t*) rd->data) = (db_recno_t)  Scm_GetInteger(data); /*  */
      rd->size = (sizeof (db_recno_t));
      /* rd->ulen = (sizeof (db_recno_t)); */
      *real_data = rd;
      return 1;
   } else Scm_Error("data not string, or <thunk>, but ...");
   /* return real_data;*/
};

#endif




/* this is inverse  */

/* [27 giu 04]: the DB layer starts to allocate via gc_malloc_  !!
 * t->data was malloced() !!
 *  mmc:  i would like to free(1) the DBT, but reuse the ->data.   it is 6 x int32  but i use stack for DBT !!
 *  */
ScmObj
convert_thang_to_scheme(DBT* t, int type)
{
   ScmObj k;


   /* All ->data coming here was malloc-ed by the DB (via our GC_malloc) */
   if (t->flags == DB_DBT_USERMEM)
      Scm_Panic("%s: not malloc-ed ->data (as this function is designed to treat, only)", __FUNCTION__);


   switch (type) {
      /* fixme:   date ?? timestamp */

   case db_thang_type_boolean:
      Scm_Warn("%s: type_boolean is implemented poorly!", __FUNCTION__);
      if (t->data)              /* fixme! t->data[0] = 'f' ?? */
      /* fixme:  */
         k = SCM_TRUE;
      else k = SCM_FALSE;
      break;

   case db_thang_type_uvector:
      /* fixme: is the thang comming with malloc-ed ->data ?  */
      k = Scm_MakeU8VectorFromArrayShared(t->size, t->data);
      break;

   case db_thang_type_scm: {
      /* copy & put 0 ? */
      /* mmc:  change it to malloc?
       *  There is a limit for alloca! */
      /* if (t->size > 8192) */

      /* This is very dumb! */
      char* a = (char*) alloca(t->size +1);
      memcpy(a, t->data,t->size);
      a[t->size] = 0;
      return Scm_ReadFromCString(a);

#if 1                           /* This is a case when we don't need the memory anymore! */
      if (t->flags != DB_DBT_USERMEM)
         my_free(t->data);
#endif
   }

   case db_thang_type_string:
      /* fixme! */
#if 0
      if (!t->data)
         k = SCM_EOF;
#endif
      /* this makes a pointer inside the atomic mem-space. No copying!*/
      k = Scm_MakeString(t->data,
                         t->size,
                         /* fixme: t->len  */
                         -1,
                         /* this is crucial!! */
                         SCM_MAKSTR_IMMUTABLE
                         /* SCM_MAKSTR_COPYING */
         );             /* SCM_MAKSTR_IMMUTABLE
                         * SCM_MAKSTR_COPYING
                         * SCM_MAKSTR_INCOMPLETE    mmc!*/
#if 0
      /* This would of course be very wrong!  here*/
      if (t->size > 0)
         GC_free(t->data);
#endif
      break;


   case db_thang_type_int:
      if (t->data)
         {
            k = Scm_MakeInteger(* (int*)t->data);

#if 1                           /* This is a case when we don't need the memory anymore! */
            if (t->flags != DB_DBT_USERMEM)
               my_free(t->data);
#endif
         }
      else
         {
            k = SCM_EOF;
         }
      /* this time i would free it! */
      break;
   default:
      k = Scm_MakeU8VectorFromArrayShared(t->size, t->data);
   };

   /* should i `free' ? */
   return k;
}


/*
 * return either EOF or the value converted.
 */
ScmObj my_db_get(DB* db, DB_TXN* txnid, ScmObj key, int flags)
{
   DBT data;
   int result;

   CHECK_DB(db);

   INIT_THANG(data); /* data.flags = DB_DBT_MALLOC;  */


   /* mmc: this cannot be on the stack:
             might happen to be a pointer to a pre-existing value */
   DBT* key_thang;
   int freshly_allocated =
      scheme_2_thang(key, &key_thang,
                     ((ScmDb* )(db->app_private))->key_type);
   /* extract_key_dbt(key, &real_key); */

   result = db->get(db, txnid, key_thang, &data, flags);

   dispose_thang(freshly_allocated, key_thang);


   if (result == DB_NOTFOUND){
      SCM_RETURN(SCM_EOF);
   };


   /* fixme!  this should throw! */
   if (result){
      db->err(db, result, "db-get-string: flags: %d", flags);
   }

/* fprintf(stderr, "received: %d %d %s\\n", data.size, data.ulen, data.data); */

   ScmObj s = convert_thang_to_scheme(&data, ((ScmDb* )(db->app_private))->value_type);

#if  (! gc_alloc)
   if (data.data)
      free (data.data);
#endif

   SCM_RETURN(s);
};


/* New strategy:
 * ~~~~~~~~~~~~~
 * KEY is either a scm_thang or not.
 * If it is `not', we will create a (C) thang, use it, and destroy it.
 * ->data will be in scheme memory all the time, unless it is a number!
 * if it is a number it will be freed together with the thang!
 *
 * If it is an `scm_thang', we will use that, and will not malloc/free any memory.
 */

ScmObj my_db_put(DB* db, DB_TXN* txnid, ScmObj key, ScmObj data, int flags)
{
   DBT* db_data;
   DBT* db_key;

   CHECK_DB(db);
   int freshly_allocated_key = scheme_2_thang(key, &db_key,
                                              ((ScmDb* )(db->app_private))->key_type);

   int freshly_allocated_data = scheme_2_thang(data, &db_data,
                                               ((ScmDb* )(db->app_private))->value_type);


   int result = db->put(db, txnid, db_key, db_data, flags);


   dispose_thang(freshly_allocated_data, db_data);


   dispose_thang(freshly_allocated_key, db_key);

   /* fixme!  this should throw! */
   if (result){
      /* ->put failed on */
      db->err(db, result, "db-put: flags: %d", flags);
   }

/* fprintf(stderr, "received: %d %d %s\\n", data.size, data.ulen, data.data); */

   SCM_RETURN(Scm_MakeInteger(result));
};



/* New Strategy */
ScmObj my_db_del(DB* db, DB_TXN* txnid, ScmObj key, int flags)
{
   DBT* db_key;

   CHECK_DB(db);
   int freshly_allocated_key = scheme_2_thang(key, &db_key,
                                              ((ScmDb* )(db->app_private))->key_type);

   int result = db->del(db, txnid, db_key, flags);

   dispose_thang(freshly_allocated_key, db_key);


   /* fixme!  this should throw! */
   if (result){
      /* ->put failed on */
      db->err(db, result, "db-put: flags: %d", flags);
   }

   SCM_RETURN(Scm_MakeInteger(result));
}






/***** `db_cursor'  */
static void db_cursor_finalize(ScmObj obj, void* data);

ScmObj
new_db_cursor (ScmDb* db_scm, DBC* e)
{
  // DB* db = SCM_DB(db_scm);

  ScmDbC* g = SCM_NEW(ScmDbC); /* this is not atomic !!! */
  SCM_SET_CLASS(g, SCM_CLASS_DBC);
  g->cursor=e;
  g->db = db_scm;

  /* inheritance! */
  g->value_type = db_scm->value_type;
  g->key_type = db_scm->key_type;

#if DEBUG
  Scm_Warn("%s: value type: %d", __FUNCTION__, g->value_type);
#endif
  Scm_RegisterFinalizer(SCM_OBJ(g), db_cursor_finalize, NULL);
  return SCM_OBJ(g);
}


static void db_cursor_finalize(ScmObj obj, void* data)
{

   DBC* dbc = SCM_DBC(obj);
   ScmDbC* sc = (ScmDbC*) obj;

   sc->db = NULL;
   /* sc->value_type  */

   if (dbc) {
      /* cursor is created by BDB, using our malloc! */
      /* my_free(e); */
      int res = dbc->c_close(dbc);
      if (res)
         Scm_Panic("%s: c_close returned error!", __FUNCTION__);

      SCM_DBC(obj) = NULL;
   }
}


ScmObj db_cursor_close(ScmObj cursor)
{
   /* todo: Check the DB?   also, ref-counting? */
   DBC* dbc = SCM_DBC(cursor);
   int ret = dbc->c_close(dbc);
   SCM_DBC(cursor) = NULL;
   return Scm_MakeInteger(ret);
}





/* mmc: Why do I keep the converted thangs in the cursor_scm ??   fixme!


 * KEY, DATA and FLAGS  `act' on the cursor. the result is
 * KEY and DATA updated in some way.  And we return them as 2 values.
 *
 */
ScmObj
scm_db_cursor_get (ScmDbC* cursor_scm, DBT* key, DBT* data, int flags)
{
   if ((key->flags == DB_DBT_USERMEM) || (data->flags == DB_DBT_USERMEM))
      Scm_Panic("%s: not malloc-ed ->data", __FUNCTION__);

   DBC* cursor  = SCM_DBC(cursor_scm);
   int result = cursor->c_get(cursor, key, data, flags);
   /* fixme:  if  result -> data ? */

   if (result == DB_NOTFOUND)
      {
         cursor_scm->key = cursor_scm->value = SCM_EOF;
      }
   else if (result)
      {
         DB* db = SCM_DBC_DB(cursor_scm);
         db->err(db, result, "flags: %d", flags);
         /* Scm_Error(\"db-get-recnum, %d\", result); */
      }
   else
      {
         cursor_scm->key   = convert_thang_to_scheme(key, cursor_scm->key_type);
         cursor_scm->value = convert_thang_to_scheme(data, cursor_scm ->value_type);
      };
   return Scm_Values2(cursor_scm->key, cursor_scm->value);
};




/* ScmDbC* cursor_scm */
ScmObj
scm_db_cursor_put (ScmObj cursor_scm, ScmObj key, ScmObj data, int flags)
{
   /* todo: Check the cursor! */
   /* todo: CHECK_DBC(scm_cursor);*/

   ScmDbC* cursor_s  =(ScmDbC* ) cursor_scm;

#if 0
   ScmDbC* cursor_scm = ((ScmDbC* )(cursor->app_private));
#else
   DBC* cursor  = SCM_DBC(cursor_scm);
#endif
   DBT* db_data;
   DBT* db_key;

   int freshly_allocated_key = scheme_2_thang(key, &db_key,
                                              cursor_s->key_type);

   int freshly_allocated_data = scheme_2_thang(data, &db_data,
                                               cursor_s->key_type);


   int result = cursor->c_put(cursor, db_key, db_data, flags);


   dispose_thang(freshly_allocated_data, db_data);
   dispose_thang(freshly_allocated_key, db_key);



   /* fixme!  this should throw! */
   if (result){
      /* ->put failed on */
      DB* db = SCM_DB(cursor_s->db);
      db->err(db, result, "%s: flags: %d", __SCM_FUNCTION__, flags);
   }
   SCM_RETURN(Scm_MakeInteger(result));
};



/* Printer:*/
void dbc_print(ScmObj obj, ScmPort *out, ScmWriteContext *ctx)
{
   DBC* D=SCM_DBC(obj);
   if (D) {
      Scm_Printf(out, "#<db-cursor>");
   } else
      Scm_Printf(out, "#<db-cursor null>") ;
}






/* `Stats' */
/* static */
ScmObj new_db_hash_stat (DB_HASH_STAT* s)
{
   ScmDbHashStat* g = SCM_NEW(ScmDbHashStat);
   SCM_SET_CLASS(g, &ScmDB_HASH_STAT_class);
   g->stat=s;
   /*  Scm_RegisterFinalizer(SCM_OBJ(g), db_env_finalize, NULL);*/
   return SCM_OBJ(g);
}

/* static */
ScmObj new_db_btree_stat (DB_BTREE_STAT* s)
{
   ScmDbBtreeStat* g = SCM_NEW(ScmDbBtreeStat);
   SCM_SET_CLASS(g, &ScmDB_BTREE_STAT_class);
   g->stat=s;
   /*  Scm_RegisterFinalizer(SCM_OBJ(g), db_env_finalize, NULL);*/
   return SCM_OBJ(g);
}



/***** purely-C functions which might help the DB system: */

u_int32_t
compare_prefix(DB *dbp, const DBT* a, const DBT *b)
{
   size_t cnt, len;
   u_int8_t *p1, *p2;

   cnt = 1;
   len = a->size > b->size ? b->size : a->size;
   for (p1 =
           a->data, p2 = b->data; len--; ++p1, ++p2, ++cnt)
      if (*p1 != *p2)
         return (cnt);
/*
 * They match up to the smaller of the two sizes.
 * Collate the longer after the shorter.
 */
   if (a->size < b->size)
      return (a->size + 1);
   if (b->size < a->size)
      return (b->size + 1);
   return (b->size);
}






/*** Hack for initialization stub */
void internal_init_bdb(ScmModule*);

void Scm_Init_libgauche_bdb(void)
{
   ScmModule *mod;
   SCM_INIT_EXTENSION(bdb);
   mod = SCM_MODULE(SCM_FIND_MODULE("bdb", TRUE)); /* create if necessary*/

   {
      int major, minor, patch;
      char* version = db_version(&major, &minor, &patch);
      if ((major != DB_VERSION_MAJOR)
          || (minor != DB_VERSION_MINOR)
          || (patch != DB_VERSION_PATCH))
         Scm_Error("the Berkeley library linked is %s. But this gauche-bdb module was compiled with version: %s\n",
                   version,
                   DB_VERSION_STRING);
   }

   internal_init_bdb(mod);

}
