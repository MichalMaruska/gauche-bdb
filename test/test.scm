#! /usr/bin/gosh

(use gauche.test)

(test-start "Gauche-Berkeley-DB TEST")

(use bdb)
(use bdb-hi)
(use gauche.uvector)
(test-module 'bdb)
(test-module 'bdb-hi)

(test-section "low level bdb")

;; make a directory & DB ENV.  Create a DB, fill in string key/data, & retrieve.
;;

(let1 tmpdir "./tmpdir"
  (format #t "creating a temporary Dir: ~a\n" tmpdir)
  (sys-mkdir tmpdir #o777)
  (let1 db-env (db-env-create 0)

    (slot-set! db-env 'temp tmpdir)
    ;(slot-set! db-env 'data-dir ".")
    (slot-set! db-env 'data-dir ".")

    (format #t "env: flags ~d, dir ~a, temp ~a\n"
            (ref db-env 'flags)
            (ref db-env 'data-dir)
            (ref db-env 'temp))

    (db-env-open db-env "."  (+ DB_CREATE
                                DB_INIT_MPOOL
                                DB_PRIVATE)
                 0)        ;; 0-> default!

    ;; create a DB:
    (let* ((txn null-transaction)
           (db (db-create db-env 0))       ; null-env
           )
      (db-open db txn "test.db" "" DB_BTREE DB_CREATE 0) ;

      ;;
      (db-put db txn
              (db-make-thang (string->u8vector "key"))
              (db-make-thang (string->u8vector "value")))


      (slot-set! db 'key-type db_thang_type_scm)
      (slot-set! db 'value-type db_thang_type_int)
      (db-put db txn 1 10)

      (db-put db txn "key" 11)

      (db-sync db 0)

      ;;
      (db-del db txn (db-make-thang (string->u8vector "key")) 0)

      ;; update?
      ;(sys-abort)
      (db-close db 0)
      )

    (db-env-close db-env 0)
    )

  ;(sys-rmdir tmpdir)
  )

;(test* "fold (list)" '(6 5 4 3 2 1)
;       (fold cons '() '(1 2 3 4 5 6)))




(test-section "High level bdb")


(test-end)
