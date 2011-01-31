
;; copy  from PG to BDB

(define-module bdb.pg
  (export
   copy-pg->db
   )
  (use pg)
  (use pg-hi)
  (use bdb)
  (use pg.sql)
;(use gauche.uvector)
  )
(select-module bdb.pg)


(define (copy-pg->db query db . args)
  (let-optionals* args
      ((conn (pg-connect ""))
       (fields '(0 1)))

    ;; assert
    ;; the key & value types match that of the pg result!
    
    (let* ((result (pg-exec conn query)))
      ;; copy:
                                        ;(pg-ntuples result)
      (pg-foreach-result result fields
        (lambda (key value) 
          (db-put db null-transaction
                  key value 0)))
      ;;
      (db-sync db 0))))


