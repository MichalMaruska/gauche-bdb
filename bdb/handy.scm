
(define-module bdb.handy
  (export
   scan-db
   copy-recno
   db->ndata
   )
  (use bdb)
  (use mmc.simple)
  ;(use gauche.uvector)
  )
(select-module bdb.handy)
  

(define (db->ndata db)
  (ref (db-btree-stat db 0) 'bt-ndata))


(define (scan-db db  callback)
  (for-numbers* i 1 (db->ndata db)
    (let1 record (db-get-recnum db null-transaction i)
                                        ;(fork-event-deserialize
      ;; u8vector
      (callback record)
                                        ;(if (ref event 'press)
      )))


;; copy between 2 RECNOs:
(define (copy-recno from to)
  (scan-db from
           (lambda (record)
             (db-put-recnum to null-transaction 1
                            record ;(db-make-thang record)
                            DB_APPEND)
             )))