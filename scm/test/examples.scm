
~/gauche/gg/gauche-mmc/text/traduzioni.scm
~/gauche/gg/gauche-kahua-mmc/bin/dump-trad.scm
~/gauche/gg/gauche-kahua-mmc/bin/update-trad-complete.scm

;; how to open shared env & db:
(db-create-open :env (db-env-create-open
                      :home dir
                      :shared #t)
                :filename fff
                ;:open
                )



;; open-string-db-create
(db-create-open
 :filename filename
 :name name
 :type DB_HASH
 :create #t
 :exclude #t
 ;; fixme: use <string> <integer> ??
 :key-type db_thang_type_string
 :value-type db_thang_type_string
 )




